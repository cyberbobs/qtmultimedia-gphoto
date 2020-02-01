#include <QAbstractVideoSurface>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QVideoSurfaceFormat>

#include "gphotocamera.h"
#include "gphotocamerasession.h"
#include "gphotocontroller.h"

namespace {
    constexpr auto maxDownscaleSteps = 8;
    constexpr auto maxFileIndex = 9999;
    constexpr auto maxPreviewWidth = 800;
}

GPhotoCameraSession::GPhotoCameraSession(std::weak_ptr<GPhotoController> controller, QObject *parent)
    : QObject(parent)
    , m_controller(std::move(controller))
{
    if (const auto &controller = m_controller.lock()) {
        using Controller = GPhotoController;
        using Session = GPhotoCameraSession;

        connect(controller.get(), &Controller::captureModeChanged, this, &Session::onCaptureModeChanged);
        connect(controller.get(), &Controller::error, this, &Session::onError);
        connect(controller.get(), &Controller::imageCaptureError, this, &Session::onImageCaptureError);
        connect(controller.get(), &Controller::imageCaptured, this, &Session::onImageCaptured);
        connect(controller.get(), &Controller::previewCaptured, this, &Session::onPreviewCaptured);
        connect(controller.get(), &Controller::readyForCaptureChanged, this, &Session::onReadyForCaptureChanged);
        connect(controller.get(), &Controller::stateChanged, this, &Session::onStateChanged);
        connect(controller.get(), &Controller::statusChanged, this, &Session::onStatusChanged);
    }
}

QList<QByteArray> GPhotoCameraSession::cameraNames() const
{
    if (const auto &controller = m_controller.lock())
        return controller->cameraNames();

    return {};
}

QByteArray GPhotoCameraSession::defaultCameraName() const
{
    if (const auto &controller = m_controller.lock())
        return controller->defaultCameraName();

    return {};
}

QCamera::State GPhotoCameraSession::state() const
{
    return m_state;
}

void GPhotoCameraSession::setState(QCamera::State state)
{
    if (const auto &controller = m_controller.lock())
        controller->setState(m_cameraIndex, state);
}

QCamera::Status GPhotoCameraSession::status() const
{
    return m_status;
}

bool GPhotoCameraSession::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return (QCamera::CaptureViewfinder == mode || QCamera::CaptureStillImage == mode);
}

QCamera::CaptureModes GPhotoCameraSession::captureMode() const
{
    return m_captureMode;
}

void GPhotoCameraSession::setCaptureMode(QCamera::CaptureModes captureMode)
{
    if (const auto &controller = m_controller.lock())
        controller->setCaptureMode(m_cameraIndex, captureMode);
}

bool GPhotoCameraSession::isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const
{
    Q_UNUSED(destination)

    return true;
}


QCameraImageCapture::CaptureDestinations GPhotoCameraSession::captureDestination() const
{
    return m_captureDestination;
}


void GPhotoCameraSession::setCaptureDestination(QCameraImageCapture::CaptureDestinations destination)
{
    if (m_captureDestination != destination) {
        m_captureDestination = destination;
        emit captureDestinationChanged(destination);
    }
}

bool GPhotoCameraSession::isReadyForCapture() const
{
    return m_readyForCapture;
}

int GPhotoCameraSession::capture(const QString &fileName)
{
    ++m_captureId;

    if (const auto &controller = m_controller.lock())
        controller->capturePhoto(m_cameraIndex, m_captureId, fileName);

    return m_captureId;
}

QAbstractVideoSurface* GPhotoCameraSession::surface() const
{
    return m_surface;
}

void GPhotoCameraSession::setSurface(QAbstractVideoSurface *surface)
{
    if (m_surface != surface)
        m_surface = surface;
}

QVariant GPhotoCameraSession::parameter(const QString &name) const
{
    if (const auto &controller = m_controller.lock())
        return controller->parameter(m_cameraIndex, name);

    return {};
}

bool GPhotoCameraSession::setParameter(const QString &name, const QVariant &value)
{
    if (const auto &controller = m_controller.lock())
        return controller->setParameter(m_cameraIndex, name, value);

    return false;
}

QVariantList GPhotoCameraSession::parameterValues(const QString &name, QMetaType::Type valueType) const
{
    if (const auto &controller = m_controller.lock())
        return controller->parameterValues(m_cameraIndex, name, valueType);

    return {};
}

void GPhotoCameraSession::setCamera(int cameraIndex)
{
    if (m_cameraIndex != cameraIndex) {
        m_cameraIndex = cameraIndex;
        if (const auto &controller = m_controller.lock()) {
            onCaptureModeChanged(cameraIndex, controller->captureMode(m_cameraIndex));
            onStateChanged(cameraIndex, controller->state(m_cameraIndex));
            onStatusChanged(cameraIndex, controller->status(m_cameraIndex));
            controller->initCamera(cameraIndex);
        }
    }
}

