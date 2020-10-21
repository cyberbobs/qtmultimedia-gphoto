#include "gphotocamerafocuscontrol.h"

GPhotoCameraFocusControl::GPhotoCameraFocusControl(QObject *parent)
    : QCameraFocusControl(parent)
    , m_focusMode(QCameraFocus::AutoFocus)
    , m_focusPointMode(QCameraFocus::FocusPointAuto)
    , m_focusPoint(0.5, 0.5)
{
}

QCameraFocus::FocusModes GPhotoCameraFocusControl::focusMode() const
{
    return m_focusMode;
}

void GPhotoCameraFocusControl::setFocusMode(QCameraFocus::FocusModes mode)
{
    m_focusMode = mode;
}

bool GPhotoCameraFocusControl::isFocusModeSupported(QCameraFocus::FocusModes mode) const
{
    switch (mode) {
    case QCameraFocus::AutoFocus:
        return true;
    default:
        return (mode & QCameraFocus::AutoFocus);
    }
}

QCameraFocus::FocusPointMode GPhotoCameraFocusControl::focusPointMode() const
{
    return m_focusPointMode;
}

void GPhotoCameraFocusControl::setFocusPointMode(QCameraFocus::FocusPointMode mode)
{
    if (m_focusPointMode != mode) {
        m_focusPointMode = mode;
        emit focusPointModeChanged(m_focusPointMode);
        emit focusZonesChanged();
    }
}

bool GPhotoCameraFocusControl::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const
{
    return (mode == QCameraFocus::FocusPointAuto);
}

QPointF GPhotoCameraFocusControl::customFocusPoint() const
{
    return m_focusPoint;
}

void GPhotoCameraFocusControl::setCustomFocusPoint(const QPointF &point)
{
  if (m_focusPoint != point) {
      m_focusPoint = point;
      emit customFocusPointChanged(m_focusPoint);
  }
}

QCameraFocusZoneList GPhotoCameraFocusControl::focusZones() const
{
    return QCameraFocusZoneList();
}
