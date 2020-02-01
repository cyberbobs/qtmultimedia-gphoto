#include "gphotocameracontrol.h"
#include "gphotocamerasession.h"

GPhotoCameraControl::GPhotoCameraControl(GPhotoCameraSession *session, QObject *parent)
    : QCameraControl(parent)
    , m_session(session)
{
    using Session = GPhotoCameraSession;
    using Control = GPhotoCameraControl;

    connect(m_session, &Session::captureModeChanged, this, &Control::captureModeChanged);
    connect(m_session, &Session::error, this, &Control::error);
    connect(m_session, &Session::stateChanged, this, &Control::stateChanged);
    connect(m_session, &Session::statusChanged, this, &Control::statusChanged);
}

QCamera::State GPhotoCameraControl::state() const
{
    return m_session->state();
}

void GPhotoCameraControl::setState(QCamera::State state)
{
    return m_session->setState(state);
}

QCamera::Status GPhotoCameraControl::status() const
{
    return m_session->status();
}

bool GPhotoCameraControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return m_session->isCaptureModeSupported(mode);
}

QCamera::CaptureModes GPhotoCameraControl::captureMode() const
{
    return m_session->captureMode();
}

void GPhotoCameraControl::setCaptureMode(QCamera::CaptureModes captureMode)
{
    m_session->setCaptureMode(captureMode);
}

bool GPhotoCameraControl::canChangeProperty(QCameraControl::PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(changeType)
    Q_UNUSED(status)

    return false;
}