void GPhotoCameraSession::onCaptureModeChanged(int cameraIndex, QCamera::CaptureModes captureMode)
{
    if (m_cameraIndex == cameraIndex && m_captureMode != captureMode) {
        m_captureMode = captureMode;
        emit captureModeChanged(captureMode);
    }
}

void GPhotoCameraSession::onError(int cameraIndex, int errorCode, const QString &errorString)
{
    if (m_cameraIndex == cameraIndex)
        emit error(errorCode, errorString);
}

void GPhotoCameraSession::onImageCaptureError(int cameraIndex, int id, int errorCode, const QString &errorString)
{
    if (m_cameraIndex == cameraIndex)
        emit imageCaptureError(id, errorCode, errorString);
}

void GPhotoCameraSession::onImageCaptured(int cameraIndex, int id, const QByteArray &imageData,
                                          const QString &format, const QString &fileName)
{
    if (m_cameraIndex != cameraIndex)
        return;

    if (format.startsWith(QLatin1String("jp"), Qt::CaseInsensitive)) {
        auto image = QImage::fromData(imageData);
        if (!image.isNull()) {
            auto previewSize = image.size();
            auto downScaleSteps = 0;
            while (previewSize.width() > maxPreviewWidth && downScaleSteps < maxDownscaleSteps) {
                previewSize.rwidth() /= 2;
                previewSize.rheight() /= 2;
                ++downScaleSteps;
            }

            const auto &snapPreview = image.scaled(previewSize);
            emit imageCaptured(id, snapPreview);

            if (m_captureDestination & QCameraImageCapture::CaptureToBuffer) {
                QVideoFrame frame(image);
                emit imageAvailable(id, frame);
            }
        }
    }

    if (m_captureDestination & QCameraImageCapture::CaptureToFile) {
        QString actualFileName(fileName);
        if (actualFileName.isEmpty()) {
            auto dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
            if (dir.isEmpty()) {
                emit imageCaptureError(id, QCameraImageCapture::ResourceError,
                                       tr("Could not determine writable location for saving captured image"));
                return;
            }

            dir += QLatin1String("/DCIM%1.") + format;
            // Trying to find free filename
            for (auto i = 0; i < maxFileIndex; ++i) {
                auto tmpFileName = dir.arg(i, 4, 10, QChar('0'));
                if (!QFile(tmpFileName).exists()) {
                    actualFileName = std::move(tmpFileName);
                    break;
                }
            }

            if (actualFileName.isEmpty()) {
                emit imageCaptureError(id, QCameraImageCapture::ResourceError,
                                       tr("Could not determine writable location for saving captured image"));
                return;
            }
        }

        QFile file(actualFileName);
        if (file.open(QFile::WriteOnly)) {
            if (file.write(imageData)) {
                emit imageSaved(id, actualFileName);
            } else {
                emit imageCaptureError(id, QCameraImageCapture::OutOfSpaceError, file.errorString());
            }
        } else {
            auto errorMessage = tr("Could not open destination file:\n%1").arg(actualFileName);
            emit imageCaptureError(id, QCameraImageCapture::ResourceError, errorMessage);
        }
    }
}

void GPhotoCameraSession::onPreviewCaptured(int cameraIndex, const QImage &image)
{
    if (m_cameraIndex == cameraIndex && QCamera::ActiveState == m_state && m_surface && !image.isNull()) {
        if (m_surface->isActive() && image.size() != m_surface->surfaceFormat().frameSize())
            m_surface->stop();

        if (!m_surface->isActive())
            m_surface->start(QVideoSurfaceFormat(image.size(), QVideoFrame::Format_RGB32));

        QVideoFrame frame(image);
        m_surface->present(frame);
        emit videoFrameProbed(frame);
    }
}

void GPhotoCameraSession::onReadyForCaptureChanged(int cameraIndex, bool readyForCapture)
{
    if (m_cameraIndex == cameraIndex && m_readyForCapture != readyForCapture) {
        m_readyForCapture = readyForCapture;
        emit readyForCaptureChanged(readyForCapture);
    }
}

void GPhotoCameraSession::onStateChanged(int cameraIndex, QCamera::State state)
{
    if (m_cameraIndex == cameraIndex && m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void GPhotoCameraSession::onStatusChanged(int cameraIndex, QCamera::Status status)
{
    if (m_cameraIndex == cameraIndex && m_status != status) {
        m_status = status;
        emit statusChanged(status);
    }
}
