#include "gphotocamerasession.h"
#include "gphotovideoprobecontrol.h"

GPhotoVideoProbeControl::GPhotoVideoProbeControl(GPhotoCameraSession *session, QObject *parent)
    : QMediaVideoProbeControl(parent)
    , m_session(session)
{
    using Session = GPhotoCameraSession;
    using Control = GPhotoVideoProbeControl;

    connect(m_session, &Session::videoFrameProbed, this, &Control::videoFrameProbed);
}
