#ifndef GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H
#define GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H

#include <QCameraCaptureDestinationControl>

class GPhotoCameraSession;

class GPhotoCameraCaptureDestinationControl final : public QCameraCaptureDestinationControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraCaptureDestinationControl(GPhotoCameraSession *session, QObject *parent = nullptr);
    ~GPhotoCameraCaptureDestinationControl() = default;

    GPhotoCameraCaptureDestinationControl(GPhotoCameraCaptureDestinationControl&&) = delete;
    GPhotoCameraCaptureDestinationControl& operator=(GPhotoCameraCaptureDestinationControl&&) = delete;

    bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const final;
    QCameraImageCapture::CaptureDestinations captureDestination() const final;
    void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination) final;

private:
    Q_DISABLE_COPY(GPhotoCameraCaptureDestinationControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H
