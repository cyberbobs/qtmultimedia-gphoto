#ifndef GPHOTOCAMERAIMAGECAPTURECONTROL_H
#define GPHOTOCAMERAIMAGECAPTURECONTROL_H

#include <QCameraImageCaptureControl>

class GPhotoCameraSession;

class GPhotoCameraImageCaptureControl final : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraImageCaptureControl(GPhotoCameraSession *session, QObject *parent = nullptr);
    ~GPhotoCameraImageCaptureControl() = default;

    GPhotoCameraImageCaptureControl(GPhotoCameraImageCaptureControl&&) = delete;
    GPhotoCameraImageCaptureControl& operator=(GPhotoCameraImageCaptureControl&&) = delete;

    QCameraImageCapture::DriveMode driveMode() const final;
    void setDriveMode(QCameraImageCapture::DriveMode driveMode) final;

    bool isReadyForCapture() const final;

    int capture(const QString &fileName) final;
    void cancelCapture() final;

private:
    Q_DISABLE_COPY(GPhotoCameraImageCaptureControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOCAMERAIMAGECAPTURECONTROL_H
