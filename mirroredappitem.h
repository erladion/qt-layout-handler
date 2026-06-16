#ifndef MIRROREDAPPITEM_H
#define MIRROREDAPPITEM_H

#include <QRectF>

#include "constants.h"
#include "resizableappitem.h"

class CapturePipeline;
class CropHandleItem;

// A scene item that mirrors a live window. It owns a CapturePipeline (the
// GStreamer side) and is responsible only for rendering frames, the interactive
// crop UI, and the context menu.
class MirroredAppItem : public ResizableAppItem {
  Q_OBJECT
public:
  MirroredAppItem(const QString& captureSource);

  enum { Type = Constants::Item::MirroredAppItem };
  int type() const override { return Type; }

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  // Cropping. Values live in the capture pipeline; these forward to it.
  void updateCropValues(int top, int bottom, int left, int right);
  int cropTop() const;
  int cropBottom() const;
  int cropLeft() const;
  int cropRight() const;

  void updateCropHandles(CropHandleItem* movedHandle, int position);
  void applyInteractiveCrop();

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

  CapturePipeline* m_pPipeline = nullptr;

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
