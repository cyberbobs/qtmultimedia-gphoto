#ifndef GPHOTOVIDEORENDERERCONTROL_H
#define GPHOTOVIDEORENDERERCONTROL_H

#include <QVideoRendererControl>

class GPhotoCameraSession;

class GPhotoVideoRendererControl final : public QVideoRendererControl
{
    Q_OBJECT
public:
    explicit GPhotoVideoRendererControl(GPhotoCameraSession *session, QObject *parent = nullptr);
    ~GPhotoVideoRendererControl() = default;

    GPhotoVideoRendererControl(GPhotoVideoRendererControl&&) = delete;
    GPhotoVideoRendererControl& operator=(GPhotoVideoRendererControl&&) = delete;

    QAbstractVideoSurface* surface() const final;
    void setSurface(QAbstractVideoSurface *surface) final;

private:
    Q_DISABLE_COPY(GPhotoVideoRendererControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEORENDERERCONTROL_H
