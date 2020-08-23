#include <QCameraImageCapture>
#include <QThread>
#include <QFileInfo>

#include "gphotocamera.h"

namespace {
    constexpr auto capturingFailLimit = 10;
    constexpr auto viewfinderParameter = "viewfinder";
    constexpr auto waitForEventTimeout = 10;
}

using CameraWidgetPtr = std::unique_ptr<CameraWidget, int (*)(CameraWidget*)>;
using VoidPtr = std::unique_ptr<void, void (*)(void*)>;

QDebug operator<<(QDebug dbg, const CameraWidgetType &t)
{
    switch (t) {
    case GP_WIDGET_WINDOW:
        dbg.nospace() << "GP_WIDGET_WINDOW";
        break;
    case GP_WIDGET_SECTION:
        dbg.nospace() << "GP_WIDGET_SECTION";
        break;
    case GP_WIDGET_TEXT:
        dbg.nospace() << "GP_WIDGET_TEXT";
        break;
    case GP_WIDGET_RANGE:
        dbg.nospace() << "GP_WIDGET_RANGE";
        break;
    case GP_WIDGET_TOGGLE:
        dbg.nospace() << "GP_WIDGET_TOGGLE";
        break;
    case GP_WIDGET_RADIO:
        dbg.nospace() << "GP_WIDGET_RADIO";
        break;
    case GP_WIDGET_MENU:
        dbg.nospace() << "GP_WIDGET_MENU";
        break;
    case GP_WIDGET_BUTTON:
        dbg.nospace() << "GP_WIDGET_BUTTON";
        break;
    case GP_WIDGET_DATE:
        dbg.nospace() << "GP_WIDGET_DATE";
        break;
    }

    return dbg.space();
}

GPhotoCamera::GPhotoCamera(GPContext *context, const CameraAbilities &abilities,
                           const GPPortInfo &portInfo, QObject *parent)
    : QObject(parent)
    , m_context(context)
    , m_abilities(abilities)
    , m_portInfo(portInfo)
    , m_camera(nullptr, gp_camera_free)
    , m_file(nullptr, gp_file_free)
{
    connect(this, &GPhotoCamera::previewCaptured, this, &GPhotoCamera::capturePreview, Qt::QueuedConnection);
}

GPhotoCamera::~GPhotoCamera()
{
    closeCamera();
}

void GPhotoCamera::setState(QCamera::State state)
{
    if (m_state == state)
        return;

    auto previousState = m_state;

    if (QCamera::UnloadedState == previousState) {
        if (QCamera::LoadedState == state) {
            openCamera();
        } else if (QCamera::ActiveState == state) {
            startViewFinder();
        }
    } else if (QCamera::LoadedState == previousState) {
        if (QCamera::UnloadedState == state) {
            closeCamera();
        } else if (QCamera::ActiveState == state) {
            startViewFinder();
        }
    } else if (QCamera::ActiveState == previousState) {
        if (QCamera::UnloadedState == state) {
            closeCamera();
        } else if (QCamera::LoadedState == state) {
            stopViewFinder();
        }
    }
}

void GPhotoCamera::setCaptureMode(QCamera::CaptureModes captureMode)
{
    if (m_captureMode != captureMode) {
        m_captureMode = captureMode;
        emit captureModeChanged(captureMode);
    }
}

