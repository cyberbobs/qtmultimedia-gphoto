#ifndef GPHOTOVIDEOPROBECONTROL_H
#define GPHOTOVIDEOPROBECONTROL_H

#include <QMediaVideoProbeControl>

class GPhotoCameraSession;

class GPhotoVideoProbeControl final : public QMediaVideoProbeControl
{
    Q_OBJECT
public:
    explicit GPhotoVideoProbeControl(GPhotoCameraSession *session, QObject *parent = nullptr);
    ~GPhotoVideoProbeControl() = default;

    GPhotoVideoProbeControl(GPhotoVideoProbeControl&&) = delete;
    GPhotoVideoProbeControl& operator=(GPhotoVideoProbeControl&&) = delete;

private:
    Q_DISABLE_COPY(GPhotoVideoProbeControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEOPROBECONTROL_H
