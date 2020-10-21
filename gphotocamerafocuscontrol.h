#ifndef GPHOTOCAMERAFOCUSCONTROL_H
#define GPHOTOCAMERAFOCUSCONTROL_H

#include <QCameraFocusControl>

class GPhotoCameraFocusControl final : public QCameraFocusControl
{
    Q_OBJECT
  public:
    explicit GPhotoCameraFocusControl(QObject *parent = nullptr);

  public:
    QCameraFocus::FocusModes focusMode() const final;
    void setFocusMode(QCameraFocus::FocusModes mode) final;
    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const final;
    QCameraFocus::FocusPointMode focusPointMode() const final;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) final;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const final;
    QPointF customFocusPoint() const final;
    void setCustomFocusPoint(const QPointF &point) final;
    QCameraFocusZoneList focusZones() const final;

  private:
    QCameraFocus::FocusModes m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QPointF m_focusPoint;
};

#endif // GPHOTOCAMERAFOCUSCONTROL_H