void GPhotoCamera::capturePhoto(int id, const QString &fileName)
{
    if (!isReadyForCapture()) {
        emit imageCaptureError(id, QCameraImageCapture::NotReadyError, tr("Camera is not ready"));
        return;
    }

    setMirrorPosition(MirrorPosition::Down);

    // Capture the frame from camera
    // See https://github.com/gphoto/libgphoto2/issues/156 for RAW+JPEG fix
    auto ret = gp_camera_trigger_capture(m_camera.get(), m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Failed to capture frame:" << ret;
        emit imageCaptureError(id, QCameraImageCapture::ResourceError, tr("Failed to capture frame"));
        return;
    }

    CameraEvent event;
    auto done = false;

    do {
        event = waitForNextEvent(1000); // todo: How long to wait for long exposures?

        if (GP_EVENT_FILE_ADDED == event.event) {
            // Download the file
            CameraFile* file = nullptr;
            gp_file_new(&file);
            // Unique pointer will free memory on exit
            auto filePtr = CameraFilePtr(file, gp_file_free);

            auto ret = gp_camera_file_get(m_camera.get(), event.folderName.toLatin1(), event.fileName.toLatin1(),
                                          GP_FILE_TYPE_NORMAL, file, m_context);

            if (ret < GP_OK) {
                qWarning() << "GPhoto: Failed to get file from camera:" << ret;
                emit imageCaptureError(id, QCameraImageCapture::ResourceError, tr("Failed to download file from camera"));
            } else {
                const char* data = nullptr;
                unsigned long int size = 0;

                ret = gp_file_get_data_and_size(file, &data, &size);
                if (ret < GP_OK) {
                    qWarning() << "GPhoto: Failed to get file data and size from camera:" << ret;
                    emit imageCaptureError(id, QCameraImageCapture::ResourceError, tr("Failed to download file from camera"));
                } else {
                    auto format = QFileInfo(event.fileName).suffix();

                    if (fileName.isEmpty()) {
                        // no proposal file name
                        emit imageCaptured(id, QByteArray(data, int(size)), format, fileName);
                    } else {
                        if (QFileInfo(fileName).suffix() == format) {
                            // extension matches, so use proposed name:
                            emit imageCaptured(id, QByteArray(data, int(size)), format, fileName);
                        } else {
                            // other extension, so use empty name
                            emit imageCaptured(id, QByteArray(data, int(size)), format, QString());
                        }
                    }
                }
            }
        } else if (GP_EVENT_CAPTURE_COMPLETE == event.event) {
            done = true;
        }
    } while (!done);

    setMirrorPosition(MirrorPosition::Up);
}

QVariant GPhotoCamera::parameter(const QString &name)
{
    CameraWidget *root = nullptr;
    auto ret = gp_camera_get_config(m_camera.get(), &root, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get root option from gphoto while getting parameter" << qPrintable(name);
        return QVariant();
    }

    CameraWidget *option = nullptr;
    ret = gp_widget_get_child_by_name(root, qPrintable(name), &option);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get config widget from gphoto";
        return QVariant();
    }

    // Unique pointer will free memory on exit
    auto optionPtr = CameraWidgetPtr(option, gp_widget_free);

    CameraWidgetType type;
    ret = gp_widget_get_type(option, &type);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get config widget type from gphoto";
        return QVariant();
    }

    if (type == GP_WIDGET_RADIO) {
        char *value;
        ret = gp_widget_get_value(option, &value);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: Unable to get value for option" << qPrintable(name) << "from gphoto";
            return QVariant();
        }
        return QString::fromLocal8Bit(value);
    }

    if (type == GP_WIDGET_TOGGLE) {
        auto value = 0;
        ret = gp_widget_get_value(option, &value);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: Unable to get value for option" << qPrintable(name) << "from gphoto";
            return QVariant();
        }
        return value != 0;
    }

    qWarning() << "GPhoto: Options of type" << type << "are currently not supported";
    return QVariant();
}

