#ifndef CAPTUREPIPELINE_H
#define CAPTUREPIPELINE_H

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QSize>
#include <QString>

#include <gst/gst.h>

class QTimer;

// Owns the GStreamer capture/record pipeline for a single window source: builds
// the pipeline, publishes decoded frames via frameReady(), and applies live
// crop / frame-rate / use-damage / recording changes. Frame delivery is
// thread-safe; frameReady() and sourceSizeChanged() are emitted from GStreamer's
// streaming thread, so connect to them with a queued connection.
class CapturePipeline : public QObject {
  Q_OBJECT
public:
  explicit CapturePipeline(const QString& captureSource, QObject* parent = nullptr);
  ~CapturePipeline() override;

  // Latest decoded frame (thread-safe; null until the first frame arrives).
  QImage currentFrame() const;
  // Size of the latest frame, i.e. the post-crop source resolution (thread-safe).
  QSize sourceSize() const;

  bool isXImageSource() const;

  // Recording.
  bool isRecording() const { return m_isRecording; }
  void startRecording(const QString& filename);
  void stopRecording();

  // Capture tuning (each rebuilds the pipeline).
  int framerate() const { return m_captureFramerate; }
  void setFramerate(int fps);
  bool useDamage() const { return m_useDamage; }
  void setUseDamage(bool on);

  // Crop. setCrop replaces the crop (clamped to the source) and applies it
  // throttled; addCrop adds to the current crop and applies it immediately
  // (used by the interactive crop, whose values are cumulative).
  int cropTop() const { return m_cropTop; }
  int cropBottom() const { return m_cropBottom; }
  int cropLeft() const { return m_cropLeft; }
  int cropRight() const { return m_cropRight; }
  void setCrop(int top, int bottom, int left, int right);
  void addCrop(int top, int bottom, int left, int right);

signals:
  void frameReady();
  void sourceSizeChanged(const QSize& size);

private:
  void rebuild();
  QString generatePipelineString();
  void applyCropLive(bool pauseWhileSetting);

  static GstFlowReturn onNewSample(GstElement* sink, gpointer data);
  static gboolean busCall(GstBus* bus, GstMessage* message, gpointer data);

  QString m_captureSource;

  GstElement* m_pipeline = nullptr;
  guint m_busWatchId = 0;

  bool m_isRecording = false;
  QString m_recordFilename;

  int m_captureFramerate = 30;
  bool m_useDamage = false;

  int m_cropTop = 0;
  int m_cropBottom = 0;
  int m_cropLeft = 0;
  int m_cropRight = 0;
  QTimer* m_cropThrottleTimer = nullptr;
  bool m_cropPending = false;

  // Thread-safe frame handoff.
  mutable QMutex m_frameMutex;
  QImage m_currentFrame;  // guarded by m_frameMutex
  QSize m_sourceSize;     // guarded by m_frameMutex
  QImage m_bufferImage;   // streaming thread only
  QSize m_lastFrameSize;  // streaming thread only
};

#endif  // CAPTUREPIPELINE_H
