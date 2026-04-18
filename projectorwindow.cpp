#include "projectorwindow.h"
#include <QPainter>

ProjectorWindow::ProjectorWindow(QGraphicsScene* scene, QWidget* parent) : QWidget(parent), m_pScene(scene) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle("Layout Output");

  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor("#1e1e1e"));  // Black/Dark background
  setAutoFillBackground(true);
  setPalette(pal);

  m_pRenderTimer = new QTimer(this);
  connect(m_pRenderTimer, &QTimer::timeout, this, [this]() { this->update(); });
  m_pRenderTimer->start(33);  // 30 FPS Render Loop
}

void ProjectorWindow::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);
  if (!m_pScene)
    return;

  QPainter painter(this);
  // Wipe background to prevent ghosting on different aspect ratios
  painter.fillRect(this->rect(), Qt::black);

  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

  m_pScene->render(&painter, this->rect(), m_pScene->sceneRect(), Qt::KeepAspectRatio);
}
