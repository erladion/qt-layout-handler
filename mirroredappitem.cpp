#include "mirroredappitem.h"

#include <QActionGroup>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

#include "capturepipeline.h"
#include "crophandleitem.h"

MirroredAppItem::MirroredAppItem(const QString& captureSource, const QString& appClass, const QString& appTitle, const CaptureSettings& settings)
    : ResizableAppItem("", QRectF(0, 0, 800, 600)), m_appClass(appClass), m_appTitle(appTitle), m_pendingSettings(settings) {
  setCacheMode(QGraphicsItem::NoCache);

  if (!captureSource.isEmpty()) {
    m_pPipeline = new CapturePipeline(captureSource, this);
    applyPendingToPipeline();
    wirePipeline();
  }

  setupCustomActions();
}

void MirroredAppItem::wirePipeline() {
  // Repaint when a new frame arrives. The signal comes from the streaming
  // thread, so deliver it queued onto the GUI thread.
  connect(
      m_pPipeline, &CapturePipeline::frameReady, this,
      [this]() {
        if (scene()) {
          scene()->invalidate(sceneBoundingRect(), QGraphicsScene::ItemLayer);
        }
      },
      Qt::QueuedConnection);

  // Match the item's aspect ratio / size to the incoming source resolution.
  connect(
      m_pPipeline, &CapturePipeline::sourceSizeChanged, this,
      [this](const QSize& size) {
        if (size.height() <= 0) {
          return;
        }
        const double targetRatio = static_cast<double>(size.width()) / size.height();
        setTargetAspectRatio(targetRatio);
        setAspectRatioEnabled(true);
        const QRectF currentRect = rect();
        setRect(0, 0, currentRect.width(), currentRect.width() / targetRatio);
        updateStatusText();
      },
      Qt::QueuedConnection);
}

void MirroredAppItem::applyPendingToPipeline() {
  if (m_pPipeline) {
    m_pPipeline->setCaptureSettings(m_pendingSettings.cropTop, m_pendingSettings.cropBottom, m_pendingSettings.cropLeft, m_pendingSettings.cropRight,
                                    m_pendingSettings.framerate, m_pendingSettings.useDamage);
  }
}

void MirroredAppItem::bindToSource(const QString& captureSource) {
  if (captureSource.isEmpty()) {
    return;
  }

  // Preserve current settings, then swap pipelines.
  m_pendingSettings = captureSettings();
  delete m_pPipeline;  // null-safe; the old pipeline finalizes itself.
  m_pPipeline = new CapturePipeline(captureSource, this);
  applyPendingToPipeline();
  wirePipeline();

  updateStatusText();
  update();
}

void MirroredAppItem::setIdentity(const QString& appClass, const QString& appTitle) {
  m_appClass = appClass;
  m_appTitle = appTitle;
  updateStatusText();
  update();
}

CaptureSettings MirroredAppItem::captureSettings() const {
  if (m_pPipeline) {
    return {m_pPipeline->cropTop(),   m_pPipeline->cropBottom(), m_pPipeline->cropLeft(),
            m_pPipeline->cropRight(),  m_pPipeline->framerate(),  m_pPipeline->useDamage()};
  }
  return m_pendingSettings;
}

void MirroredAppItem::setupCustomActions() {
  QAction* bindAction = new QAction("Bind to window…", this);
  connect(bindAction, &QAction::triggered, this, [this]() { emit rebindRequested(this); });
  m_pContextMenu.addAction(bindAction);

  QAction* recordAction = new QAction("Start Recording", this);
  connect(recordAction, &QAction::triggered, this, [this]() {
    if (!m_pPipeline) {
      return;
    }
    if (!m_pPipeline->isRecording()) {
      const QString filename = QFileDialog::getSaveFileName(nullptr, "Save Video", "", "Video (*.mkv)", nullptr, QFileDialog::DontUseNativeDialog);
      if (filename.isEmpty()) {
        return;
      }
      m_pPipeline->startRecording(filename);
    } else {
      m_pPipeline->stopRecording();
    }
  });
  m_pContextMenu.addAction(recordAction);

  QAction* interactiveCropAction = new QAction("Interactive Crop", this);
  connect(interactiveCropAction, &QAction::triggered, this, [this]() {
    if (m_pPipeline) {
      enterCropMode();
    }
  });
  m_pContextMenu.addAction(interactiveCropAction);

  // Per-item capture frame rate. Dropping heavy windows (e.g. VS Code) to a
  // lower rate reduces the X-server load and repaint cost they impose.
  QMenu* fpsMenu = m_pContextMenu.addMenu("Capture FPS");
  QActionGroup* fpsGroup = new QActionGroup(this);
  fpsGroup->setExclusive(true);
  const int fpsOptions[] = {5, 10, 15, 24, 30, 60};
  const int currentFps = captureSettings().framerate;
  for (int fps : fpsOptions) {
    QAction* fpsAction = fpsMenu->addAction(QString("%1 fps").arg(fps));
    fpsAction->setCheckable(true);
    fpsAction->setChecked(fps == currentFps);
    fpsGroup->addAction(fpsAction);
    connect(fpsAction, &QAction::triggered, this, [this, fps]() {
      if (m_pPipeline) {
        m_pPipeline->setFramerate(fps);
      } else {
        m_pendingSettings.framerate = fps;
      }
    });
  }

  // use-damage only affects X11 ximagesrc captures, but we always offer it (it's
  // a harmless no-op on other sources) so disconnected placeholders can set it.
  QAction* damageAction = new QAction("Use XDamage (lighter, jittery)", this);
  damageAction->setCheckable(true);
  damageAction->setChecked(captureSettings().useDamage);
  connect(damageAction, &QAction::toggled, this, [this](bool on) {
    if (m_pPipeline) {
      m_pPipeline->setUseDamage(on);
    } else {
      m_pendingSettings.useDamage = on;
    }
  });
  m_pContextMenu.addAction(damageAction);
}

