#include "gphotocameralockcontrol.h"
#include "gphotocamerasession.h"

#include <QCameraFocusControl>

namespace {
    constexpr auto autofocusdriveParameter = "autofocusdrive";
    constexpr auto cancelautofocusParameter = "cancelautofocus";
}

GPhotoCameraLockControl::GPhotoCameraLockControl(GPhotoCameraSession *session, QObject *parent)
  : QCameraLocksControl(parent)
  , m_session(session)
{
    connect(session, &GPhotoCameraSession::statusChanged, this, &GPhotoCameraLockControl::onStatusChanged);
}

QCamera::LockTypes GPhotoCameraLockControl::supportedLocks() const
{
    return QCamera::LockFocus;
}

QCamera::LockStatus GPhotoCameraLockControl::lockStatus(QCamera::LockType lock) const
{
    if (QCamera::LockFocus == lock && m_pendingLocks & QCamera::LockFocus)
        return QCamera::Searching;

    return m_lockStatus.value(lock);
}

void GPhotoCameraLockControl::searchAndLock(QCamera::LockTypes locks)
{
    m_pendingLocks &= ~locks;

    if (locks & QCamera::LockFocus) {
        auto focusMode = m_session->cameraFocusControl()->focusMode();
        if (focusMode & QCameraFocus::AutoFocus) {
            if (QCamera::ActiveStatus == m_session->status())
                startFocusing();
            else
                m_pendingLocks |= QCamera::LockFocus;
        }
    }
}

void GPhotoCameraLockControl::unlock(QCamera::LockTypes locks)
{
    m_pendingLocks &= ~locks;

    if (locks & QCamera::LockFocus)
        stopFocusing();
}

void GPhotoCameraLockControl::onStatusChanged(QCamera::Status status)
{
    if (QCamera::ActiveStatus == status && m_pendingLocks & QCamera::LockFocus) {
        startFocusing();
    }
}

void GPhotoCameraLockControl::setLockStatus(QCamera::LockType lock, QCamera::LockStatus status,
                                            QCamera::LockChangeReason reason)
{
    auto oldStatus = m_lockStatus.value(lock, QCamera::Unlocked);
    if (oldStatus != status) {
        m_lockStatus.insert(lock, status);
        emit lockStatusChanged(lock, status, reason);
    }
}

void GPhotoCameraLockControl::startFocusing()
{
    if (QCamera::ActiveStatus == m_session->status()) {
        if (m_session->setParameter(QLatin1String(autofocusdriveParameter), true)) {
            m_pendingLocks &= ~QCamera::LockFocus;
            setLockStatus(QCamera::LockFocus, QCamera::Locked, QCamera::UserRequest);
        } else {
            setLockStatus(QCamera::LockFocus, QCamera::Locked, QCamera::LockFailed);
        }
    }
}

void GPhotoCameraLockControl::stopFocusing()
{
    if (m_session->parameter(QLatin1String(cancelautofocusParameter)).isValid()) {
        // Canon
        if (m_session->setParameter(QLatin1String(cancelautofocusParameter), true)) {
            setLockStatus(QCamera::LockFocus, QCamera::Unlocked, QCamera::UserRequest);
        }
    } else {
        // Nikon
        setLockStatus(QCamera::LockFocus, QCamera::Unlocked, QCamera::UserRequest);
    }
}
