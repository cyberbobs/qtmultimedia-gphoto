#include "gphotocameracapturedestinationcontrol.h"
#include "gphotocameracontrol.h"
#include "gphotocamerafocuscontrol.h"
#include "gphotocameraimagecapturecontrol.h"
#include "gphotocameralockcontrol.h"
#include "gphotocamerasession.h"
#include "gphotoexposurecontrol.h"
#include "gphotomediaservice.h"
#include "gphotovideoinputdevicecontrol.h"
#include "gphotovideoprobecontrol.h"
#include "gphotovideorenderercontrol.h"

GPhotoMediaService::GPhotoMediaService(std::weak_ptr<GPhotoController> controller, QObject *parent)
    : QMediaService(parent)
    , m_session(new GPhotoCameraSession(std::move(controller)))
{
}

GPhotoMediaService::~GPhotoMediaService()
{
}

QMediaControl *GPhotoMediaService::requestControl(const char *name)
{
    if (qstrcmp(name, QCameraCaptureDestinationControl_iid) == 0)
        return new GPhotoCameraCaptureDestinationControl(m_session.get(), this);

    if (qstrcmp(name, QCameraControl_iid) == 0)
        return new GPhotoCameraControl(m_session.get(), this);

    if (qstrcmp(name, QCameraFocusControl_iid) == 0)
        return m_session->cameraFocusControl();

    if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return new GPhotoExposureControl(m_session.get(), this);

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return new GPhotoCameraImageCaptureControl(m_session.get(), this);

    if (qstrcmp(name, QCameraLocksControl_iid) == 0)
        return new GPhotoCameraLockControl(m_session.get(), this);

    if (qstrcmp(name, QMediaVideoProbeControl_iid) == 0)
        return new GPhotoVideoProbeControl(m_session.get(), this);

    if (qstrcmp(name, QVideoDeviceSelectorControl_iid) == 0)
        return new GPhotoVideoInputDeviceControl(m_session.get(), this);

    if (qstrcmp(name, QVideoRendererControl_iid) == 0)
        return new GPhotoVideoRendererControl(m_session.get(), this);

    return nullptr;
}

void GPhotoMediaService::releaseControl(QMediaControl *control)
{
    delete control;
}
