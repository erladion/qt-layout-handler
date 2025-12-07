#include "rulerbar.h"
#include <QDebug>
#include <QGraphicsView>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QStatusBar>
#include <cmath>
#include "guidelineitem.h"

RulerBar::RulerBar(Orientation orientation, QGraphicsView* view, QWidget* parent)
    : QWidget(parent), m_orientation(orientation), m_view(view), m_cursorPos(-1000), m_dragging(false) {
  setMouseTracking(true);

  if (m_orientation == Horizontal)
    setFixedHeight(25);
  else
    setFixedWidth(25);

  setStyleSheet("background-color: #2b2b2b; color: #a0a0a0;");
}

void RulerBar::updateCursorPos(const QPoint& pos) {
  if (!m_view)
    return;

  QPointF scenePos = m_view->mapToScene(pos);

  if (m_orientation == Horizontal)
    m_cursorPos = scenePos.x();
  else
    m_cursorPos = scenePos.y();

  // Update Status Bar with integer coordinates
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(window());
  if (mainWindow && mainWindow->statusBar()) {
    QString coordText = QString("X: %1, Y: %2").arg(static_cast<int>(std::round(scenePos.x()))).arg(static_cast<int>(std::round(scenePos.y())));
    mainWindow->statusBar()->showMessage(coordText);
  }

  update();
}

void RulerBar::mouseMoveEvent(QMouseEvent* event) {
  if (m_view) {
    QPoint viewPos;
    if (m_orientation == Horizontal) {
      viewPos = QPoint(event->pos().x(), 0);
    } else {
      viewPos = QPoint(0, event->pos().y());
    }

    // Delegate to shared function to ensure status bar updates
    updateCursorPos(viewPos);
  }

  if (m_dragging) {
    if (m_orientation == Horizontal)
      m_dragPos = event->pos().y();
    else
      m_dragPos = event->pos().x();
    setCursor(Qt::SplitVCursor);
  }

  QWidget::mouseMoveEvent(event);
}

void RulerBar::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_dragging = true;
  }
  QWidget::mousePressEvent(event);
}

void RulerBar::mouseReleaseEvent(QMouseEvent* event) {
  if (m_dragging && m_view) {
    // Condition 1: Dragged out into view
    bool droppedInView = false;
    if (m_orientation == Horizontal && event->pos().y() > height())
      droppedInView = true;
    if (m_orientation == Vertical && event->pos().x() > width())
      droppedInView = true;

    // Condition 2: Simply clicked inside the ruler
    bool clickedInRuler = rect().contains(event->pos());

    if (droppedInView || clickedInRuler) {
      QPointF dropScenePos = m_view->mapToScene(event->pos());

      GuideLineItem::Orientation guideOri;
      qreal pos;

      if (m_orientation == Horizontal) {
        // Top Ruler: measures X axis -> Create Vertical Line
        guideOri = GuideLineItem::Vertical;
        pos = dropScenePos.x();
      } else {
        // Left Ruler: measures Y axis -> Create Horizontal Line
        guideOri = GuideLineItem::Horizontal;
        pos = dropScenePos.y();
      }

      GuideLineItem* guide = new GuideLineItem(guideOri, pos);

      // REVERT: Direct add to scene (No Undo Command)
      if (m_view->scene()) {
        m_view->scene()->addItem(guide);
      }
    }

    m_dragging = false;
    setCursor(Qt::ArrowCursor);
  }
  QWidget::mouseReleaseEvent(event);
}

void RulerBar::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter p(this);
  p.fillRect(rect(), QColor(43, 43, 43));

  p.setPen(QColor(160, 160, 160));
  QFont font = p.font();
  font.setPointSize(7);
  p.setFont(font);

  if (!m_view)
    return;

  QRect viewportRect = m_view->viewport()->rect();
  QPointF topLeft = m_view->mapToScene(viewportRect.topLeft());
  QPointF bottomRight = m_view->mapToScene(viewportRect.bottomRight());

  double start, end;
  double scale = m_view->transform().m11();

  if (m_orientation == Horizontal) {
    start = topLeft.x();
    end = bottomRight.x();
  } else {
    start = topLeft.y();
    end = bottomRight.y();
  }

  double step = 100;
  if (scale > 2.0)
    step = 10;
  else if (scale > 0.8)
    step = 50;
  else if (scale < 0.2)
    step = 500;

  double firstTick = std::floor(start / step) * step;
  int numTicks = std::ceil((end - firstTick) / step);

  for (int i = 0; i <= numTicks; ++i) {
    double v = firstTick + (i * step);
    if (v > end)
      break;

    QPointF widgetPt;
    if (m_orientation == Horizontal)
      widgetPt = m_view->mapFromScene(v, 0);
    else
      widgetPt = m_view->mapFromScene(0, v);

    int pos = (m_orientation == Horizontal) ? widgetPt.x() : widgetPt.y();

    if (m_orientation == Horizontal) {
      p.drawLine(pos, 15, pos, 25);
      p.drawText(pos + 2, 12, QString::number((int)v));
    } else {
      p.drawLine(15, pos, 25, pos);
      p.save();
      p.translate(12, pos + 2);
      p.rotate(-90);
      p.drawText(0, 0, QString::number((int)v));
      p.restore();
    }
  }

  p.setPen(QPen(Qt::red, 1));

  QPointF markerPt;
  if (m_orientation == Horizontal)
    markerPt = m_view->mapFromScene(m_cursorPos, 0);
  else
    markerPt = m_view->mapFromScene(0, m_cursorPos);

  int markerPos = (m_orientation == Horizontal) ? markerPt.x() : markerPt.y();

  if (m_orientation == Horizontal)
    p.drawLine(markerPos, 0, markerPos, 25);
  else
    p.drawLine(0, markerPos, 25, markerPos);
}
