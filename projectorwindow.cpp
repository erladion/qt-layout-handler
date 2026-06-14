#include "projectorwindow.h"
#include <QPainter>

#include "constants.h"

ProjectorWindow::ProjectorWindow(QGraphicsScene* scene, QWidget* parent) : QWidget(parent) {
  setWindowTitle("Layout Output");
  setAttribute(Qt::WA_DeleteOnClose);

  setScene(scene);
}

void ProjectorWindow::setScene(QGraphicsScene* scene) {
  if (m_pScene == scene) {
    return;
  }

  if (m_pScene) {
    m_pScene->disconnect(this);
  }
  m_pScene = scene;
  if (m_pScene) {
    // Only request a repaint when an item moves, video updates, or laser is drawn.
    connect(m_pScene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) { update(); });
  }
  update();
}

void ProjectorWindow::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);
  if (!m_pScene) {
    return;
  }

  QPainter painter;
  // Check if the window is in a valid state to be painted
  if (!painter.begin(this)) {
    return;  // The window is likely shutting down or minimized. Bail out safely.
  }

  // Fill the output with the work-area colour so the background (and any
  // letterbox bars from aspect-fitting) matches the editor instead of being
  // transparent.
  QColor bg = QColor::fromRgba(Constants::Color::WorkspaceFill);
  bg.setAlpha(255);
  painter.fillRect(rect(), bg);

  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

  m_pScene->render(&painter, rect(), m_pScene->sceneRect(), Qt::KeepAspectRatio);

  painter.end();
}
