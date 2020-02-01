#include <functional>

#include <QDebug>
#include <QEventLoop>
#include <QThread>

#include <gphoto2/gphoto2-list.h>
#include <gphoto2/gphoto2-port-result.h>

#include "gphotocamera.h"
#include "gphotoworker.h"

namespace {
    constexpr auto deviceCacheLifetime = 1000;
}

using CameraListPtr = std::unique_ptr<CameraList, int (*)(CameraList*)>;

GPhotoWorker::GPhotoWorker()
    : m_context(gp_context_new(), gp_context_unref)
    , m_portInfoList(nullptr, gp_port_info_list_free)
    , m_abilitiesList(nullptr, gp_abilities_list_free)
{
    GPPortInfoList *piList;
    gp_port_info_list_new(&piList);
    m_portInfoList.reset(piList);

    CameraAbilitiesList *caList;
    gp_abilities_list_new(&caList);
    m_abilitiesList.reset(caList);
}

GPhotoWorker::~GPhotoWorker()
{
}

bool GPhotoWorker::init()
{
    Q_ASSERT(m_context);

    auto ret = gp_port_info_list_load(m_portInfoList.get());
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load port info list";
        return false;
    }

    ret = gp_port_info_list_count(m_portInfoList.get());
    if (ret < 1) {
        qWarning() << "GPhoto: port info list is empty";
        return false;
    }

    ret = gp_abilities_list_load(m_abilitiesList.get(), m_context.get());
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load camera abilities list";
        return false;
    }

    ret = gp_abilities_list_count(m_abilitiesList.get());
    if (ret < 1) {
        qWarning() << "GPhoto: camera abilities list is empty";
        return false;
    }

    return true;
}

QList<QByteArray> GPhotoWorker::cameraNames()
{
    updateDevices();
    return m_names.values();
}

QByteArray GPhotoWorker::defaultCameraName()
{
    updateDevices();
    return m_defaultCameraName;
}

void GPhotoWorker::initCamera(int cameraIndex)
{
    const auto &path = m_paths.value(cameraIndex);
    if (path.isEmpty()) {
        qWarning() << "Unable to init camera with index" << cameraIndex;
        return;
    }

    if (m_cameras.cend() != m_cameras.find(path)) {
        // Already initialized
        return;
    }

    auto ok = false;

    const auto &abilities = getCameraAbilities(path, &ok);
    if (!ok) {
        qWarning() << "Unable to get abilities for camera with index" << cameraIndex;
        return;
    }

    const auto &portInfo = getPortInfo(path, &ok);
    if (!ok) {
        qWarning() << "Unable to get port info for camera with index" << cameraIndex;
        return;
    }

    auto camera = new GPhotoCamera(m_context.get(), abilities, portInfo, this);

    using Camera = GPhotoCamera;
    using Worker = GPhotoWorker;
    using namespace std::placeholders;

    connect(camera, &Camera::captureModeChanged, this, std::bind(&Worker::captureModeChanged, this, cameraIndex, _1));
    connect(camera, &Camera::error, this, std::bind(&Worker::error, this, cameraIndex, _1, _2));
    connect(camera, &Camera::imageCaptureError, this, std::bind(&Worker::imageCaptureError, this, cameraIndex, _1, _2, _3));
    connect(camera, &Camera::imageCaptured, this, std::bind(&Worker::imageCaptured, this, cameraIndex, _1, _2, _3, _4));
    connect(camera, &Camera::previewCaptured, this, std::bind(&Worker::previewCaptured, this, cameraIndex, _1));
    connect(camera, &Camera::readyForCaptureChanged, this, std::bind(&Worker::readyForCaptureChanged, this, cameraIndex, _1));
    connect(camera, &Camera::stateChanged, this, std::bind(&Worker::stateChanged, this, cameraIndex, _1));
    connect(camera, &Camera::statusChanged, this, std::bind(&Worker::statusChanged, this, cameraIndex, _1));

    m_cameras.emplace(std::make_pair(path, camera));
}

