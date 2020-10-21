#ifndef GPHOTOCAMERALOCKCONTROL_H
#define GPHOTOCAMERALOCKCONTROL_H

#include <QCamera>
#include <QCameraLocksControl>

class GPhotoCameraSession;

class GPhotoCameraLockControl final : public QCameraLocksControl
{
    Q_OBJECT
  public:
    explicit GPhotoCameraLockControl(GPhotoCameraSession *session, QObject *parent = nullptr);

    QCamera::LockTypes supportedLocks() const final;
    QCamera::LockStatus lockStatus(QCamera::LockType lock) const final;
    void searchAndLock(QCamera::LockTypes locks) final;
    void unlock(QCamera::LockTypes locks) final;

  private slots:
    void onStatusChanged(QCamera::Status status);

  private:
    void setLockStatus(QCamera::LockType lock, QCamera::LockStatus status, QCamera::LockChangeReason reason);
    void startFocusing();
    void stopFocusing();

    GPhotoCameraSession *const m_session;
    QCamera::LockTypes m_pendingLocks;
    QMap<QCamera::LockType, QCamera::LockStatus> m_lockStatus;
};

#endif // GPHOTOCAMERALOCKCONTROL_H
