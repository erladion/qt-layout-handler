#ifndef DRAGHANDLEWIDGET_H
#define DRAGHANDLEWIDGET_H

#include <QWidget>

class DragHandleWidget : public QWidget {
  Q_OBJECT
public:
  explicit DragHandleWidget(QWidget* parent = nullptr);

protected:
  void paintEvent(QPaintEvent* event) override;
};

#endif  // DRAGHANDLEWIDGET_H
