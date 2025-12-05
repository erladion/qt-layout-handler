#include "rulerbar.h"
#include <QGraphicsView>
#include <QPainter>
#include <QScrollBar>
#include <cmath>

RulerBar::RulerBar(Orientation orientation, QGraphicsView* view, QWidget* parent) : QWidget(parent), m_orientation(orientation), m_view(view) {
  // Fixed size for the ruler thickness
  if (m_orientation == Horizontal) {
    setFixedHeight(25);
  } else {
    setFixedWidth(25);
  }
}

void RulerBar::updateCursorPos(const QPoint& pos) {
  m_lastMousePos = pos;
  update();  // Redraw to move the marker
}

void RulerBar::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.fillRect(rect(), QColor(50, 50, 50));  // Background

  // Get Viewport Transform to map pixels to scene units
  QTransform t = m_view->viewportTransform();

  // Scale factor (how many pixels is 1 scene unit?)
  // Horizontal scale is m11, Vertical is m22
  double scale = (m_orientation == Horizontal) ? t.m11() : t.m22();

  // Offset (where does the scene start relative to viewport 0?)
  // dx is horizontal translation, dy is vertical
  double offset = (m_orientation == Horizontal) ? t.dx() : t.dy();

  // Calculate start and end in SCENE coordinates visible on this ruler
  // We Map the widget pixels (0 to width) back to scene coords
  // Formula: sceneCoord = (widgetCoord - offset) / scale
  double startScene = (0 - offset) / scale;
  double endScene = ((m_orientation == Horizontal ? width() : height()) - offset) / scale;

  // Define Tick Interval based on Zoom level
  // We want ticks roughly every 50-100 visual pixels
  double visualInterval = 50.0;
  double logicalInterval = visualInterval / scale;

  // Round interval to nice numbers (10, 50, 100, 500, etc.)
  double magnitude = std::pow(10, std::floor(std::log10(logicalInterval)));
  if (logicalInterval / magnitude < 2)
    logicalInterval = 2 * magnitude;
  else if (logicalInterval / magnitude < 5)
    logicalInterval = 5 * magnitude;
  else
    logicalInterval = 10 * magnitude;

  // Align start to the grid
  double startTick = std::floor(startScene / logicalInterval) * logicalInterval;

  // DRAW TICKS
  painter.setPen(QColor(180, 180, 180));
  QFont font = painter.font();
  font.setPointSize(8);
  painter.setFont(font);

  for (double current = startTick; current <= endScene; current += logicalInterval) {
    // Convert back to Widget pixels to draw
    double pixelPos = (current * scale) + offset;

    if (m_orientation == Horizontal) {
      // Major Tick
      painter.drawLine(QPointF(pixelPos, 15), QPointF(pixelPos, 25));
      // Text
      painter.drawText(QRectF(pixelPos + 2, 0, 50, 15), Qt::AlignLeft | Qt::AlignTop, QString::number((int)current));

      // Minor Ticks (4 subdivisions)
      double minorStep = logicalInterval / 5.0;
      for (int i = 1; i < 5; i++) {
        double minorPos = ((current + i * minorStep) * scale) + offset;
        painter.drawLine(QPointF(minorPos, 20), QPointF(minorPos, 25));
      }
    } else {
      // Major Tick
      painter.drawLine(QPointF(15, pixelPos), QPointF(25, pixelPos));
      // Text (Rotated for vertical ruler looks cool, but horizontal text is easier to read)
      // Let's draw horizontal text sideways
      painter.save();
      painter.translate(12, pixelPos + 2);
      painter.rotate(90);
      painter.drawText(0, 0, QString::number((int)current));
      painter.restore();

      // Minor Ticks
      double minorStep = logicalInterval / 5.0;
      for (int i = 1; i < 5; i++) {
        double minorPos = ((current + i * minorStep) * scale) + offset;
        painter.drawLine(QPointF(20, minorPos), QPointF(25, minorPos));
      }
    }
  }

  // DRAW CURSOR MARKER
  painter.setPen(QPen(Qt::red, 1));
  if (m_orientation == Horizontal) {
    painter.drawLine(m_lastMousePos.x(), 0, m_lastMousePos.x(), height());
  } else {
    painter.drawLine(0, m_lastMousePos.y(), width(), m_lastMousePos.y());
  }
}
