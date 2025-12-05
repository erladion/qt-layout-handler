#include "layoutscene.h"
#include <QGraphicsRectItem>
#include <QLineF>
#include <QPainter>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
#include <cmath>

// =========================================================
// ArtboardItem
// =========================================================
class ArtboardItem : public QGraphicsRectItem {
public:
  ArtboardItem(const QRectF& rect) : QGraphicsRectItem(rect) {
    setZValue(-1000);
    setAcceptedMouseButtons(Qt::NoButton);
    setCacheMode(QGraphicsItem::NoCache);
  }

  void setWallpaper(const QPixmap& pix) {
    m_wallpaper = pix;
    update();  // Trigger repaint
  }

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    if (!layoutScene)
      return;

    QRectF fullRect = rect();

    // 1. Draw Background / Wallpaper
    // We draw this FIRST so everything else sits on top.
    if (!m_wallpaper.isNull()) {
      // Draw the pixmap scaled to fill the entire artboard rect
      painter->drawPixmap(fullRect, m_wallpaper, m_wallpaper.rect());

      // Optional: Add a semi-transparent dark overlay to ensure white text/lines remain visible
      // painter->fillRect(fullRect, QColor(0, 0, 0, 50));
    } else {
      // Default grey background
      painter->fillRect(fullRect, QBrush(QColor(80, 80, 80)));
    }

    // 2. Draw Grid (From Cache)
    if (layoutScene->isGridEnabled()) {
      QPen pen(QColor(110, 110, 110));
      pen.setWidth(0);
      pen.setCosmetic(true);
      painter->setPen(pen);
      painter->drawLines(layoutScene->gridLines());
    }

    // 3. Draw System Bars
    int topH = layoutScene->topBarHeight();
    int botH = layoutScene->bottomBarHeight();

    if (topH > 0) {
      QRectF topBar(fullRect.left(), fullRect.top(), fullRect.width(), topH);
      painter->fillRect(topBar, QBrush(Qt::black));
      painter->setPen(Qt::white);
      painter->drawText(topBar.adjusted(10, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, "Top Bar");
    }

    if (botH > 0) {
      QRectF botBar(fullRect.left(), fullRect.bottom() - botH, fullRect.width(), botH);
      painter->fillRect(botBar, QBrush(Qt::black));
      painter->setPen(Qt::white);
      painter->drawText(botBar.adjusted(10, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, "Bottom Bar");
    }

    // 4. Border
    QPen borderPen(Qt::white, 2);
    borderPen.setCosmetic(true);
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(fullRect);
  }

private:
  QPixmap m_wallpaper;
};

// =========================================================
// LayoutScene Implementation
// =========================================================

static QVector<QLineF> calculateGridLines(int gridSize, QRectF workArea) {
  QVector<QLineF> lines;
  int estimated = (workArea.width() / gridSize) + (workArea.height() / gridSize) + 2;
  lines.reserve(estimated);

  qreal startX = std::round(workArea.left() / gridSize) * gridSize;
  for (qreal x = startX; x <= workArea.right(); x += gridSize) {
    if (x >= workArea.left())
      lines.append(QLineF(x, workArea.top(), x, workArea.bottom()));
  }

  qreal startY = std::round(workArea.top() / gridSize) * gridSize;
  for (qreal y = startY; y <= workArea.bottom(); y += gridSize) {
    if (y >= workArea.top())
      lines.append(QLineF(workArea.left(), y, workArea.right(), y));
  }
  return lines;
}

LayoutScene::LayoutScene(qreal x, qreal y, qreal w, qreal h, QObject* parent)
    : QGraphicsScene(parent), m_gridEnabled(false), m_gridSize(50), m_topBarHeight(0), m_bottomBarHeight(0), m_bgItem(nullptr) {
  setSceneRect(x, y, w, h);
  ArtboardItem* bg = new ArtboardItem(QRectF(x, y, w, h));
  addItem(bg);
  m_bgItem = bg;

  connect(&m_gridWatcher, &QFutureWatcher<QVector<QLineF>>::finished, this, &LayoutScene::onGridCalculationFinished);
}

bool LayoutScene::isGridEnabled() const {
  return m_gridEnabled;
}
int LayoutScene::gridSize() const {
  return m_gridSize;
}

void LayoutScene::setGridEnabled(bool enabled) {
  m_gridEnabled = enabled;
  triggerGridUpdate();
}

void LayoutScene::setGridSize(int size) {
  m_gridSize = size;
  triggerGridUpdate();
}

void LayoutScene::setTopBarHeight(int h) {
  m_topBarHeight = h;
  triggerGridUpdate();
}

void LayoutScene::setBottomBarHeight(int h) {
  m_bottomBarHeight = h;
  triggerGridUpdate();
}

// FIX: New Method to pass wallpaper to ArtboardItem
void LayoutScene::setWallpaper(const QPixmap& pix) {
  if (m_bgItem) {
    // Safe cast because we know we created it as ArtboardItem
    static_cast<ArtboardItem*>(m_bgItem)->setWallpaper(pix);
  }
}

QRectF LayoutScene::getWorkingArea() const {
  return QRectF(sceneRect().left(), sceneRect().top() + m_topBarHeight, sceneRect().width(),
                sceneRect().height() - m_topBarHeight - m_bottomBarHeight);
}

void LayoutScene::triggerGridUpdate() {
  if (m_gridEnabled) {
    QFuture<QVector<QLineF>> future = QtConcurrent::run(calculateGridLines, m_gridSize, getWorkingArea());
    m_gridWatcher.setFuture(future);
  } else {
    m_cachedGridLines.clear();
    if (m_bgItem)
      m_bgItem->update();
    update();
  }
}

void LayoutScene::onGridCalculationFinished() {
  m_cachedGridLines = m_gridWatcher.result();
  if (m_bgItem)
    m_bgItem->update();
  update();
}

void LayoutScene::drawBackground(QPainter* painter, const QRectF& rect) {
  Q_UNUSED(painter);
  Q_UNUSED(rect);
}
