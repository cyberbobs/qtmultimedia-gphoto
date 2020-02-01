#ifndef GPHOTOMEDIASERVICE_H
#define GPHOTOMEDIASERVICE_H

#include <memory>

#include <QMediaService>

class GPhotoController;
class GPhotoCameraSession;

class GPhotoMediaService final : public QMediaService
{
    Q_OBJECT
public:
    explicit GPhotoMediaService(std::weak_ptr<GPhotoController> controller, QObject *parent = nullptr);
    ~GPhotoMediaService();

    GPhotoMediaService(GPhotoMediaService&&) = delete;
    GPhotoMediaService& operator=(GPhotoMediaService&&) = delete;

    QMediaControl* requestControl(const char *name) final;
    void releaseControl(QMediaControl *control) final;

private:
    Q_DISABLE_COPY(GPhotoMediaService)

    std::unique_ptr<GPhotoCameraSession> m_session;
};

#endif // GPHOTOMEDIASERVICE_H
