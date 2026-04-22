#ifndef PROJECTORWINDOW_H
#define PROJECTORWINDOW_H

#include <QGraphicsScene>
#include <QPointer>
#include <QWidget>

class ProjectorWindow : public QWidget {
  Q_OBJECT
public:
  explicit ProjectorWindow(QGraphicsScene* scene, QWidget* parent = nullptr);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  QPointer<QGraphicsScene> m_pScene;
};

#endif  // PROJECTORWINDOW_H
