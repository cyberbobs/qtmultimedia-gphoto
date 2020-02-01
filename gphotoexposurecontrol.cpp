#include "gphotocamerasession.h"
#include "gphotoexposurecontrol.h"

namespace {
    constexpr auto apertureParameter = "aperture";
    constexpr auto exposureCompensationParameter = "exposurecompensation";
    constexpr auto isoParameter = "iso";
    constexpr auto shutterSpeedParameter = "shutterspeed";
}

GPhotoExposureControl::GPhotoExposureControl(GPhotoCameraSession *session, QObject *parent)
    : QCameraExposureControl(parent)
    , m_session(session)
{
    m_state = m_session->state();

    using Session = GPhotoCameraSession;
    using Control = GPhotoExposureControl;

    connect(m_session, &Session::stateChanged, this, &Control::stateChanged);
}

QVariant GPhotoExposureControl::actualValue(QCameraExposureControl::ExposureParameter parameter) const
{
    if (QCamera::UnloadedState == m_state)
        return QVariant();

    if (Aperture == parameter) {
        const auto &value = m_session->parameter(QLatin1String(apertureParameter));
        auto ok = false;
        // We use a workaround for flawed russian i18n of gphoto2 strings
        const auto &aperture = value.toString().replace(',', '.').toDouble(&ok);
        return ok ? QVariant(aperture) : QVariant();
    }

    if (ExposureCompensation == parameter) {
        const auto &value = m_session->parameter(QLatin1String(exposureCompensationParameter));
        auto ok = false;
        // We use a workaround for flawed russian i18n of gphoto2 strings
        const auto &exposure = value.toString().replace(',', '.').toDouble(&ok);
        return ok ? QVariant(exposure) : QVariant();
    }

    if (ISO == parameter) {
        auto ok = false;
        const auto &iso = m_session->parameter(QLatin1String(isoParameter)).toInt(&ok);
        // Invalid QVariant for Auto ISO
        return ok ? QVariant(iso) : QVariant();
    }

    if (ShutterSpeed == parameter)
        return convertShutterSpeed(m_session->parameter(QLatin1String(shutterSpeedParameter)).toString());

    return QVariant();
}

bool GPhotoExposureControl::isParameterSupported(QCameraExposureControl::ExposureParameter parameter) const
{
    switch (parameter) {
    case QCameraExposureControl::ISO:
        return true;
    case QCameraExposureControl::Aperture:
        return true;
    case QCameraExposureControl::ShutterSpeed:
        return true;
    case QCameraExposureControl::ExposureCompensation:
        return true;
    case QCameraExposureControl::FlashPower:
        return false;
    case QCameraExposureControl::TorchPower:
        return false;
    case QCameraExposureControl::FlashCompensation:
        return false;
    case QCameraExposureControl::SpotMeteringPoint:
        return false;
    case QCameraExposureControl::ExposureMode:
        return false;
    case QCameraExposureControl::MeteringMode:
        return false;
    case QCameraExposureControl::ExtendedExposureParameter:
        return false;
    }

    return false;
}

QVariant GPhotoExposureControl::requestedValue(QCameraExposureControl::ExposureParameter parameter) const
{
    qDebug("requestedValue %d", parameter);
    return m_requestedValues.value(parameter);
}

