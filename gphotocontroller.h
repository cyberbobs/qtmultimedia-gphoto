#ifndef GPHOTOCONTROLLER_H
#define GPHOTOCONTROLLER_H

#include <memory>

#include <QCamera>
#include <QObject>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

class GPhotoCamera;
class GPhotoWorker;

class GPhotoController final : public QObject
{
    Q_OBJECT
public:
    explicit GPhotoController(QObject *parent = nullptr);
    ~GPhotoController();

    GPhotoController(GPhotoController&&) = delete;
    GPhotoController& operator=(GPhotoController&&) = delete;

    bool init();

    QList<QByteArray> cameraNames() const;
    QByteArray defaultCameraName() const;

    void initCamera(int cameraIndex) const;
    void capturePhoto(int cameraIndex, int id, const QString &fileName) const;

    QCamera::CaptureModes captureMode(int cameraIndex) const;
    void setCaptureMode(int cameraIndex, QCamera::CaptureModes captureMode);

    QCamera::State state(int cameraIndex) const;
    void setState(int cameraIndex, QCamera::State state) const;

    QCamera::Status status(int cameraIndex) const;

    QVariant parameter(int cameraIndex, const QString &name) const;
    bool setParameter(int cameraIndex, const QString &name, const QVariant &value);
    QVariantList parameterValues(int cameraIndex, const QString &name, QMetaType::Type valueType) const;

signals:
    void captureModeChanged(int cameraIndex, QCamera::CaptureModes);
    void error(int cameraIndex, int errorCode, const QString &errorString);
    void imageCaptured(int cameraIndex, int id, const QByteArray &imageData,
                       const QString &format, const QString &fileName);
    void imageCaptureError(int cameraIndex, int id, int errorCode, const QString &errorString);
    void previewCaptured(int cameraIndex, const QImage &image);
    void readyForCaptureChanged(int cameraIndex, bool);
    void stateChanged(int cameraIndex, QCamera::State);
    void statusChanged(int cameraIndex, QCamera::Status);

private slots:
    void onCaptureModeChanged(int cameraIndex, QCamera::CaptureModes captureMode);
    void onStateChanged(int cameraIndex, QCamera::State state);
    void onStatusChanged(int cameraIndex, QCamera::Status status);

private:
    Q_DISABLE_COPY(GPhotoController)

    std::unique_ptr<QThread> m_workerThread;
    std::unique_ptr<GPhotoWorker> m_worker;

    QMap<int, QCamera::CaptureModes> m_captureModes;
    QMap<int, QCamera::State> m_states;
    QMap<int, QCamera::Status> m_statuses;
    QMap<int, bool> m_capturings;
};

#endif // GPHOTOCONTROLLER_H
