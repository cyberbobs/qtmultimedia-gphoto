#ifndef GPHOTOSERVICEPLUGIN_H
#define GPHOTOSERVICEPLUGIN_H

#include <memory>

#include <QMediaServiceDefaultDeviceInterface>
#include <QMediaServiceProviderPlugin>
#include <QMediaServiceSupportedDevicesInterface>

class GPhotoController;

class GPhotoServicePlugin final
    : public QMediaServiceProviderPlugin
    , public QMediaServiceSupportedDevicesInterface
    , public QMediaServiceDefaultDeviceInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedDevicesInterface)
    Q_INTERFACES(QMediaServiceDefaultDeviceInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "gphoto.json")
public:
    GPhotoServicePlugin() = default;
    ~GPhotoServicePlugin() = default;

    GPhotoServicePlugin(GPhotoServicePlugin&&) = delete;
    GPhotoServicePlugin& operator=(GPhotoServicePlugin&&) = delete;

    QMediaService* create(const QString &key) final;
    void release(QMediaService *service) final;

    QByteArray defaultDevice(const QByteArray &service) const final;
    QList<QByteArray> devices(const QByteArray &service) const final;
    QString deviceDescription(const QByteArray &service, const QByteArray &device) final;

private:
    Q_DISABLE_COPY(GPhotoServicePlugin)

    std::weak_ptr<GPhotoController> getController() const;

    mutable std::shared_ptr<GPhotoController> m_controller;
};

#endif // GPHOTOSERVICEPLUGIN_H
