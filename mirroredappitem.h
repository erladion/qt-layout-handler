#ifndef MIRROREDAPPITEM_H
#define MIRROREDAPPITEM_H

#include <QRectF>
#include <QString>

#include "constants.h"
#include "resizableappitem.h"

class CapturePipeline;
class CropHandleItem;

// Capture settings that survive connect/disconnect and are serialized with the
// layout. While connected, the live CapturePipeline mirrors these values.
struct CaptureSettings {
  int cropTop = 0;
  int cropBottom = 0;
  int cropLeft = 0;
  int cropRight = 0;
  int framerate = 30;
  bool useDamage = false;
};

// A scene item that mirrors a live window. It owns a CapturePipeline (the
// GStreamer side) when connected, and is responsible for rendering frames, the
// interactive crop UI, the context menu, and carrying the stable window identity
// (WM_CLASS + title) so a saved layout can re-bind it to a live window.
class MirroredAppItem : public ResizableAppItem {
  Q_OBJECT
public:
  // An empty captureSource creates a disconnected placeholder (no live window
  // matched yet); it can be bound later via bindToSource().
  MirroredAppItem(const QString& captureSource, const QString& appClass = QString(), const QString& appTitle = QString(),
                  const CaptureSettings& settings = CaptureSettings());

  enum { Type = Constants::Item::MirroredAppItem };
  int type() const override { return Type; }

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  bool isConnected() const { return m_pPipeline != nullptr; }
  void bindToSource(const QString& captureSource);

  // Stable identity used to re-match this capture to a live window on load.
  QString appClass() const { return m_appClass; }
  QString appTitle() const { return m_appTitle; }
  void setIdentity(const QString& appClass, const QString& appTitle);

  // The current settings (read from the live pipeline when connected).
  CaptureSettings captureSettings() const;

  // Cropping. Values live in the capture pipeline (or pending settings while
  // disconnected); these forward to them.
  void updateCropValues(int top, int bottom, int left, int right);
  int cropTop() const;
  int cropBottom() const;
  int cropLeft() const;
  int cropRight() const;

  void updateCropHandles(CropHandleItem* movedHandle, int position);
  void applyInteractiveCrop();

signals:
  // Emitted from the "Bind to window…" action so the controller can show the
  // open-window picker and call bindToSource().
  void rebindRequested(MirroredAppItem* item);

protected:
  void setupCustomActions();
  void updateStatusText() override;

  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
  void enterCropMode();
  void exitCropMode();
  void wirePipeline();           // Connects the pipeline's frame/size signals.
  void applyPendingToPipeline();  // Pushes m_pendingSettings into the pipeline.

  CapturePipeline* m_pPipeline = nullptr;

  QString m_appClass;
  QString m_appTitle;
  CaptureSettings m_pendingSettings;  // Authoritative while disconnected.

  // Interactive crop UI state.
  bool m_isCropping = false;
  QRectF m_tempCropRect;

  CropHandleItem* m_topLeftHandle = nullptr;
  CropHandleItem* m_topRightHandle = nullptr;
  CropHandleItem* m_bottomLeftHandle = nullptr;
  CropHandleItem* m_bottomRightHandle = nullptr;
  CropHandleItem* m_applyButton = nullptr;
};

#endif  // MIRROREDAPPITEM_H
