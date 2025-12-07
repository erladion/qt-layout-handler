#ifndef RULERBAR_H
#define RULERBAR_H

#include <QWidget>

class QGraphicsView;

class RulerBar : public QWidget {
  Q_OBJECT

public:
  enum Orientation { Horizontal, Vertical };

  RulerBar(Orientation orientation, QGraphicsView* view, QWidget* parent = nullptr);

  void updateCursorPos(const QPoint& pos);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  Orientation m_orientation;
  QGraphicsView* m_pView;
  double m_cursorPos;  // Position in SCENE coordinates

  // Dragging state for creating guides
  bool m_dragging;
  double m_dragPos;  // Pixel position on the widget
};

#endif  // RULERBAR_H
