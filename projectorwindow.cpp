#include "projectorwindow.h"
#include <QPainter>

ProjectorWindow::ProjectorWindow(QGraphicsScene* scene, QWidget* parent) : QWidget(parent), m_pScene(scene) {
  setWindowTitle("Layout Output");
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_TranslucentBackground);

  connect(m_pScene, &QGraphicsScene::changed, this, [this](const QList<QRectF>& region) {
    // Only request a repaint when an item moves, video updates, or laser is drawn
    this->update();
  });
}

void ProjectorWindow::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);
  if (!m_pScene) {
    return;
  }

  QPainter painter(this);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.fillRect(rect(), Qt::transparent);
  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

  m_pScene->render(&painter, rect(), m_pScene->sceneRect(), Qt::KeepAspectRatio);
}
