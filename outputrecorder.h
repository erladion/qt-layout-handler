#ifndef OUTPUTRECORDER_H
#define OUTPUTRECORDER_H

#include <QImage>
#include <QObject>
#include <QPointer>

#include <gst/gst.h>

class QGraphicsScene;
class QTimer;

// Records the program output (the composited scene shown on the projector) to
// an .mkv file. Renders the scene at a fixed frame rate on the GUI thread and
// pushes each frame into a GStreamer appsrc -> H.264 -> matroskamux pipeline.
class OutputRecorder : public QObject {
  Q_OBJECT
public:
  explicit OutputRecorder(QObject* parent = nullptr);
  ~OutputRecorder() override;

  // Begins recording the given scene to filename. Returns false if it could not
  // start (no scene, an empty scene rect, or the pipeline failed to launch).
  bool start(QGraphicsScene* scene, const QString& filename);
  void stop();
  bool isRecording() const { return m_isRecording; }

private:
  void grabFrame();

  QPointer<QGraphicsScene> m_pScene;
  GstElement* m_pPipeline = nullptr;
  GstElement* m_pAppSrc = nullptr;
  QTimer* m_pTimer = nullptr;

  bool m_isRecording = false;
  int m_width = 0;
  int m_height = 0;
  int m_fps = 30;
  quint64 m_frameCount = 0;

  QImage m_frameImage;  // Reused render target, avoids a per-frame allocation.
};

#endif  // OUTPUTRECORDER_H
