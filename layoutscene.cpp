#include "layoutscene.h"
#include "resizableappitem.h"
#include "snappingitemgroup.h"  // Needed for casting checks
#include "zoneitem.h"

#include <QGraphicsRectItem>
#include <QLineF>
#include <QPainter>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>  // for std::sort, std::min, std::max
#include <cmath>

// =========================================================
// ArtboardItem (Internal)
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
    update();
  }

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    if (!layoutScene)
      return;

    QRectF fullRect = rect();

    // 1. Draw Background
    if (!m_wallpaper.isNull()) {
      painter->drawPixmap(fullRect, m_wallpaper, m_wallpaper.rect());
    } else {
      painter->fillRect(fullRect, QBrush(QColor(80, 80, 80)));
    }

    // 2. Draw Grid
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

void LayoutScene::setWallpaper(const QPixmap& pix) {
  if (m_bgItem) {
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

// --- REFACTORED ALIGNMENT LOGIC ---

// Helper to check if an item is a valid target for alignment (App, Zone, or Group)
static bool isValidItem(QGraphicsItem* item) {
  return (dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item) || dynamic_cast<SnappingItemGroup*>(item));
}

void LayoutScene::alignSelectionLeft() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.isEmpty())
    return;

  if (sel.size() == 1) {
    if (isValidItem(sel.first())) {
      QRectF safe = getWorkingArea();
      sel.first()->moveBy(safe.left() - sel.first()->sceneBoundingRect().left(), 0);
    }
    return;
  }

  qreal minX = std::numeric_limits<qreal>::max();
  for (auto item : sel)
    if (isValidItem(item))
      minX = std::min(minX, item->pos().x());
  for (auto item : sel)
    if (isValidItem(item))
      item->setPos(minX, item->pos().y());
}

void LayoutScene::alignSelectionRight() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.isEmpty())
    return;

  if (sel.size() == 1) {
    if (isValidItem(sel.first())) {
      QRectF safe = getWorkingArea();
      sel.first()->moveBy(safe.right() - sel.first()->sceneBoundingRect().right(), 0);
    }
    return;
  }

  qreal maxRight = -std::numeric_limits<qreal>::max();
  for (auto item : sel)
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      maxRight = std::max(maxRight, app->pos().x() + app->rect().width());
    else if (auto grp = dynamic_cast<SnappingItemGroup*>(item))
      maxRight = std::max(maxRight, grp->pos().x() + grp->boundingRect().width());  // Simplified

  // To be precise with mixed types, we should use sceneBoundingRect for calculation but setPos for movement.
  // However, simpler logic for "Right Align" usually aligns by RIGHT EDGE.
  // Re-implementing using bounding rects for robustness:
  qreal targetX = -std::numeric_limits<qreal>::max();
  for (auto item : sel) {
    if (isValidItem(item))
      targetX = std::max(targetX, item->sceneBoundingRect().right());
  }

  for (auto item : sel) {
    if (isValidItem(item)) {
      // New X = Target Right Edge - Item Width
      // However, item->pos() is local.
      // MoveBy is safer.
      qreal diff = targetX - item->sceneBoundingRect().right();
      item->moveBy(diff, 0);
    }
  }
}

void LayoutScene::alignSelectionTop() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.isEmpty())
    return;

  if (sel.size() == 1) {
    if (isValidItem(sel.first())) {
      QRectF safe = getWorkingArea();
      sel.first()->moveBy(0, safe.top() - sel.first()->sceneBoundingRect().top());
    }
    return;
  }

  qreal minY = std::numeric_limits<qreal>::max();
  for (auto item : sel)
    if (isValidItem(item))
      minY = std::min(minY, item->sceneBoundingRect().top());

  for (auto item : sel) {
    if (isValidItem(item)) {
      qreal diff = minY - item->sceneBoundingRect().top();
      item->moveBy(0, diff);
    }
  }
}

void LayoutScene::alignSelectionBottom() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.isEmpty())
    return;

  if (sel.size() == 1) {
    if (isValidItem(sel.first())) {
      QRectF safe = getWorkingArea();
      sel.first()->moveBy(0, safe.bottom() - sel.first()->sceneBoundingRect().bottom());
    }
    return;
  }

  qreal targetY = -std::numeric_limits<qreal>::max();
  for (auto item : sel)
    if (isValidItem(item))
      targetY = std::max(targetY, item->sceneBoundingRect().bottom());

  for (auto item : sel) {
    if (isValidItem(item)) {
      qreal diff = targetY - item->sceneBoundingRect().bottom();
      item->moveBy(0, diff);
    }
  }
}