void MirroredAppItem::enterCropMode() {
  if (m_isCropping) {
    return;
  }
  m_isCropping = true;
  m_tempCropRect = rect();  // Start with the full current size.

  m_topLeftHandle = new CropHandleItem(CropHandleItem::TopLeft, this);
  m_topRightHandle = new CropHandleItem(CropHandleItem::TopRight, this);
  m_bottomLeftHandle = new CropHandleItem(CropHandleItem::BottomLeft, this);
  m_bottomRightHandle = new CropHandleItem(CropHandleItem::BottomRight, this);

  m_applyButton = new CropHandleItem(CropHandleItem::ApplyButton, this);

  m_topLeftHandle->setPos(m_tempCropRect.topLeft());
  m_topRightHandle->setPos(m_tempCropRect.topRight());
  m_bottomLeftHandle->setPos(m_tempCropRect.bottomLeft());
  m_bottomRightHandle->setPos(m_tempCropRect.bottomRight());

  // Position the apply button below the bottom-right corner.
  m_applyButton->setPos(m_tempCropRect.bottomRight() + QPointF(-30, 10));

  update();  // Force a repaint to show the overlay.
}

void MirroredAppItem::exitCropMode() {
  m_isCropping = false;
  delete m_topLeftHandle;
  m_topLeftHandle = nullptr;
  delete m_topRightHandle;
  m_topRightHandle = nullptr;
  delete m_bottomLeftHandle;
  m_bottomLeftHandle = nullptr;
  delete m_bottomRightHandle;
  m_bottomRightHandle = nullptr;
  delete m_applyButton;
  m_applyButton = nullptr;
  update();
}

void MirroredAppItem::updateCropHandles(CropHandleItem* movedHandle, int pos) {
  QPointF p = movedHandle->pos();

  // Clamp coordinates so handles can't cross each other.
  if (pos == CropHandleItem::TopLeft) {
    m_tempCropRect.setTopLeft(p);
    m_topRightHandle->setY(p.y());
    m_bottomLeftHandle->setX(p.x());
  } else if (pos == CropHandleItem::TopRight) {
    m_tempCropRect.setTopRight(p);
    m_topLeftHandle->setY(p.y());
    m_bottomRightHandle->setX(p.x());
  } else if (pos == CropHandleItem::BottomLeft) {
    m_tempCropRect.setBottomLeft(p);
    m_bottomRightHandle->setY(p.y());
    m_topLeftHandle->setX(p.x());
  } else if (pos == CropHandleItem::BottomRight) {
    m_tempCropRect.setBottomRight(p);
    m_bottomLeftHandle->setY(p.y());
    m_topRightHandle->setX(p.x());
  }

  // Move the apply button to follow the crop box.
  m_applyButton->setPos(m_tempCropRect.bottomRight() + QPointF(-30, 10));

  update();  // Redraw the dark overlay.
}

void MirroredAppItem::updateCropValues(int top, int bottom, int left, int right) {
  if (m_pPipeline) {
    m_pPipeline->setCrop(top, bottom, left, right);
  } else {
    m_pendingSettings.cropTop = top;
    m_pendingSettings.cropBottom = bottom;
    m_pendingSettings.cropLeft = left;
    m_pendingSettings.cropRight = right;
  }
}

int MirroredAppItem::cropTop() const {
  return m_pPipeline ? m_pPipeline->cropTop() : m_pendingSettings.cropTop;
}
int MirroredAppItem::cropBottom() const {
  return m_pPipeline ? m_pPipeline->cropBottom() : m_pendingSettings.cropBottom;
}
int MirroredAppItem::cropLeft() const {
  return m_pPipeline ? m_pPipeline->cropLeft() : m_pendingSettings.cropLeft;
}
int MirroredAppItem::cropRight() const {
  return m_pPipeline ? m_pPipeline->cropRight() : m_pendingSettings.cropRight;
}

void MirroredAppItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  if (!m_pPipeline) {
    // Disconnected placeholder: no live window matched.
    painter->save();
    painter->fillRect(rect(), QColor(40, 40, 40, 220));
    painter->setPen(Qt::white);
    const QString label = m_appTitle.isEmpty() ? m_appClass : m_appTitle;
    painter->drawText(rect(), Qt::AlignCenter, QString("Waiting for:\n%1\n\n(right-click → Bind to window)").arg(label));
    painter->restore();

    QBrush originalBrush = brush();
    setBrush(Qt::NoBrush);
    ResizableAppItem::paint(painter, option, widget);
    setBrush(originalBrush);
    return;
  }

  const QImage frameToDraw = m_pPipeline->currentFrame();

  if (!frameToDraw.isNull()) {
    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawImage(rect(), frameToDraw);
    painter->restore();
  } else {
    painter->save();
    painter->fillRect(rect(), QColor(40, 40, 40, 200));
    painter->setPen(Qt::white);
    painter->drawText(rect(), Qt::AlignCenter, "Waiting for Video Feed...");
    painter->restore();
  }

  QBrush originalBrush = brush();
  setBrush(Qt::NoBrush);
  ResizableAppItem::paint(painter, option, widget);
  setBrush(originalBrush);

  if (m_isCropping) {
    painter->save();
    // Dark overlay over the excluded areas.
    QPainterPath fullPath;
    fullPath.addRect(rect());

    QPainterPath cropPath;
    cropPath.addRect(m_tempCropRect);

    QPainterPath maskPath = fullPath.subtracted(cropPath);

    painter->setBrush(QColor(0, 0, 0, 150));  // Semi-transparent black.
    painter->setPen(Qt::NoPen);
    painter->drawPath(maskPath);

    // Dashed outline around the selected crop area.
    QPen dashedPen(Qt::white, 2, Qt::DashLine);
    painter->setPen(dashedPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_tempCropRect);
    painter->restore();
  }
}

void MirroredAppItem::applyInteractiveCrop() {
  if (!m_pPipeline) {
    exitCropMode();
    return;
  }

  int topDelta = 0, bottomDelta = 0, leftDelta = 0, rightDelta = 0;

  const QImage frame = m_pPipeline->currentFrame();
  if (!frame.isNull()) {
    // Crop fractions of the displayed rect map to pixels of the current
    // (already-cropped) frame; videocrop values are cumulative.
    const double leftPercent = m_tempCropRect.left() / rect().width();
    const double rightPercent = (rect().width() - m_tempCropRect.right()) / rect().width();
    const double topPercent = m_tempCropRect.top() / rect().height();
    const double bottomPercent = (rect().height() - m_tempCropRect.bottom()) / rect().height();

    leftDelta = static_cast<int>(leftPercent * frame.width());
    rightDelta = static_cast<int>(rightPercent * frame.width());
    topDelta = static_cast<int>(topPercent * frame.height());
    bottomDelta = static_cast<int>(bottomPercent * frame.height());
  }

  exitCropMode();
  m_pPipeline->addCrop(topDelta, bottomDelta, leftDelta, rightDelta);
}

void MirroredAppItem::updateStatusText() {
  QPointF p = scenePos();
  QString status = QString("%1, %2").arg((int)p.x()).arg((int)p.y());

  const QSize src = m_pPipeline ? m_pPipeline->sourceSize() : QSize();
  if (src.isValid()) {
    status += QString(" (Src: %1x%2, Disp: %3x%4)").arg(src.width()).arg(src.height()).arg((int)rect().width()).arg((int)rect().height());
  }
  if (isLocked()) {
    status += " [LOCKED]";
  }

  m_pStatusText->setPlainText(status);
  m_pStatusText->setPos(5, rect().height() - m_pStatusText->boundingRect().height());
}

void MirroredAppItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
  if (m_isCropping) {
    // Force a standard cursor so it doesn't show the resize arrows.
    setCursor(Qt::ArrowCursor);
    return;
  }
  ResizableAppItem::hoverMoveEvent(event);
}

void MirroredAppItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  if (m_isCropping) {
    // Accept the event so the click doesn't fall through to the scene.
    event->accept();
    return;
  }
  ResizableAppItem::mousePressEvent(event);
}

void MirroredAppItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  if (m_isCropping) {
    return;  // Block moving and resizing.
  }
  ResizableAppItem::mouseMoveEvent(event);
}

void MirroredAppItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  if (m_isCropping) {
    return;
  }
  ResizableAppItem::mouseReleaseEvent(event);
}
