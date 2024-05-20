#include <QAbstractVideoSurface>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QStandardPaths>
#include <QThread>
#include <QVideoSurfaceFormat>

#include "gphotocamera.h"
#include "gphotocontroller.h"
#include "gphotoworker.h"

namespace {
    constexpr auto workerThreadTimeout = 30000;
}

GPhotoController::GPhotoController(QObject *parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
    , m_worker(new GPhotoWorker)
{
    m_worker->moveToThread(m_workerThread.get());

    connect(m_worker.get(), &GPhotoWorker::captureModeChanged, this, &GPhotoController::onCaptureModeChanged);
    connect(m_worker.get(), &GPhotoWorker::error, this, &GPhotoController::error);
    connect(m_worker.get(), &GPhotoWorker::imageCaptureError, this, &GPhotoController::imageCaptureError);
    connect(m_worker.get(), &GPhotoWorker::imageCaptured, this, &GPhotoController::imageCaptured);
    connect(m_worker.get(), &GPhotoWorker::previewCaptured, this, &GPhotoController::previewCaptured);
    connect(m_worker.get(), &GPhotoWorker::readyForCaptureChanged, this, &GPhotoController::readyForCaptureChanged);
    connect(m_worker.get(), &GPhotoWorker::stateChanged, this, &GPhotoController::onStateChanged);
    connect(m_worker.get(), &GPhotoWorker::statusChanged, this, &GPhotoController::onStatusChanged);

    m_workerThread->start();
}

GPhotoController::~GPhotoController()
{
    m_workerThread->requestInterruption();
    m_workerThread->quit();
    if (!m_workerThread->wait(workerThreadTimeout))
        m_workerThread->terminate();
}

bool GPhotoController::init()
{
    auto result = false;
    QMetaObject::invokeMethod(m_worker.get(), "init", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result));
    return result;
}

QList<QByteArray> GPhotoController::cameraNames() const
{
    QList<QByteArray> result;
    QMetaObject::invokeMethod(m_worker.get(), "cameraNames", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QList<QByteArray>, result));
    return result;
}

QByteArray GPhotoController::defaultCameraName() const
{
    QByteArray result;
    QMetaObject::invokeMethod(m_worker.get(), "defaultCameraName", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QByteArray, result));
    return result;
}

void GPhotoController::capturePhoto(int cameraIndex, int id, const QString &fileName) const
{
    QMetaObject::invokeMethod(m_worker.get(), "capturePhoto", Qt::QueuedConnection,
                              Q_ARG(int, cameraIndex), Q_ARG(int, id), Q_ARG(QString, fileName));
}

QCamera::CaptureModes GPhotoController::captureMode(int cameraIndex) const
{
    return m_captureModes.contains(cameraIndex) ? m_captureModes.value(cameraIndex) : QCamera::CaptureStillImage;
}

void GPhotoController::setCaptureMode(int cameraIndex, QCamera::CaptureModes captureMode)
{
    QMetaObject::invokeMethod(m_worker.get(), "setCaptureMode", Qt::QueuedConnection,
                              Q_ARG(int, cameraIndex), Q_ARG(QCamera::CaptureModes, captureMode));
}

QCamera::State GPhotoController::state(int cameraIndex) const
{
    return m_states.contains(cameraIndex) ? m_states.value(cameraIndex) : QCamera::UnloadedState;
}

void GPhotoController::setState(int cameraIndex, QCamera::State state) const
{
    QMetaObject::invokeMethod(m_worker.get(), "setState", Qt::QueuedConnection,
                              Q_ARG(int, cameraIndex), Q_ARG(QCamera::State, state));
}

QCamera::Status GPhotoController::status(int cameraIndex) const
{
    return m_statuses.contains(cameraIndex) ? m_statuses.value(cameraIndex) : QCamera::UnloadedStatus;
}

QVariant GPhotoController::parameter(int cameraIndex, const QString &name) const
{
    QVariant result;
    QMetaObject::invokeMethod(m_worker.get(), "parameter", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, result), Q_ARG(int, cameraIndex), Q_ARG(QString, name));
    return result;
}

bool GPhotoController::setParameter(int cameraIndex, const QString &name, const QVariant &value)
{
    auto result = false;
    QMetaObject::invokeMethod(m_worker.get(), "setParameter", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result), Q_ARG(int, cameraIndex),
                              Q_ARG(QString, name), Q_ARG(QVariant, value));
    return result;
}

QVariantList GPhotoController::parameterValues(int cameraIndex, const QString &name, QMetaType::Type valueType) const
{
    auto result = QVariantList();
    QMetaObject::invokeMethod(m_worker.get(), "parameterValues", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariantList, result), Q_ARG(int, cameraIndex),
                              Q_ARG(QString, name), Q_ARG(QMetaType::Type, valueType));
    return result;
}

void GPhotoController::onCaptureModeChanged(int cameraIndex, QCamera::CaptureModes captureMode)
{
    if (m_captureModes.value(cameraIndex, QCamera::CaptureStillImage) != captureMode) {
        m_captureModes[cameraIndex] = captureMode;
        emit captureModeChanged(cameraIndex, captureMode);
    }
}

void GPhotoController::onStateChanged(int cameraIndex, QCamera::State state)
{
    if (m_states.value(cameraIndex, QCamera::UnloadedState) != state) {
        m_states[cameraIndex] = state;
        emit stateChanged(cameraIndex, state);
    }
}

void GPhotoController::onStatusChanged(int cameraIndex, QCamera::Status status)
{
    if (m_statuses.value(cameraIndex, QCamera::UnloadedStatus) != status) {
        m_statuses[cameraIndex] = status;
        emit statusChanged(cameraIndex, status);
    }
}
