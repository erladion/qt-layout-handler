#include "mirroredappitem.h"

#include <QApplication>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QPainter>

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

MirroredAppItem::MirroredAppItem(const QString& captureSource)
    : ResizableAppItem("Live Stream", QRectF(0, 0, 800, 600)), m_captureSource(captureSource) {
  setCacheMode(QGraphicsItem::NoCache);  // CRITICAL for video performance

  m_cropThrottleTimer = new QTimer(this);
  m_cropThrottleTimer->setSingleShot(true);
  connect(m_cropThrottleTimer, &QTimer::timeout, this, [this]() {
    if (!m_pipeline || m_isRecording)
      return;
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    GstElement* crop = gst_bin_get_by_name(GST_BIN(m_pipeline), "mycrop");
    if (crop) {
      g_object_set(crop, "top", m_cropTop, "bottom", m_cropBottom, "left", m_cropLeft, "right", m_cropRight, nullptr);
      gst_object_unref(crop);
    }
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    m_cropPending = false;
  });

  m_pRenderTimer = new QTimer(this);
  connect(m_pRenderTimer, &QTimer::timeout, this, [this]() {
    if (m_frameReady) {
      if (scene())
        scene()->invalidate(sceneBoundingRect(), QGraphicsScene::ItemLayer);
      m_frameReady = false;
    }
  });
  m_pRenderTimer->start(16);  // 60 FPS UI polling

  rebuildPipeline();

  setupCustomActions();
}

MirroredAppItem::~MirroredAppItem() {
  if (m_pRenderTimer)
    m_pRenderTimer->stop();
  if (m_pipeline) {
    if (m_isRecording) {
      gst_element_send_event(m_pipeline, gst_event_new_eos());
      GstBus* bus = gst_element_get_bus(m_pipeline);
      GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_SECOND * 2, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
      if (msg)
        gst_message_unref(msg);
      gst_object_unref(bus);
    }
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
  }
}

QString MirroredAppItem::generatePipelineString() {
  // THE FIX: Added videoconvert and videorate right after the source
  QString baseStr = QString(
                        "%1 ! "
                        "videoconvert ! "
                        "videorate ! "
                        "video/x-raw,framerate=30/1 ! "
                        "videocrop name=mycrop top=%2 bottom=%3 left=%4 right=%5 ! ")
                        .arg(m_captureSource)
                        .arg(m_cropTop)
                        .arg(m_cropBottom)
                        .arg(m_cropLeft)
                        .arg(m_cropRight);

  if (!m_isRecording) {
    return baseStr + "videoconvert ! video/x-raw,format=BGRx ! appsink name=mysink";
  } else {
    return baseStr + QString(
                         "tee name=t ! "
                         "queue ! videoconvert ! video/x-raw,format=BGRx ! appsink name=mysink "
                         "t. ! queue ! videoconvert ! nvh264enc zerolatency=true ! h264parse ! matroskamux ! filesink location=\"%1\"")
                         .arg(m_recordFilename);
  }
}

void MirroredAppItem::rebuildPipeline() {
  if (m_pipeline) {
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
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
  // 1. Start the pipeline
  gst_element_set_state(m_pipeline, GST_STATE_PLAYING);

  // ==========================================
  // 2. TEMPORARY DEBUG TRAP
  // ==========================================
  // Wait for up to 1 second to see if GStreamer immediately panics
  GstBus* bus = gst_element_get_bus(m_pipeline);
  GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_SECOND * 1, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_WARNING));

  if (msg != nullptr) {
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
      GError* err;
      gchar* debug_info;
      gst_message_parse_error(msg, &err, &debug_info);
      qDebug() << "GSTREAMER FATAL ERROR:" << err->message;
      qDebug() << "Debug Info:" << (debug_info ? debug_info : "none");
      g_error_free(err);
      g_free(debug_info);
    } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_WARNING) {
      GError* err;
      gchar* debug_info;
      gst_message_parse_warning(msg, &err, &debug_info);
      qDebug() << "GSTREAMER WARNING:" << err->message;
      g_error_free(err);
      g_free(debug_info);
    } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
      qDebug() << "GSTREAMER EOS: The stream ended immediately. gdiscreencapsrc could not bind to the window.";
    }
    gst_message_unref(msg);
  } else {
    qDebug() << "GStreamer started successfully. No immediate errors.";
  }
  gst_object_unref(bus);
  // ==========================================
}