void GPhotoWorker::setState(int cameraIndex, QCamera::State state)
{
    const auto &path = m_paths.value(cameraIndex);
    if (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
        m_cameras.at(path)->setState(state);
}

void GPhotoWorker::setCaptureMode(int cameraIndex, QCamera::CaptureModes captureMode)
{
    const auto &path = m_paths.value(cameraIndex);
    if (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
        m_cameras.at(path)->setCaptureMode(captureMode);
}

void GPhotoWorker::capturePhoto(int cameraIndex, int id, const QString &fileName)
{
    const auto &path = m_paths.value(cameraIndex);
    if (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
        m_cameras.at(path)->capturePhoto(id, fileName);
}

QVariant GPhotoWorker::parameter(int cameraIndex, const QString &name)
{
    const auto &path = m_paths.value(cameraIndex);
    return (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
           ? m_cameras.at(path)->parameter(name) : QVariant();
}

bool GPhotoWorker::setParameter(int cameraIndex, const QString &name, const QVariant &value)
{
    const auto &path = m_paths.value(cameraIndex);
    return (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
           ? m_cameras.at(path)->setParameter(name, value) : false;
}

QVariantList GPhotoWorker::parameterValues(int cameraIndex, const QString &name, QMetaType::Type valueType) const
{
    const auto &path = m_paths.value(cameraIndex);
    return (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
           ? m_cameras.at(path)->parameterValues(name, valueType) : QVariantList();
}

CameraAbilities GPhotoWorker::getCameraAbilities(const QByteArray &path, bool *ok)
{
    CameraAbilities abilities;

    if (!m_models.contains(path)) {
        if (ok) *ok = false;
        return abilities;
    }

    const auto &model = m_models.value(path);
    auto abilitiesIndex = gp_abilities_list_lookup_model(m_abilitiesList.get(), model.constData());
    if (abilitiesIndex < GP_OK) {
        qWarning() << "GPhoto: unable to find camera abilities";
        if (ok) *ok = false;
        return abilities;
    }

    auto ret = gp_abilities_list_get_abilities(m_abilitiesList.get(), abilitiesIndex, &abilities);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera abilities";
        if (ok) *ok = false;
        return abilities;
    }

    if (ok) *ok = true;
    return abilities;
}

GPPortInfo GPhotoWorker::getPortInfo(const QByteArray &path, bool *ok)
{
    GPPortInfo info;
    gp_port_info_new(&info);

    auto port = gp_port_info_list_lookup_path(m_portInfoList.get(), path.constData());
    if (port < GP_OK) {
        qWarning() << "GPhoto: unable to find camera port";
        if (ok) *ok = false;
        return info;
    }

    auto ret = gp_port_info_list_get_info(m_portInfoList.get(), port, &info);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera port info";
        if (ok) *ok = false;
        return info;
    }

    if (ok) *ok = true;
    return info;
}

void GPhotoWorker::updateDevices()
{
    QMutexLocker locker(&m_mutex);

    if (m_cacheAgeTimer.isValid() && deviceCacheLifetime < m_cacheAgeTimer.elapsed()) {
        m_paths.clear();
        m_names.clear();
        m_defaultCameraName.clear();
    }

    if (!m_paths.isEmpty())
        return;

    CameraList *cameraList;
    gp_list_new(&cameraList);

    // Unique pointer will free memory on exit
    auto cameraListPtr = CameraListPtr(cameraList, gp_list_free);

    auto ret = gp_camera_autodetect(cameraList, m_context.get());
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to detect camera";
        return;
    }

    auto cameraCount = gp_list_count(cameraList);
    if (cameraCount < 1) {
        m_models.clear();
        m_cameras.clear();
        return;
    }

    QMap<QByteArray, QByteArray> prevModels;
    std::swap(m_models, prevModels);
    QMap<QByteArray, int> nameIndexes;
    for (auto i = 0; i < cameraCount; ++i) {
        const char *gpPath = nullptr;
        ret = gp_list_get_value(cameraList, i, &gpPath);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera path";
            continue;
        }

        const char *gpName = nullptr;
        ret = gp_list_get_name(cameraList, i, &gpName);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera name";
            continue;
        }

        auto path = QByteArray(gpPath);
        auto model = QByteArray(gpName);
        auto name = model;
        if (nameIndexes.contains(name))
            name.append(QString(QLatin1String(" (%1)")).arg(++nameIndexes[name]));
        else
            nameIndexes.insert(name, 0);

//        qDebug() << "GPhoto: found" << qPrintable(name) << "at path" << qPrintable(path);

        m_paths.append(path);
        m_models.insert(path, model);
        m_names.insert(path, name);

        if (prevModels.contains(path)) {
          const auto &prevModel = prevModels.value(path);
          if (prevModel != model)
              m_cameras.erase(m_cameras.find(path));
        }
    }

    if (!m_paths.isEmpty()) {
        m_defaultCameraName = m_names.value(m_paths.first());
        m_cacheAgeTimer.restart();
    }
}
