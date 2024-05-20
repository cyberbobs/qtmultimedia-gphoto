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
    return m_names;
}

QByteArray GPhotoWorker::defaultCameraName()
{
    updateDevices();
    return m_defaultCameraName;
}

void GPhotoWorker::setState(int cameraIndex, QCamera::State state)
{
    if (!isCameraIndexValid(cameraIndex))
        return;

    const auto &path = m_paths.at(cameraIndex);
    if (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
        m_cameras.at(path)->setState(state);
}

void GPhotoWorker::setCaptureMode(int cameraIndex, QCamera::CaptureModes captureMode)
{
    if (!isCameraIndexValid(cameraIndex))
        return;

    const auto &path = m_paths.at(cameraIndex);
    if (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
        m_cameras.at(path)->setCaptureMode(captureMode);
}

void GPhotoWorker::capturePhoto(int cameraIndex, int id, const QString &fileName)
{
    if (!isCameraIndexValid(cameraIndex))
        return;

    const auto &path = m_paths.at(cameraIndex);
    if (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
        m_cameras.at(path)->capturePhoto(id, fileName);
}

QVariant GPhotoWorker::parameter(int cameraIndex, const QString &name)
{
    if (!isCameraIndexValid(cameraIndex))
      return {};

    const auto &path = m_paths.at(cameraIndex);
    return (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
           ? m_cameras.at(path)->parameter(name) : QVariant();
}

bool GPhotoWorker::setParameter(int cameraIndex, const QString &name, const QVariant &value)
{
    if (!isCameraIndexValid(cameraIndex))
      return false;

    const auto &path = m_paths.at(cameraIndex);
    return (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
           ? m_cameras.at(path)->setParameter(name, value) : false;
}

QVariantList GPhotoWorker::parameterValues(int cameraIndex, const QString &name, QMetaType::Type valueType) const
{
    if (!isCameraIndexValid(cameraIndex))
      return {};

    const auto &path = m_paths.at(cameraIndex);
    return (!path.isEmpty() && m_cameras.cend() != m_cameras.find(path))
           ? m_cameras.at(path)->parameterValues(name, valueType) : QVariantList();
}

CameraAbilities GPhotoWorker::getCameraAbilities(int cameraIndex, bool *ok)
{
    CameraAbilities abilities;

    if (!isCameraIndexValid(cameraIndex)) {
        if (ok) *ok = false;
        return abilities;
    }

    const auto &model = m_models.at(cameraIndex);
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

GPPortInfo GPhotoWorker::getPortInfo(int cameraIndex, bool *ok)
{
    GPPortInfo info;
    gp_port_info_new(&info);

    if (!isCameraIndexValid(cameraIndex)) {
        if (ok) *ok = false;
        return info;
    }

    const auto &path = m_paths.at(cameraIndex);

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

bool GPhotoWorker::isCameraIndexValid(int index) const
{
    return (0 <= index && index < m_paths.size());
}

void GPhotoWorker::updateDevices()
{
    QMutexLocker locker(&m_mutex);

    auto cacheIsExpired = (m_cacheAgeTimer.isValid() && deviceCacheLifetime < m_cacheAgeTimer.elapsed());
    if (!m_paths.isEmpty() && !cacheIsExpired)
        return;

    m_paths.clear();
    m_models.clear();
    m_names.clear();
    m_defaultCameraName.clear();

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
        return;
    }

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
            name.append(QString(QLatin1String(" (%1)")).arg(++nameIndexes[name]).toLatin1());
        else
            nameIndexes.insert(name, 0);

        m_paths.append(path);
        m_models.append(model);
        m_names.append(name);

//        if (m_cameras.cend() == m_cameras.find(path))
//            qDebug() << "GPhoto: found" << qPrintable(name) << "at path" << qPrintable(path);

        setupCamera(path);
    }

    // Delete disconnected cameras
    for (auto it = m_cameras.cbegin(); it != m_cameras.cend();)
        it = !m_paths.contains(it->first) ? m_cameras.erase(it) : std::next(it);

    if (!m_paths.isEmpty()) {
        m_defaultCameraName = m_names.first();
        m_cacheAgeTimer.restart();
    }
}

void GPhotoWorker::setupCamera(const QByteArray &path)
{
    auto cameraIndex = m_paths.indexOf(path);
    if (!isCameraIndexValid(cameraIndex)) {
        qWarning() << "GPhoto: Unable to setup camera with path" << qPrintable(path);
        return;
    }

    auto it = m_cameras.find(path);
    if (m_cameras.cend() != it) {
        it->second->setIndex(cameraIndex);
        return;
    }

    auto ok = false;

    const auto &abilities = getCameraAbilities(cameraIndex, &ok);
    if (!ok) {
        qWarning() << "GPhoto: Unable to get abilities for camera with index" << cameraIndex;
        return;
    }

    const auto &portInfo = getPortInfo(cameraIndex, &ok);
    if (!ok) {
        qWarning() << "GPhoto: Unable to get port info for camera with index" << cameraIndex;
        return;
    }

    auto camera = new GPhotoCamera(m_context.get(), abilities, portInfo, cameraIndex, this);

    using Camera = GPhotoCamera;
    using Worker = GPhotoWorker;
    using namespace std::placeholders;

    connect(camera, &Camera::captureModeChanged, this, &Worker::captureModeChanged);
    connect(camera, &Camera::error, this, &Worker::error);
    connect(camera, &Camera::imageCaptureError, this, &Worker::imageCaptureError);
    connect(camera, &Camera::imageCaptured, this, &Worker::imageCaptured);
    connect(camera, &Camera::previewCaptured, this, &Worker::previewCaptured);
    connect(camera, &Camera::readyForCaptureChanged, this, &Worker::readyForCaptureChanged);
    connect(camera, &Camera::stateChanged, this, &Worker::stateChanged);
    connect(camera, &Camera::statusChanged, this, &Worker::statusChanged);

    m_cameras.emplace(path, camera);
}
