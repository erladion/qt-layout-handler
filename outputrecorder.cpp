#include "outputrecorder.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QTimer>
#include <QtMath>

#include <cstring>

#include <gst/app/gstappsrc.h>

#include "gstutils.h"

OutputRecorder::OutputRecorder(QObject* parent) : QObject(parent) {
  m_pTimer = new QTimer(this);
  connect(m_pTimer, &QTimer::timeout, this, &OutputRecorder::grabFrame);
}

OutputRecorder::~OutputRecorder() {
  stop();
}

bool OutputRecorder::start(QGraphicsScene* scene, const QString& filename) {
  if (m_isRecording || !scene || filename.isEmpty()) {
    return false;
  }

  const QRectF sceneRect = scene->sceneRect();
  if (sceneRect.width() < 1.0 || sceneRect.height() < 1.0) {
    return false;
  }

  // Cap the long edge to keep file size/encoder load sane, preserving aspect.
  // Encoders require even dimensions, so round each down to a multiple of 2.
  const double maxHeight = 1080.0;
  const double scale = qMin(1.0, maxHeight / sceneRect.height());
  m_width = static_cast<int>(sceneRect.width() * scale) & ~1;
  m_height = static_cast<int>(sceneRect.height() * scale) & ~1;
  if (m_width < 2 || m_height < 2) {
    return false;
  }

  const QString encoder = selectH264Encoder();
  const QString pipelineStr =
      QString(
          "appsrc name=mysrc is-live=true format=time do-timestamp=false ! "
          "videoconvert ! %1 ! h264parse ! matroskamux ! "
          "filesink location=\"%2\"")
          .arg(encoder, filename);

  GError* error = nullptr;
  m_pPipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), &error);
  if (error) {
    g_error_free(error);
    if (m_pPipeline) {
      gst_object_unref(m_pPipeline);
      m_pPipeline = nullptr;
    }
    return false;
  }

  m_pAppSrc = gst_bin_get_by_name(GST_BIN(m_pPipeline), "mysrc");
  if (!m_pAppSrc) {
    gst_object_unref(m_pPipeline);
    m_pPipeline = nullptr;
    return false;
  }

  GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRx", "width", G_TYPE_INT, m_width, "height", G_TYPE_INT, m_height,
                                      "framerate", GST_TYPE_FRACTION, m_fps, 1, nullptr);
  gst_app_src_set_caps(GST_APP_SRC(m_pAppSrc), caps);
  gst_caps_unref(caps);

  if (gst_element_set_state(m_pPipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    gst_object_unref(m_pAppSrc);
    m_pAppSrc = nullptr;
    gst_object_unref(m_pPipeline);
    m_pPipeline = nullptr;
    return false;
  }

  m_frameImage = QImage(m_width, m_height, QImage::Format_RGB32);
  m_frameCount = 0;
  m_pScene = scene;
  m_isRecording = true;
  m_pTimer->start(1000 / m_fps);
  return true;
}

void OutputRecorder::grabFrame() {
  if (!m_isRecording || !m_pScene || !m_pAppSrc) {
    return;
  }

  m_frameImage.fill(Qt::black);
  QPainter painter(&m_frameImage);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  // Image dimensions already match the scene's aspect ratio, so stretch to fill.
  m_pScene->render(&painter, QRectF(0, 0, m_width, m_height), m_pScene->sceneRect(), Qt::IgnoreAspectRatio);
  painter.end();

  const gsize size = static_cast<gsize>(m_frameImage.sizeInBytes());
  GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);

  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  std::memcpy(map.data, m_frameImage.constBits(), size);
  gst_buffer_unmap(buffer, &map);

  const GstClockTime duration = gst_util_uint64_scale(GST_SECOND, 1, m_fps);
  GST_BUFFER_PTS(buffer) = m_frameCount * duration;
  GST_BUFFER_DURATION(buffer) = duration;
  ++m_frameCount;

  // push-buffer takes ownership of the buffer; no unref needed afterwards.
  gst_app_src_push_buffer(GST_APP_SRC(m_pAppSrc), buffer);
}

void OutputRecorder::stop() {
  if (!m_isRecording) {
    return;
  }
  m_isRecording = false;
  m_pTimer->stop();

  if (m_pAppSrc) {
    gst_app_src_end_of_stream(GST_APP_SRC(m_pAppSrc));
  }

  if (m_pPipeline) {
    // Wait briefly for EOS to propagate so the muxer finalizes the file.
    GstBus* bus = gst_element_get_bus(m_pPipeline);
    if (bus) {
      gst_bus_timed_pop_filtered(bus, 3 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
      gst_object_unref(bus);
    }
    gst_element_set_state(m_pPipeline, GST_STATE_NULL);
  }

  if (m_pAppSrc) {
    gst_object_unref(m_pAppSrc);
    m_pAppSrc = nullptr;
  }
  if (m_pPipeline) {
    gst_object_unref(m_pPipeline);
    m_pPipeline = nullptr;
  }
  m_pScene = nullptr;
}
