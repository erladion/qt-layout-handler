#ifndef RULERBAR_H
#define RULERBAR_H

#include <QWidget>

class QGraphicsView;

class RulerBar : public QWidget {
  Q_OBJECT
public:
  enum Orientation { Horizontal, Vertical };

  explicit RulerBar(Orientation orientation, QGraphicsView* view, QWidget* parent = nullptr);

  // Updates the visual cursor marker on the ruler
  void updateCursorPos(const QPoint& pos);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  Orientation m_orientation;
  QGraphicsView* m_view;
  QPoint m_lastMousePos;

  void drawTicker(QPainter& painter);
};

#endif  // RULERBAR_H
