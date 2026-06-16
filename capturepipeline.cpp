#include "capturepipeline.h"

#include <QDebug>
#include <QTimer>

#include <algorithm>
#include <cstring>
#include <thread>

#include <gst/app/gstappsink.h>

#include "gstutils.h"

CapturePipeline::CapturePipeline(const QString& captureSource, QObject* parent) : QObject(parent), m_captureSource(captureSource) {
  m_cropThrottleTimer = new QTimer(this);
  m_cropThrottleTimer->setSingleShot(true);
  connect(m_cropThrottleTimer, &QTimer::timeout, this, [this]() {
    if (!m_pipeline || m_isRecording) {
      return;
    }
    applyCropLive(true);
    m_cropPending = false;
  });

  rebuild();
}

CapturePipeline::~CapturePipeline() {
  if (m_busWatchId > 0) {
    g_source_remove(m_busWatchId);  // Remove the watch safely.
    m_busWatchId = 0;
  }

  if (m_pipeline) {
    GstElement* sink = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysink");
    if (sink) {
      g_signal_handlers_disconnect_by_func(sink, (gpointer)onNewSample, this);
      gst_object_unref(sink);
    }

    // Send EOS so any in-progress recording finalizes its file.
    gst_element_send_event(m_pipeline, gst_event_new_eos());

    // Hand the pipeline to a detached thread to wait for EOS and tear down, so
    // the GUI thread doesn't block.
    GstElement* pipelineToClean = m_pipeline;
    m_pipeline = nullptr;

    std::thread([pipelineToClean]() {
      GstBus* bus = gst_element_get_bus(pipelineToClean);
      GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_SECOND * 2, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
      if (msg) {
        gst_message_unref(msg);
      } else {
        qWarning() << "Timeout waiting for EOS during teardown.";
      }
      gst_object_unref(bus);
      gst_element_set_state(pipelineToClean, GST_STATE_NULL);
      gst_object_unref(pipelineToClean);
      qDebug() << "Pipeline teardown complete in background.";
    }).detach();
  }
}

QImage CapturePipeline::currentFrame() const {
  QMutexLocker lock(&m_frameMutex);
  return m_currentFrame;
}

QSize CapturePipeline::sourceSize() const {
  QMutexLocker lock(&m_frameMutex);
  return m_sourceSize;
}

bool CapturePipeline::isXImageSource() const {
  return m_captureSource.startsWith("ximagesrc");
}

void CapturePipeline::startRecording(const QString& filename) {
  if (m_isRecording || filename.isEmpty()) {
    return;
  }
  m_recordFilename = filename;
  m_isRecording = true;
  rebuild();
}

void CapturePipeline::stopRecording() {
  if (!m_isRecording) {
    return;
  }
  m_isRecording = false;
  rebuild();
}

void CapturePipeline::setFramerate(int fps) {
  if (m_captureFramerate == fps) {
    return;
  }
  m_captureFramerate = fps;
  rebuild();
}

void CapturePipeline::setUseDamage(bool on) {
  if (m_useDamage == on) {
    return;
  }
  m_useDamage = on;
  rebuild();
}

void CapturePipeline::setCrop(int top, int bottom, int left, int right) {
  const QSize src = sourceSize();
  if (src.isValid()) {
    const int maxWidth = src.width();
    const int maxHeight = src.height();
    left = std::max(0, left);
    right = std::max(0, right);
    top = std::max(0, top);
    bottom = std::max(0, bottom);
    if (left + right >= maxWidth) {
      right = maxWidth - left - 1;
    }
    if (top + bottom >= maxHeight) {
      bottom = maxHeight - top - 1;
    }
  }
  m_cropTop = top;
  m_cropBottom = bottom;
  m_cropLeft = left;
  m_cropRight = right;

  if (!m_cropPending) {
    m_cropPending = true;
    m_cropThrottleTimer->start(30);
  }
}

void CapturePipeline::addCrop(int top, int bottom, int left, int right) {
  m_cropTop += top;
  m_cropBottom += bottom;
  m_cropLeft += left;
  m_cropRight += right;
  applyCropLive(false);
}

void CapturePipeline::applyCropLive(bool pauseWhileSetting) {
  if (!m_pipeline) {
    return;
  }
  if (pauseWhileSetting) {
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
  }
  GstElement* crop = gst_bin_get_by_name(GST_BIN(m_pipeline), "mycrop");
  if (crop) {
    g_object_set(crop, "top", m_cropTop, "bottom", m_cropBottom, "left", m_cropLeft, "right", m_cropRight, nullptr);
    gst_object_unref(crop);
  }
  if (pauseWhileSetting) {
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  }
}

