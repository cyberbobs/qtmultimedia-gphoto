#include <QDebug>

#include "gphotocontroller.h"
#include "gphotomediaservice.h"
#include "gphotoserviceplugin.h"

QMediaService* GPhotoServicePlugin::create(const QString &key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new GPhotoMediaService(getController());

    qWarning() << "GPhoto service plugin: unsupported key:" << key;
    return nullptr;
}

void GPhotoServicePlugin::release(QMediaService *service)
{
    delete service;
}

QByteArray GPhotoServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (const auto &controller = getController().lock())
            return controller->defaultCameraName();
    }

    return {};
}

QList<QByteArray> GPhotoServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (const auto &controller = getController().lock())
            return controller->cameraNames();
    }

    return QList<QByteArray>();
}

QString GPhotoServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    return (service == Q_MEDIASERVICE_CAMERA) ? device : QString();
}

std::weak_ptr<GPhotoController> GPhotoServicePlugin::getController() const
{
    if (!m_controller) {
        auto controller = std::make_shared<GPhotoController>();
        if (!controller->init()) {
            qWarning() << "GPhoto service plugin: unable to initialize GPhotoController";
            return {};
        }
        m_controller = std::move(controller);
    }

    return m_controller;
}