bool GPhotoCamera::setParameter(const QString &name, const QVariant &value)
{
    CameraWidget *root = nullptr;
    auto ret = gp_camera_get_config(m_camera.get(), &root, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get root option from gphoto while setting parameter" << qPrintable(name);
        return false;
    }

    // Get widget pointer
    CameraWidget *option = nullptr;
    ret = gp_widget_get_child_by_name(root, qPrintable(name), &option);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get option" << qPrintable(name) << "from gphoto";
        return false;
    }

    // Unique pointer will free memory on exit
    auto optionPtr = CameraWidgetPtr(option, gp_widget_free);

    // Get option type
    CameraWidgetType type;
    ret = gp_widget_get_type(option, &type);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get option type from gphoto";
        return false;
    }

    if (type == GP_WIDGET_RADIO) {
        if (value.type() == QVariant::String) {
            // String, need no conversion
            ret = gp_widget_set_value(option, qPrintable(value.toString()));

            if (ret < GP_OK) {
                qWarning() << "GPhoto: Failed to set value" << value << "to" << name << "option:" << ret;
                return false;
            }

            ret = gp_camera_set_config(m_camera.get(), root, m_context);

            if (ret < GP_OK) {
                qWarning() << "GPhoto: Failed to set config to camera";
                return false;
            }

            waitForOperationCompleted();
            return true;
        }

        if (value.type() == QVariant::Double) {
            // Trying to find nearest possible value (with the distance of 0.1) and set it to property
            auto v = value.toDouble();

            auto count = gp_widget_count_choices(option);
            for (auto i = 0; i < count; ++i) {
                const char *choice = nullptr;
                gp_widget_get_choice(option, i, &choice);

                // We use a workaround for flawed russian i18n of gphoto2 strings
                auto ok = false;
                auto choiceValue = QString::fromLocal8Bit(choice).replace(',', '.').toDouble(&ok);
                if (!ok) {
                    qDebug() << "GPhoto: Failed to convert value" << choice << "to double";
                    continue;
                }

                if (qAbs(choiceValue - v) < 0.1) {
                    ret = gp_widget_set_value(option, choice);
                    if (ret < GP_OK) {
                        qWarning() << "GPhoto: Failed to set value" << choice << "to" << name << "option:" << ret;
                        return false;
                    }

                    ret = gp_camera_set_config(m_camera.get(), root, m_context);
                    if (ret < GP_OK) {
                        qWarning() << "GPhoto: Failed to set config to camera";
                        return false;
                    }

                    waitForOperationCompleted();
                    return true;
                }
            }

            qWarning() << "GPhoto: Can't find value matching to" << v << "for option" << name;
            return false;
        }

        if (value.type() == QVariant::Int) {
            // Little hacks for 'ISO' option: if the value is -1, we pick the first non-integer value
            // we found and set it as a parameter
            auto v = value.toInt();


            auto count = gp_widget_count_choices(option);
            for (auto i = 0; i < count; ++i) {
                const char *choice = nullptr;
                gp_widget_get_choice(option, i, &choice);

                auto ok = false;
                auto choiceValue = QString::fromLocal8Bit(choice).toInt(&ok);

                if ((ok && choiceValue == v) || (!ok && v == -1)) {
                    ret = gp_widget_set_value(option, choice);
                    if (ret < GP_OK) {
                        qWarning() << "GPhoto: Failed to set value" << choice << "to" << name << "option:" << ret;
                        return false;
                    }

                    ret = gp_camera_set_config(m_camera.get(), root, m_context);
                    if (ret < GP_OK) {
                        qWarning() << "GPhoto: Failed to set config to camera";
                        return false;
                    }

                    waitForOperationCompleted();
                    return true;
                }
            }

            qWarning() << "GPhoto: Can't find value matching to" << v << "for option" << name;
            return false;
        }

        qWarning() << "GPhoto: Failed to set value" << value << "to" << name << "option. Type" << value.type()
                   << "is not supported";
        return false;
    }

    if (type == GP_WIDGET_TOGGLE) {
        auto v = 0;
        if (value.canConvert<int>()) {
            v = value.toInt();
        } else {
            qWarning() << "GPhoto: Failed to set value" << value << "to" << name << "option. Type" << value.type()
                       << "is not supported";
            return false;
        }

        ret = gp_widget_set_value(option, &v);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: Failed to set value" << v << "to" << name << "option:" << ret;
            return false;
        }

        ret = gp_camera_set_config(m_camera.get(), root, m_context);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: Failed to set config to camera";
            return false;
        }

        waitForOperationCompleted();
        return true;
    }

    qWarning() << "GPhoto: Options of type" << type << "are currently not supported";
    return false;
}

QVariantList GPhotoCamera::parameterValues(const QString &name, QMetaType::Type valueType)
{
    CameraWidget *root = nullptr;
    auto ret = gp_camera_get_config(m_camera.get(), &root, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get root option from gphoto while setting parameter" << qPrintable(name);
        return {};
    }

    // Get widget pointer
    CameraWidget *option = nullptr;
    ret = gp_widget_get_child_by_name(root, qPrintable(name), &option);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get option" << qPrintable(name) << "from gphoto";
        return {};
    }

    // Unique pointer will free memory on exit
    auto optionPtr = CameraWidgetPtr(option, gp_widget_free);

    // Get option type
    CameraWidgetType type;
    ret = gp_widget_get_type(option, &type);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get option type from gphoto";
        return {};
    }

    QVariantList values;

    if (GP_WIDGET_RANGE == type) {
        auto min = 0.0F;
        auto max = 0.0F;
        auto step = 0.0F;

        ret = gp_widget_get_range(option, &min, &max, &step);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: Unable to get widget range from gphoto";
            return {};
        }

        values.append(min);

        while (min < max)
            values.append(min += step);

        values.append(max);
    } else if (GP_WIDGET_RADIO == type) {
        auto count = gp_widget_count_choices(option);
        for (auto i = 0; i < count; ++i) {
            const char *choice = nullptr;
            gp_widget_get_choice(option, i, &choice);
            const auto &str = QString::fromLocal8Bit(choice);

            auto ok = false;
            QVariant value;

            switch (valueType) {
            case QMetaType::Double:
                // We use a workaround for flawed russian i18n of gphoto2 strings
                value = QString(str).replace(',', '.').toDouble(&ok);
                if (!ok) {
                    qWarning() << "GPhoto: Failed to convert value" << choice << "to double";
                    continue;
                }
                break;
            case QMetaType::Int:
                value = str.toInt(&ok);
                if (!ok) {
                    qWarning() << "GPhoto: Failed to convert value" << choice << "to int";
                    continue;
                }
                break;
            case QMetaType::QString:
                value = str;
                break;
            default:
                qWarning() << "GPhoto: Failed to convert unsupported value" << choice;
                continue;
            }

            values.append(value);
        }
    }

    return values;
}

