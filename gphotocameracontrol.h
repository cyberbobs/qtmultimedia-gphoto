#ifndef GPHOTOCAMERACONTROL_H
#define GPHOTOCAMERACONTROL_H

#include <QCameraControl>

class GPhotoCameraSession;

class GPhotoCameraControl final : public QCameraControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraControl(GPhotoCameraSession *session, QObject *parent = nullptr);
    ~GPhotoCameraControl() = default;

    GPhotoCameraControl(GPhotoCameraControl&&) = delete;
    GPhotoCameraControl& operator=(GPhotoCameraControl&&) = delete;

    QCamera::State state() const final;
    void setState(QCamera::State state) final;

    QCamera::Status status() const final;

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const final;
    QCamera::CaptureModes captureMode() const final;
    void setCaptureMode(QCamera::CaptureModes captureMode) final;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const final;

private:
    Q_DISABLE_COPY(GPhotoCameraControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOCAMERACONTROL_H
