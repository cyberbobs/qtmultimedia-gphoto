#ifndef GPHOTOCAMERA_H
#define GPHOTOCAMERA_H

#include <memory>

#include <QCamera>
#include <QObject>

#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-port-info-list.h>

using CameraFilePtr = std::unique_ptr<CameraFile, int (*)(CameraFile*)>;
using CameraPtr = std::unique_ptr<Camera, int (*)(Camera*)>;

class GPhotoCamera final : public QObject
{
    Q_OBJECT
public:

    /** Data forming a camera event from libGPhoto
     * packed into a nicer C++ object.
     *
     * This struct is returned from waitForNextEvent().
     */
    struct CameraEvent {
        /// What happened
        CameraEventType event{GP_EVENT_UNKNOWN};
        /// For some events we get a folder / file info
        QString folderName;
        /// For some events we get a folder / file info
        QString fileName;
    };

    enum class MirrorPosition {
        Up,
        Down
    };

    GPhotoCamera(GPContext *context, const CameraAbilities &abilities,
                 const GPPortInfo &portInfo, int index, QObject *parent = nullptr);
    ~GPhotoCamera();

    GPhotoCamera(GPhotoCamera&&) = delete;
    GPhotoCamera&operator=(GPhotoCamera&&) = delete;

    void setIndex(int index);
    void setState(QCamera::State state);
    void setCaptureMode(QCamera::CaptureModes captureMode);
    void capturePhoto(int id, const QString &fileName);

    QVariant parameter(const QString &name);
    bool setParameter(const QString &name, const QVariant &value);
    QVariantList parameterValues(const QString &name, QMetaType::Type valueType);

signals:
    void captureModeChanged(int index, QCamera::CaptureModes captureMode);
    void error(int index, int errorCode, const QString &errorString);
    void imageCaptured(int index, int id, const QByteArray &imageData, const QString &format, const QString &fileName);
    void imageCaptureError(int index, int id, int errorCode, const QString &errorString);
    void previewCaptured(int index, const QImage &image);
    void readyForCaptureChanged(int index, bool readyForCapture);
    void stateChanged(int index, QCamera::State state);
    void statusChanged(int index, QCamera::Status status);

private slots:
    void capturePreview();

private:
    Q_DISABLE_COPY(GPhotoCamera)

    void openCamera();
    void closeCamera();
    void startViewFinder();
    void stopViewFinder();
    void setMirrorPosition(MirrorPosition pos);
    bool isReadyForCapture() const;
    void logOption(const char *name);
    void openCameraErrorHandle(const QString &errorText);
    void setStatus(QCamera::Status status);
    void waitForOperationCompleted();

    /** Waits for the next event to arrive and deliver event data.
     *
     * @param wait_msec max time to wait in msecs
     * @return the event which occured.
     */
    CameraEvent waitForNextEvent(int timeout);


    GPContext *const m_context;
    CameraAbilities m_abilities;
    GPPortInfo m_portInfo;
    CameraPtr m_camera;
    CameraFilePtr m_file;
    QCamera::State m_state = QCamera::UnloadedState;
    QCamera::Status m_status = QCamera::UnloadedStatus;
    QCamera::CaptureModes m_captureMode = QCamera::CaptureStillImage;
    int m_capturingFailCount = 0;
    int m_index = 0;
};

#endif // GPHOTOCAMERA_H