bool GPhotoExposureControl::setValue(QCameraExposureControl::ExposureParameter parameter, const QVariant &value)
{
    m_requestedValues[parameter] = value;
    emit requestedValueChanged(parameter);

    // Try to set parameters only on loaded camera
    if (QCamera::UnloadedState == m_state)
        return false;

    if (Aperture == parameter) {
        if (m_session->setParameter(QLatin1String(apertureParameter), value)) {
            emit actualValueChanged(parameter);
            return true;
        }
    } else if (ExposureCompensation == parameter) {
        if (m_session->setParameter(QLatin1String(exposureCompensationParameter), value)) {
            emit actualValueChanged(parameter);
            return true;
        }
    } else if (ISO == parameter) {
        QVariant v = value;
        if (!v.isValid())
            v = -1;
        if (m_session->setParameter(QLatin1String(isoParameter), v)) {
            emit actualValueChanged(parameter);
            return true;
        }
    } else if (ShutterSpeed == parameter) {
        if (QVariant::Double == value.type())  {
            const auto &values = m_session->parameterValues(QLatin1String(shutterSpeedParameter), QMetaType::QString);
            const auto &speeds = convertShutterSpeeds(values, false);
            if (values.size() == speeds.size()) {
                auto speed = value.toDouble();
                const auto &found = std::find_if(speeds.cbegin(), speeds.cend(), [speed] (const QVariant &val)
                {
                    return qFuzzyCompare(speed, val.toDouble());
                });

                if (speeds.cend() != found) {
                    auto index = int(std::distance(speeds.cbegin(), found));
                    if (0 <= index) {
                        const auto &value = values.value(index);
                        if (m_session->setParameter(QLatin1String(shutterSpeedParameter), value)) {
                            emit actualValueChanged(parameter);
                            return true;
                        }
                    }
                }
            }
        }
    } else {
        qWarning() << "Currently unsupported parameter" << parameter << "change requested";
    }

    return false;
}

QVariantList GPhotoExposureControl::supportedParameterRange(QCameraExposureControl::ExposureParameter parameter, bool *continuous) const
{
    if (nullptr != continuous)
        *continuous = false;

    switch (parameter) {
    case Aperture:
        return m_session->parameterValues(QLatin1String(apertureParameter), QMetaType::Double);
    case ExposureCompensation:
        return m_session->parameterValues(QLatin1String(exposureCompensationParameter), QMetaType::Double);
    case ISO:
        return m_session->parameterValues(QLatin1String(isoParameter), QMetaType::Int);
    case ShutterSpeed:
        return convertShutterSpeeds(m_session->parameterValues(QLatin1String(shutterSpeedParameter), QMetaType::QString));
    default:
        return {};
    }
}

void GPhotoExposureControl::stateChanged(QCamera::State state)
{
    if (m_state != state) {
        if (QCamera::UnloadedState == m_state && QCamera::LoadedState == state) {
            m_state = state;

            static const auto &parameter = metaObject()->enumerator(metaObject()->indexOfEnumerator("ExposureParameter"));
            for (auto i = 0; i < parameter.keyCount(); ++i) {
                auto p = ExposureParameter(parameter.value(i));

                if (isParameterSupported(p)) {
                    // Set all parameters requested on start to session object
                    if (m_requestedValues.contains(p))
                        setValue(p, m_requestedValues.value(p));
                    // or just notify frontend that it's allowed to get the parameter values from backend
                    else
                        emit actualValueChanged(p);
                }
            }
        } else {
            m_state = state;
        }
    }
}

QVariant GPhotoExposureControl::convertShutterSpeed(const QVariant &value)
{
    Q_ASSERT(QVariant::String == value.type());

    auto ok = false;
    const auto &str = value.toString();

    if (str.contains('/')) {
        if (1 < str.count('/')) {
            qWarning().noquote() << "Failed to convert value" << value << "to double";
            return {};
        }

        const auto &fraction = str.split('/');

        auto numerator = fraction.first().toInt(&ok);
        if (!ok) {
            qWarning().noquote() << "Failed to convert value" << value << "to double";
            return {};
        }

        auto denominator = fraction.last().toInt(&ok);
        if (!ok) {
            qWarning().noquote() << "Failed to convert value" << value << "to double";
            return {};
        }

        return qreal(numerator) / denominator;
    }

    // We use a workaround for flawed russian i18n of gphoto2 strings
    auto result = QString(str).replace(',', '.').toDouble(&ok);
    // Invalid QVariant for auto shutter speed
    return ok ? QVariant(result) : QVariant();
}

QVariantList GPhotoExposureControl::convertShutterSpeeds(const QVariantList &values, bool removeInvalids)
{
    auto result = QVariantList();

    for (const auto &value : values) {
        const auto &val = convertShutterSpeed(value);
        if (val.isValid() || !removeInvalids)
            result.append(val);
    }

    return result;
}
