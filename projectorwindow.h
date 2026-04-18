#ifndef PROJECTORWINDOW_H
#define PROJECTORWINDOW_H

#include <QGraphicsScene>
#include <QTimer>
#include <QWidget>

class ProjectorWindow : public QWidget {
  Q_OBJECT
public:
  explicit ProjectorWindow(QGraphicsScene* scene, QWidget* parent = nullptr);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  QGraphicsScene* m_pScene;
  QTimer* m_pRenderTimer;
};

#endif  // PROJECTORWINDOW_H
