#include "draghandlewidget.h"
#include <QPainter>

DragHandleWidget::DragHandleWidget(QWidget* parent) : QWidget(parent) {
  setCursor(Qt::SizeAllCursor);
  setFixedHeight(12);  // Give it a fixed height so it sits nicely at the top
}

void DragHandleWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Use a subtle grey color for the grip dots
  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(136, 136, 136));  // #888888

  constexpr int dotSize = 2;
  constexpr int spacing = 3;
  constexpr int rows = 2;
  constexpr int cols = 4;

  // Calculate total width and height of the dot group to center it
  constexpr int totalWidth = cols * dotSize + (cols - 1) * spacing;
  constexpr int totalHeight = rows * dotSize + (rows - 1) * spacing;

  const int startX = (width() - totalWidth) / 2;
  const int startY = (height() - totalHeight) / 2;

  // Draw the grid of dots
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      painter.drawEllipse(startX + c * (dotSize + spacing), startY + r * (dotSize + spacing), dotSize, dotSize);
    }
  }
}
