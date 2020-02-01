#include "gphotocameraimagecapturecontrol.h"
#include "gphotocamerasession.h"

GPhotoCameraImageCaptureControl::GPhotoCameraImageCaptureControl(GPhotoCameraSession *session, QObject *parent)
    : QCameraImageCaptureControl(parent)
    , m_session(session)
{
    using Session = GPhotoCameraSession;
    using Control = GPhotoCameraImageCaptureControl;

    connect(m_session, &Session::imageAvailable, this, &Control::imageAvailable);
    connect(m_session, &Session::imageCaptured, this, &Control::imageCaptured);
    connect(m_session, &Session::imageCaptureError, this, &Control::error);
    connect(m_session, &Session::imageSaved, this, &Control::imageSaved);
    connect(m_session, &Session::readyForCaptureChanged, this, &Control::readyForCaptureChanged);
}

QCameraImageCapture::DriveMode GPhotoCameraImageCaptureControl::driveMode() const
{
    return QCameraImageCapture::SingleImageCapture;
}

void GPhotoCameraImageCaptureControl::setDriveMode(QCameraImageCapture::DriveMode /*driveMode*/)
{
}

bool GPhotoCameraImageCaptureControl::isReadyForCapture() const
{
    return m_session->isReadyForCapture();
}

int GPhotoCameraImageCaptureControl::capture(const QString &fileName)
{
    return m_session->capture(fileName);
}

void GPhotoCameraImageCaptureControl::cancelCapture()
{
}
