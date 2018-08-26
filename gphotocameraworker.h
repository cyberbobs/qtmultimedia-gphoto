#ifndef GPHOTOCAMERAWORKER_H
#define GPHOTOCAMERAWORKER_H

#include <QObject>
#include <gphoto2/gphoto2-camera.h>
#include "gphotocamerasession.h"
#include "gphotofactory.h"


class GPhotoCameraWorker : public QObject
{
    Q_OBJECT
public:
    GPhotoCameraWorker(const CameraAbilities &abilities, const PortInfo &portInfo, QObject *parent = 0);
    ~GPhotoCameraWorker();

signals:
    void statusChanged(QCamera::Status);
    void error(int error, const QString &errorString);

    void previewCaptured(const QImage&);

    void imageCaptured(int id, const QByteArray &imageData, const QString &fileName);
    void imageCaptureError(int id, int error, const QString &errorString);

public slots:
    void openCamera();
    void closeCamera();
    void stopViewFinder();

    void capturePreview();
    void capturePhoto(int id, const QString &fileName);

    QVariant parameter(const QString &name);
    bool setParameter(const QString &name, const QVariant &value);

private:
    const CameraAbilities m_abilities;
    const PortInfo m_portInfo;
    GPContext* m_context;
    Camera *m_camera;
    CameraFile *m_file;
    int m_capturingFailCount;

    QCamera::Status m_status;

    void openCameraErrorHandle(const QString &errorText);
    void logOption(const char* name);
    void waitForOperationCompleted();
};

#endif // GPHOTOCAMERAWORKER_H
