#ifndef MIRROREDAPPITEM_H
#define MIRROREDAPPITEM_H

#include <QImage>
#include <QMutex>
#include <QTimer>

#include "constants.h"
#include "resizableappitem.h"

#include <gst/gst.h>

class MirroredAppItem : public ResizableAppItem {
  Q_OBJECT
public:
  MirroredAppItem(const QString& captureSource);
  ~MirroredAppItem();

  enum { Type = Constants::Item::MirroredAppItem };
  int type() const override { return Type; }

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  // Cropping
  void updateCropValues(int top, int bottom, int left, int right);
  int cropTop() const { return m_cropTop; }
  int cropBottom() const { return m_cropBottom; }
  int cropLeft() const { return m_cropLeft; }
  int cropRight() const { return m_cropRight; }

protected:
  void setupCustomActions() override;
  void updateStatusText() override;

private:
  void rebuildPipeline();
  QString generatePipelineString();
  static GstFlowReturn onNewSample(GstElement* sink, gpointer data);

  long m_targetXid;
  QSize m_sourceSize;

  // GStreamer Pipeline
  GstElement* m_pipeline = nullptr;
  bool m_isRecording = false;
  QString m_recordFilename;

  // Crop State
  int m_cropTop = 0;
  int m_cropBottom = 0;
  int m_cropLeft = 0;
  int m_cropRight = 0;
  QTimer* m_cropThrottleTimer;
  bool m_cropPending = false;

  // Thread-safe Rendering
  QImage m_currentFrame;
  QMutex m_frameMutex;
  bool m_frameReady = false;
  QTimer* m_pRenderTimer;

  QString m_captureSource;
};

#endif  // MIRROREDAPPITEM_H
