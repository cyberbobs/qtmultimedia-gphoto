#ifndef GPHOTOVIDEOINPUTDEVICECONTROL_H
#define GPHOTOVIDEOINPUTDEVICECONTROL_H

#include <QVideoDeviceSelectorControl>

class GPhotoCameraSession;

class GPhotoVideoInputDeviceControl final : public QVideoDeviceSelectorControl
{
    Q_OBJECT
public:
    GPhotoVideoInputDeviceControl(GPhotoCameraSession *session, QObject *parent = nullptr);
    ~GPhotoVideoInputDeviceControl() = default;

    GPhotoVideoInputDeviceControl(GPhotoVideoInputDeviceControl&&) = delete;
    GPhotoVideoInputDeviceControl& operator=(GPhotoVideoInputDeviceControl&&) = delete;

    int deviceCount() const final;

    QString deviceName(int index) const final;
    QString deviceDescription(int index) const final;

    int defaultDevice() const final;
    int selectedDevice() const final;

public slots:
    void setSelectedDevice(int index) final;

private:
    Q_DISABLE_COPY(GPhotoVideoInputDeviceControl)

    int m_selectedDevice = -1;
    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEOINPUTDEVICECONTROL_H
