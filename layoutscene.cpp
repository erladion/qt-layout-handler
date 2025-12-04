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

    // PERFORMANCE FIX: Disable Caching.
    // Caching a large item (1920x1080) causes lag during window resizing
    // because the CPU has to rescale the large cached image every frame.
    // Drawing simple primitives (lines/rects) directly is much faster here.
    setCacheMode(QGraphicsItem::NoCache);
  }

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    if (!layoutScene)
      return;

    QRectF fullRect = rect();

    // 1. Fill Artboard
    painter->fillRect(fullRect, QBrush(QColor(80, 80, 80)));

    // 2. Draw Grid (From Cache)
    if (layoutScene->isGridEnabled()) {
      QPen pen(QColor(110, 110, 110));

      // PERFORMANCE FIX: Cosmetic Pen
      // A width of 0 or cosmetic=true tells the painter to ignore
      // matrix transformations for width. This is significantly faster
      // to render during zooms/resizes.
      pen.setWidth(0);  // 0 means "1 pixel cosmetic" in Qt
      pen.setCosmetic(true);

      painter->setPen(pen);

      // FAST: Drawing pre-calculated lines from memory
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
};

// =========================================================
// LayoutScene Implementation
// =========================================================

// Static helper function for the thread (must be pure calculation)
static QVector<QLineF> calculateGridLines(int gridSize, QRectF workArea) {
  QVector<QLineF> lines;

  // Estimate size to prevent reallocations
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

  // Connect the thread watcher
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

QRectF LayoutScene::getWorkingArea() const {
  return QRectF(sceneRect().left(), sceneRect().top() + m_topBarHeight, sceneRect().width(),
                sceneRect().height() - m_topBarHeight - m_bottomBarHeight);
}

// Spawns the background thread
void LayoutScene::triggerGridUpdate() {
  if (m_gridEnabled) {
    // Run calculation in background thread
    QFuture<QVector<QLineF>> future = QtConcurrent::run(calculateGridLines, m_gridSize, getWorkingArea());
    m_gridWatcher.setFuture(future);
  } else {
    // If disabled, just clear immediately
    m_cachedGridLines.clear();
    if (m_bgItem)
      m_bgItem->update();
    update();
  }
}

// Called on Main Thread when calculation is done
void LayoutScene::onGridCalculationFinished() {
  m_cachedGridLines = m_gridWatcher.result();

  // Force the Artboard item to update its cache
  if (m_bgItem)
    m_bgItem->update();
  update();
}

void LayoutScene::drawBackground(QPainter* painter, const QRectF& rect) {
  Q_UNUSED(painter);
  Q_UNUSED(rect);
}