void LayoutScene::alignSelectionCenterH() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.isEmpty())
    return;

  if (sel.size() == 1) {
    if (isValidItem(sel.first())) {
      QRectF safe = getWorkingArea();
      qreal targetCenter = safe.center().x();
      qreal currentCenter = sel.first()->sceneBoundingRect().center().x();
      sel.first()->moveBy(targetCenter - currentCenter, 0);
    }
    return;
  }

  QRectF totalRect;
  bool first = true;
  for (auto item : sel) {
    if (isValidItem(item)) {
      if (first) {
        totalRect = item->sceneBoundingRect();
        first = false;
      } else
        totalRect = totalRect.united(item->sceneBoundingRect());
    }
  }
  qreal centerX = totalRect.center().x();
  for (auto item : sel)
    if (isValidItem(item)) {
      qreal itemCenter = item->sceneBoundingRect().center().x();
      item->moveBy(centerX - itemCenter, 0);
    }
}

void LayoutScene::alignSelectionCenterV() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.isEmpty())
    return;

  if (sel.size() == 1) {
    if (isValidItem(sel.first())) {
      QRectF safe = getWorkingArea();
      qreal targetCenter = safe.center().y();
      qreal currentCenter = sel.first()->sceneBoundingRect().center().y();
      sel.first()->moveBy(0, targetCenter - currentCenter);
    }
    return;
  }

  QRectF totalRect;
  bool first = true;
  for (auto item : sel) {
    if (isValidItem(item)) {
      if (first) {
        totalRect = item->sceneBoundingRect();
        first = false;
      } else
        totalRect = totalRect.united(item->sceneBoundingRect());
    }
  }
  qreal centerY = totalRect.center().y();
  for (auto item : sel)
    if (isValidItem(item)) {
      qreal itemCenter = item->sceneBoundingRect().center().y();
      item->moveBy(0, centerY - itemCenter);
    }
}

void LayoutScene::distributeSelectionH() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.size() < 3)
    return;

  // Sort by visual X position
  std::sort(sel.begin(), sel.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->sceneBoundingRect().x() < b->sceneBoundingRect().x(); });

  QGraphicsItem* first = sel.first();
  QGraphicsItem* last = sel.last();

  if (!isValidItem(first) || !isValidItem(last))
    return;

  qreal totalItemWidth = 0;
  for (auto item : sel)
    if (isValidItem(item))
      totalItemWidth += item->sceneBoundingRect().width();

  qreal startX = first->sceneBoundingRect().left();
  qreal endX = last->sceneBoundingRect().right();
  qreal totalSpan = endX - startX;

  // Calculate gap
  // Gap = (TotalSpan - SumOfWidths) / (Count - 1)
  // NOTE: This logic assumes we want equal GAPS between items, not equal centers.
  // The original code calculated gaps slightly differently.
  // Standard distribution usually distributes CENTERS or GAPS.
  // Let's stick to standard "Distribute Centers" if they are same size, or Gaps if not.
  // Let's implement "Distribute Horizontally" (Equal Gaps).

  qreal totalGapSpace = totalSpan - totalItemWidth;
  qreal gap = totalGapSpace / (sel.size() - 1);

  qreal currentX = startX;
  for (auto item : sel) {
    if (isValidItem(item)) {
      // Move item to currentX
      qreal currentItemX = item->sceneBoundingRect().x();
      item->moveBy(currentX - currentItemX, 0);

      currentX += item->sceneBoundingRect().width() + gap;
    }
  }
}

void LayoutScene::distributeSelectionV() {
  QList<QGraphicsItem*> sel = selectedItems();
  if (sel.size() < 3)
    return;

  std::sort(sel.begin(), sel.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->sceneBoundingRect().y() < b->sceneBoundingRect().y(); });

  QGraphicsItem* first = sel.first();
  QGraphicsItem* last = sel.last();

  if (!isValidItem(first) || !isValidItem(last))
    return;

  qreal totalItemHeight = 0;
  for (auto item : sel)
    if (isValidItem(item))
      totalItemHeight += item->sceneBoundingRect().height();

  qreal startY = first->sceneBoundingRect().top();
  qreal endY = last->sceneBoundingRect().bottom();
  qreal totalSpan = endY - startY;

  qreal totalGapSpace = totalSpan - totalItemHeight;
  qreal gap = totalGapSpace / (sel.size() - 1);

  qreal currentY = startY;
  for (auto item : sel) {
    if (isValidItem(item)) {
      qreal currentItemY = item->sceneBoundingRect().y();
      item->moveBy(0, currentY - currentItemY);

      currentY += item->sceneBoundingRect().height() + gap;
    }
  }
}