void GPhotoCamera::capturePreview()
{
    if (m_status != QCamera::ActiveStatus)
        return;

    gp_file_clean(m_file.get());

    auto ret = gp_camera_capture_preview(m_camera.get(), m_file.get(), m_context);
    if (GP_OK == ret) {
        const char *data = nullptr;
        unsigned long int size = 0;
        ret = gp_file_get_data_and_size(m_file.get(), &data, &size);
        if (GP_OK == ret) {
            m_capturingFailCount = 0;
            if (!QThread::currentThread()->isInterruptionRequested()) {
                auto image = QImage::fromData(QByteArray(data, int(size)));
                emit previewCaptured(image);
            }
            return;
        }
    }

    qWarning() << "GPhoto: Failed retrieving preview" << ret;
    ++m_capturingFailCount;

    if (capturingFailLimit < m_capturingFailCount) {
        qWarning() << "GPhoto: Closing camera because of capturing fail";
        emit error(QCamera::CameraError, tr("Unable to capture frame"));
        closeCamera();
    }
}

void GPhotoCamera::openCamera()
{
    // Camera is already open
    if (m_camera)
        return;

    setStatus(QCamera::LoadingStatus);

    // Create camera object
    Camera *camera;
    auto ret = gp_camera_new(&camera);
    if (ret != GP_OK) {
        openCameraErrorHandle(QLatin1String("Unable to open camera"));
        return;
    }

    auto cameraPtr = CameraPtr(camera, gp_camera_free);

    ret = gp_camera_set_abilities(camera, m_abilities);
    if (ret < GP_OK) {
        openCameraErrorHandle(QLatin1String("Unable to set abilities for camera"));
        return;
    }

    ret = gp_camera_set_port_info(camera, m_portInfo);
    if (ret < GP_OK) {
        openCameraErrorHandle(QLatin1String("Unable to set port info for camera"));
        return;
    }

    CameraFile *file;
    gp_file_new(&file);
    m_file.reset(file);
    m_camera = std::move(cameraPtr);
    m_capturingFailCount = 0;

    setStatus(QCamera::LoadedStatus);
}

void GPhotoCamera::closeCamera()
{
    // Camera is already closed
    if (!m_camera)
        return;

    if (QCamera::ActiveStatus == m_status)
        stopViewFinder();

    setStatus(QCamera::UnloadingStatus);

    gp_file_clean(m_file.get());
    m_file.reset();

    gp_camera_exit(m_camera.get(), m_context);
    m_camera.reset();

    setStatus(QCamera::UnloadedStatus);
}

void GPhotoCamera::startViewFinder()
{
    openCamera();

    if (QCamera::ActiveStatus == m_status)
        return;

    setStatus(QCamera::StartingStatus);
    setMirrorPosition(MirrorPosition::Up);
    setStatus(QCamera::ActiveStatus);

    capturePreview();
}

void GPhotoCamera::stopViewFinder()
{
    if (m_status == QCamera::LoadedStatus)
        return;

    setStatus(QCamera::StoppingStatus);
    setMirrorPosition(MirrorPosition::Down);
    setStatus(QCamera::LoadedStatus);
}

void GPhotoCamera::setMirrorPosition(MirrorPosition pos)
{
    if (parameter(QLatin1String(viewfinderParameter)).isValid()) {
        auto up = (MirrorPosition::Up == pos);
        if (!setParameter(QLatin1String(viewfinderParameter), up))
            qWarning() << "GPhoto: Failed to flap" << (up ? "up" : "down") << "camera mirror";
    }
}