void MirroredAppItem::setupCustomActions() {
  QString actionText = m_isRecording ? "Stop Recording" : "Start Recording";
  QAction* recordAction = new QAction(actionText, this);

  connect(recordAction, &QAction::triggered, this, [this]() {
    if (!m_isRecording) {
      m_recordFilename = QFileDialog::getSaveFileName(nullptr, "Save Video", "", "Video (*.mkv)", nullptr, QFileDialog::DontUseNativeDialog);
      if (m_recordFilename.isEmpty())
        return;
      m_isRecording = true;
      rebuildPipeline();
    } else {
      m_isRecording = false;
      rebuildPipeline();
    }
  });
  m_pContextMenu.addAction(recordAction);
}

void MirroredAppItem::updateCropValues(int top, int bottom, int left, int right) {
  if (m_sourceSize.isValid()) {
    int maxWidth = m_sourceSize.width();
    int maxHeight = m_sourceSize.height();
    left = std::max(0, left);
    right = std::max(0, right);
    top = std::max(0, top);
    bottom = std::max(0, bottom);
    if (left + right >= maxWidth)
      right = maxWidth - left - 1;
    if (top + bottom >= maxHeight)
      bottom = maxHeight - top - 1;
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

GstFlowReturn MirroredAppItem::onNewSample(GstElement* sink, gpointer data) {
  MirroredAppItem* item = static_cast<MirroredAppItem*>(data);
  GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
  if (!sample)
    return GST_FLOW_ERROR;

  GstBuffer* buffer = gst_sample_get_buffer(sample);
  GstCaps* caps = gst_sample_get_caps(sample);
  GstStructure* s = gst_caps_get_structure(caps, 0);

  int width, height;
  gst_structure_get_int(s, "width", &width);
  gst_structure_get_int(s, "height", &height);

  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);

  QImage tempImg((uchar*)map.data, width, height, width * 4, QImage::Format_RGB32);
  QImage copiedImg = tempImg.copy();

  gst_buffer_unmap(buffer, &map);

  item->m_frameMutex.lock();
  item->m_currentFrame = copiedImg;
  item->m_frameReady = true;
  item->m_frameMutex.unlock();

  if (item->m_sourceSize != QSize(width, height)) {
    QTimer::singleShot(0, qApp, [item, width, height]() {
      item->m_sourceSize = QSize(width, height);
      double targetRatio = static_cast<double>(width) / height;
      item->setTargetAspectRatio(targetRatio);
      item->setAspectRatioEnabled(true);
      QRectF currentRect = item->rect();
      item->setRect(0, 0, currentRect.width(), currentRect.width() / targetRatio);
      item->updateStatusText();
    });
  }

  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

void MirroredAppItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  m_frameMutex.lock();
  QImage frameToDraw = m_currentFrame;
  m_frameMutex.unlock();

  if (!frameToDraw.isNull()) {
    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawImage(rect(), frameToDraw);
    painter->restore();
  } else {
    painter->fillRect(rect(), QColor(40, 40, 40, 200));
    painter->setPen(Qt::white);
    painter->drawText(rect(), Qt::AlignCenter, "Waiting for Video Feed...");
  }

  QBrush originalBrush = brush();
  setBrush(Qt::NoBrush);
  ResizableAppItem::paint(painter, option, widget);
  setBrush(originalBrush);
}

void MirroredAppItem::updateStatusText() {
  QPointF p = scenePos();
  QString status = QString("%1, %2").arg((int)p.x()).arg((int)p.y());

  if (m_sourceSize.isValid()) {
    status +=
        QString(" (Src: %1x%2, Disp: %3x%4)").arg(m_sourceSize.width()).arg(m_sourceSize.height()).arg((int)rect().width()).arg((int)rect().height());
  }
  if (isLocked())
    status += " [LOCKED]";

  m_pStatusText->setPlainText(status);
  m_pStatusText->setPos(5, rect().height() - m_pStatusText->boundingRect().height());
}
