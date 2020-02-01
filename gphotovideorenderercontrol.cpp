#include "gphotocamerasession.h"
#include "gphotovideorenderercontrol.h"

GPhotoVideoRendererControl::GPhotoVideoRendererControl(GPhotoCameraSession *session, QObject *parent)
    : QVideoRendererControl(parent)
    , m_session(session)
{
}

QAbstractVideoSurface* GPhotoVideoRendererControl::surface() const
{
    return m_session->surface();
}

void GPhotoVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    m_session->setSurface(surface);
}