bool GPhotoCamera::isReadyForCapture() const
{
    if (m_captureMode & QCamera::CaptureStillImage)
        return (QCamera::ActiveStatus == m_status || QCamera::LoadedStatus == m_status);

    return false;
}

void GPhotoCamera::openCameraErrorHandle(const QString &errorText)
{
    qWarning() << "GPhoto:" << qPrintable(errorText);
    setStatus(QCamera::UnavailableStatus);
    emit error(QCamera::CameraError, tr("Unable to open camera"));
}

void GPhotoCamera::logOption(const char *name)
{
    CameraWidget *root;
    auto ret = gp_camera_get_config(m_camera.get(), &root, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: Unable to get root option from gphoto";
        return;
    }

    CameraWidget *option;
    ret = gp_widget_get_child_by_name(root, name, &option);
    if (ret < GP_OK)
        qWarning() << "GPhoto: Unable to get config widget from gphoto";

    // Unique pointer will free memory on exit
    auto optionPtr = CameraWidgetPtr(option, gp_widget_free);

    CameraWidgetType type;
    ret = gp_widget_get_type(option, &type);
    if (ret < GP_OK)
        qWarning() << "GPhoto: Unable to get config widget type from gphoto";

    char *value;
    ret = gp_widget_get_value(option, &value);
    if (ret < GP_OK)
        qWarning() << "GPhoto: Unable to get widget value from gphoto";

    qDebug() << "GPhoto: Option" << type << name << value;
    if (type == GP_WIDGET_RADIO) {
        auto count = gp_widget_count_choices(option);
        qDebug() << "GPhoto: Choices count:" << count;

        for (auto i = 0; i < count; ++i) {
            const char *choice = nullptr;
            gp_widget_get_choice(option, i, &choice);
            qDebug() << "  value:" << choice;
        }
    }
}

void GPhotoCamera::waitForOperationCompleted()
{
    CameraEventType type;
    auto ret = GP_OK;
    do {
        auto dataPtr = VoidPtr(nullptr, free);
        auto data = dataPtr.get();
        ret = gp_camera_wait_for_event(m_camera.get(), waitForEventTimeout, &type, &data, m_context);
    } while ((ret == GP_OK) && (type != GP_EVENT_TIMEOUT) && m_camera);
}


GPhotoCamera::CameraEvent GPhotoCamera::waitForNextEvent(int timeout)
{
    CameraEvent event;
    auto dataPtr = VoidPtr(nullptr, free);
    auto data = dataPtr.get();
    CameraEventType eventType = GP_EVENT_UNKNOWN;

    auto ret = gp_camera_wait_for_event(m_camera.get(), timeout, &eventType, &data, m_context);
    if (ret != GP_OK || GP_EVENT_UNKNOWN == eventType) {
        // according to implementation of gp_camera_wait_for_event();
        // if i dont get OK, no event type & data is updated.
        return event;
    }

    event.event = eventType;

    if (data) {
        // if we have data, it depends on the event type whats inside...
        if (GP_EVENT_FILE_ADDED == eventType || GP_EVENT_FILE_CHANGED == eventType) {
            auto file = reinterpret_cast<CameraFilePath*>(data);
            event.folderName = QString::fromLatin1(file->folder);
            event.fileName = QString::fromLatin1(file->name);
        } else if (GP_EVENT_FOLDER_ADDED == eventType) {
            auto folder= reinterpret_cast<CameraFilePath*>(data);
            event.folderName = QString::fromLatin1(folder->folder);
        }
    }

//    qDebug() << "Got gphoto camera event:" << event.event << event.folderName << event.fileName;

    return event;
}


void GPhotoCamera::setStatus(QCamera::Status status)
{
    if (m_status != status) {
        m_status = status;

        if (QCamera::LoadedStatus == m_status) {
            m_state = QCamera::LoadedState;
            emit stateChanged(m_state);
            emit readyForCaptureChanged(isReadyForCapture());
        } else if (QCamera::ActiveStatus == m_status) {
            m_state = QCamera::ActiveState;
            emit stateChanged(m_state);
            emit readyForCaptureChanged(isReadyForCapture());
        } else if (QCamera::UnloadedStatus == m_status || QCamera::UnavailableStatus == m_status) {
            m_state = QCamera::UnloadedState;
            emit stateChanged(m_state);
            emit readyForCaptureChanged(isReadyForCapture());
        }

        emit statusChanged(status);
    }
}
