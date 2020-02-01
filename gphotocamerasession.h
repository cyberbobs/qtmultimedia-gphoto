#ifndef GPHOTOCAMERASESSION_H
#define GPHOTOCAMERASESSION_H

#include <memory>

#include <QCamera>
#include <QCameraImageCapture>
#include <QObject>
#include <QPointer>

class GPhotoCamera;
class GPhotoController;

class GPhotoCameraSession final : public QObject
{
    Q_OBJECT
public:
    explicit GPhotoCameraSession(std::weak_ptr<GPhotoController> controller, QObject *parent = nullptr);
    ~GPhotoCameraSession() = default;

    GPhotoCameraSession(GPhotoCameraSession&&) = delete;
    GPhotoCameraSession& operator=(GPhotoCameraSession&&) = delete;

    QList<QByteArray> cameraNames() const;
    QByteArray defaultCameraName() const;

    // camera control
    QCamera::State state() const;
    void setState(QCamera::State state);
    QCamera::Status status() const;

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const;
    QCamera::CaptureModes captureMode() const;
    void setCaptureMode(QCamera::CaptureModes captureMode);

    // destination control
    bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const;
    QCameraImageCapture::CaptureDestinations captureDestination() const;
    void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination);

    // capture control
    bool isReadyForCapture() const;
    int capture(const QString &fileName);

    // video renderer control
    QAbstractVideoSurface* surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    // options control
    QVariant parameter(const QString &name) const;
    bool setParameter(const QString &name, const QVariant &value);
    QVariantList parameterValues(const QString &name, QMetaType::Type valueType) const;

    void setCamera(int cameraIndex);

signals:
    // camera control
    void statusChanged(QCamera::Status status);
    void stateChanged(QCamera::State state);
    void error(int errorCode, const QString &errorString);
    void captureModeChanged(QCamera::CaptureModes captureMode);

    // capture destination control
    void captureDestinationChanged(QCameraImageCapture::CaptureDestinations destination);

    // image capture control
    void imageAvailable(int id, const QVideoFrame &buffer);
    void imageCaptured(int id, const QImage &preview);
    void imageCaptureError(int id, int errorCode, const QString &errorString);
    void imageSaved(int id, const QString &fileName);
    void readyForCaptureChanged(bool readyForCapture);

    // video probe control
    void videoFrameProbed(const QVideoFrame &frame);

private slots:
    void onCaptureModeChanged(int cameraIndex, QCamera::CaptureModes captureMode);
    void onError(int cameraIndex, int errorCode, const QString &errorString);
    void onImageCaptureError(int cameraIndex, int id, int errorCode, const QString &errorString);
    void onImageCaptured(int cameraIndex, int id, const QByteArray &imageData,
                         const QString &format, const QString &fileName);
    void onPreviewCaptured(int cameraIndex, const QImage &image);
    void onReadyForCaptureChanged(int cameraIndex, bool readyForCapture);
    void onStateChanged(int cameraIndex, QCamera::State state);
    void onStatusChanged(int cameraIndex, QCamera::Status status);

private:
    Q_DISABLE_COPY(GPhotoCameraSession)

    std::weak_ptr<GPhotoController> m_controller;
    QPointer<QAbstractVideoSurface> m_surface;

    QCamera::CaptureModes m_captureMode = QCamera::CaptureStillImage;
    QCamera::State m_state = QCamera::UnloadedState;
    QCamera::Status m_status = QCamera::UnloadedStatus;

    QCameraImageCapture::CaptureDestinations m_captureDestination = QCameraImageCapture::CaptureToBuffer
                                                                    | QCameraImageCapture::CaptureToFile;

    int m_cameraIndex = -1;
    int m_captureId = 0;
    bool m_readyForCapture = false;
};

#endif // GPHOTOCAMERASESSION_H