QString CapturePipeline::generatePipelineString() {
  // use-damage is an ximagesrc-only property, so inject the per-item choice
  // right after the element name (other sources are used as-is).
  QString source = m_captureSource;
  if (source.startsWith("ximagesrc")) {
    source = QString("ximagesrc use-damage=%1 %2")
                 .arg(m_useDamage ? QStringLiteral("true") : QStringLiteral("false"), source.mid(QStringLiteral("ximagesrc").length()).trimmed());
  }

  QString baseStr =
      QString(
          "%1 ! "
          "videoconvert ! "
          "videorate ! "
          "video/x-raw,framerate=%6/1 ! "
          "videocrop name=mycrop top=%2 bottom=%3 left=%4 right=%5 ! ")
          .arg(source, QString::number(m_cropTop), QString::number(m_cropBottom), QString::number(m_cropLeft), QString::number(m_cropRight),
               QString::number(m_captureFramerate));

  if (!m_isRecording) {
    return baseStr + "videoconvert ! video/x-raw,format=BGRx ! appsink name=mysink";
  } else {
    const QString encoder = selectH264Encoder();
    return baseStr + QString(
                         "tee name=t ! "
                         "queue ! videoconvert ! video/x-raw,format=BGRx ! appsink name=mysink "
                         "t. ! queue ! videoconvert ! %1 ! h264parse ! matroskamux ! filesink location=\"%2\"")
                         .arg(encoder, m_recordFilename);
  }
}

void CapturePipeline::rebuild() {
  if (m_pipeline) {
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
  }

  if (m_busWatchId > 0) {
    g_source_remove(m_busWatchId);
    m_busWatchId = 0;
  }

  QString pipelineStr = generatePipelineString();
  GError* error = nullptr;
  m_pipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), &error);

  if (error) {
    g_error_free(error);
    return;
  }

  GstElement* sink = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysink");
  if (sink) {
    g_object_set(sink, "max-buffers", 1, "drop", TRUE, "sync", FALSE, "emit-signals", TRUE, nullptr);
    g_signal_connect(sink, "new-sample", G_CALLBACK(onNewSample), this);
    gst_object_unref(sink);
  }

  gst_element_set_state(m_pipeline, GST_STATE_PLAYING);

  GstBus* bus = gst_element_get_bus(m_pipeline);
  m_busWatchId = gst_bus_add_watch(bus, busCall, this);
  gst_object_unref(bus);
}

GstFlowReturn CapturePipeline::onNewSample(GstElement* sink, gpointer data) {
  CapturePipeline* self = static_cast<CapturePipeline*>(data);
  GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
  if (!sample) {
    return GST_FLOW_ERROR;
  }

  GstBuffer* buffer = gst_sample_get_buffer(sample);
  GstCaps* caps = gst_sample_get_caps(sample);
  GstStructure* s = gst_caps_get_structure(caps, 0);

  int width = 0, height = 0;
  gst_structure_get_int(s, "width", &width);
  gst_structure_get_int(s, "height", &height);

  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);

  if (self->m_bufferImage.size() != QSize(width, height)) {
    // Only allocate when the frame size actually changes.
    self->m_bufferImage = QImage(width, height, QImage::Format_RGB32);
  }
  std::memcpy(self->m_bufferImage.bits(), map.data, map.size);

  gst_buffer_unmap(buffer, &map);

  {
    QMutexLocker lock(&self->m_frameMutex);
    self->m_currentFrame = self->m_bufferImage;
    self->m_sourceSize = QSize(width, height);
  }

  emit self->frameReady();

  // m_lastFrameSize is streaming-thread-only and gates the resize signal.
  if (self->m_lastFrameSize != QSize(width, height)) {
    self->m_lastFrameSize = QSize(width, height);
    emit self->sourceSizeChanged(QSize(width, height));
  }

  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

gboolean CapturePipeline::busCall(GstBus* bus, GstMessage* msg, gpointer data) {
  Q_UNUSED(bus);
  Q_UNUSED(data);

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gchar* debug_info = nullptr;
      gst_message_parse_error(msg, &err, &debug_info);
      qWarning() << "GStreamer Error:" << err->message;
      if (debug_info) {
        qWarning() << "Debug info:" << debug_info;
      }
      g_clear_error(&err);
      g_free(debug_info);
      break;
    }
    case GST_MESSAGE_WARNING: {
      GError* err = nullptr;
      gchar* debug_info = nullptr;
      gst_message_parse_warning(msg, &err, &debug_info);
      qWarning() << "GStreamer Warning:" << err->message;
      g_clear_error(&err);
      g_free(debug_info);
      break;
    }
    case GST_MESSAGE_EOS:
      qDebug() << "GStreamer End-Of-Stream reached.";
      break;
    default:
      break;
  }
  return TRUE;  // Keep the watch active.
}
