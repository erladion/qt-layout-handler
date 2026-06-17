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
// pushes each frame into a GStreamer appsrc -> H.264 -> matroskamux pipeline,
// optionally muxing in the default audio input.
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

  // Whether to capture the default audio input into the recording (when an audio
  // encoder is available). Takes effect on the next start().
  bool audioEnabled() const { return m_audioEnabled; }
  void setAudioEnabled(bool on) { m_audioEnabled = on; }

private:
  void grabFrame();

  QPointer<QGraphicsScene> m_pScene;
  GstElement* m_pPipeline = nullptr;
  GstElement* m_pAppSrc = nullptr;
  QTimer* m_pTimer = nullptr;

  bool m_isRecording = false;
  bool m_audioEnabled = true;
  bool m_audioActive = false;  // True while a recording actually includes audio.
  int m_width = 0;
  int m_height = 0;
  int m_fps = 30;
  quint64 m_frameCount = 0;

  QImage m_frameImage;  // Reused render target, avoids a per-frame allocation.
};

#endif  // OUTPUTRECORDER_H
