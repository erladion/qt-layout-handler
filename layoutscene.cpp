#include "layoutscene.h"
#include "guidelineitem.h"
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "snappingutils.h"
#include "zoneitem.h"

#include <QGraphicsRectItem>
#include <QLineF>
#include <QPainter>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>

// =========================================================
// Helper: Filter Alignable Items
// =========================================================

static QList<QGraphicsItem*> getAlignableItems(const QList<QGraphicsItem*>& selection) {
  QList<QGraphicsItem*> valid;
  valid.reserve(selection.size());
  for (auto item : selection) {
    if (SnappingUtils::isSnappableItem(item))
      valid.append(item);
  }
  return valid;
}

// =========================================================
// WorkspaceItem (Internal)
// =========================================================
class WorkspaceItem : public QGraphicsRectItem {
public:
  WorkspaceItem(const QRectF& rect) : QGraphicsRectItem(rect) {
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

    auto drawBar = [&](const QRectF& barRect, const QString& label) {
      painter->fillRect(barRect, QBrush(Qt::black));
      painter->setPen(Qt::white);
      painter->drawText(barRect.adjusted(10, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
    };

    if (topH > 0) {
      drawBar(QRectF(fullRect.left(), fullRect.top(), fullRect.width(), topH), "Top Bar");
    }

    if (botH > 0) {
      drawBar(QRectF(fullRect.left(), fullRect.bottom() - botH, fullRect.width(), botH), "Bottom Bar");
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

  long long startXIndex = std::ceil(workArea.left() / (double)gridSize);
  long long endXIndex = std::floor(workArea.right() / (double)gridSize);

  for (long long i = startXIndex; i <= endXIndex; ++i) {
    qreal x = i * gridSize;
    if (x >= workArea.left() && x <= workArea.right())
      lines.append(QLineF(x, workArea.top(), x, workArea.bottom()));
  }

  long long startYIndex = std::ceil(workArea.top() / (double)gridSize);
  long long endYIndex = std::floor(workArea.bottom() / (double)gridSize);

  for (long long i = startYIndex; i <= endYIndex; ++i) {
    qreal y = i * gridSize;
    if (y >= workArea.top() && y <= workArea.bottom())
      lines.append(QLineF(workArea.left(), y, workArea.right(), y));
  }

  return lines;
}

LayoutScene::LayoutScene(qreal x, qreal y, qreal w, qreal h, QObject* parent)
    : QGraphicsScene(parent), m_gridEnabled(false), m_gridSize(50), m_topBarHeight(0), m_bottomBarHeight(0), m_bgItem(nullptr) {
  setSceneRect(x, y, w, h);
  WorkspaceItem* bg = new WorkspaceItem(QRectF(x, y, w, h));
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
    static_cast<WorkspaceItem*>(m_bgItem)->setWallpaper(pix);
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

// --- ITEM MANAGEMENT ---

void LayoutScene::clearLayout() {
  QList<QGraphicsItem*> allItems = items();
  for (auto item : allItems) {
    // Keep the Workspace/Background item
    if (item == m_bgItem)
      continue;

    // Remove Apps, Zones, Groups, Guides using the shared util check
    if (SnappingUtils::isSnappableItem(item) || dynamic_cast<GuideLineItem*>(item)) {
      removeItem(item);
      delete item;
    }
  }
}

// Centralized Factory for App Items
ResizableAppItem* LayoutScene::addAppItem(const QString& name, const QRectF& rect) {
  ResizableAppItem* item = new ResizableAppItem(name, rect);
  addItem(item);
  return item;
}

// --- ALIGNMENT LOGIC ---

void LayoutScene::alignSelectionLeft() {
  auto validItems = getAlignableItems(selectedItems());
  if (validItems.isEmpty())
    return;

  if (validItems.size() == 1) {
    QRectF safe = getWorkingArea();
    validItems.first()->moveBy(safe.left() - validItems.first()->sceneBoundingRect().left(), 0);
    return;
  }

  qreal minX = std::numeric_limits<qreal>::max();
  for (auto item : validItems)
    minX = std::min(minX, item->sceneBoundingRect().left());

  for (auto item : validItems) {
    item->moveBy(minX - item->sceneBoundingRect().left(), 0);
  }
}

void LayoutScene::alignSelectionRight() {
  auto validItems = getAlignableItems(selectedItems());
  if (validItems.isEmpty())
    return;

  if (validItems.size() == 1) {
    QRectF safe = getWorkingArea();
    validItems.first()->moveBy(safe.right() - validItems.first()->sceneBoundingRect().right(), 0);
    return;
  }

  qreal targetX = -std::numeric_limits<qreal>::max();
  for (auto item : validItems) {
    targetX = std::max(targetX, item->sceneBoundingRect().right());
  }

  for (auto item : validItems) {
    qreal diff = targetX - item->sceneBoundingRect().right();
    item->moveBy(diff, 0);
  }
}

void LayoutScene::alignSelectionTop() {
  auto validItems = getAlignableItems(selectedItems());
  if (validItems.isEmpty())
    return;

  if (validItems.size() == 1) {
    QRectF safe = getWorkingArea();
    validItems.first()->moveBy(0, safe.top() - validItems.first()->sceneBoundingRect().top());
    return;
  }

  qreal minY = std::numeric_limits<qreal>::max();
  for (auto item : validItems)
    minY = std::min(minY, item->sceneBoundingRect().top());

  for (auto item : validItems) {
    qreal diff = minY - item->sceneBoundingRect().top();
    item->moveBy(0, diff);
  }
}

void LayoutScene::alignSelectionBottom() {
  auto validItems = getAlignableItems(selectedItems());
  if (validItems.isEmpty())
    return;

  if (validItems.size() == 1) {
    QRectF safe = getWorkingArea();
    validItems.first()->moveBy(0, safe.bottom() - validItems.first()->sceneBoundingRect().bottom());
    return;
  }

  qreal targetY = -std::numeric_limits<qreal>::max();
  for (auto item : validItems)
    targetY = std::max(targetY, item->sceneBoundingRect().bottom());

  for (auto item : validItems) {
    qreal diff = targetY - item->sceneBoundingRect().bottom();
    item->moveBy(0, diff);
  }
}

void LayoutScene::alignSelectionCenterH() {
  auto validItems = getAlignableItems(selectedItems());
  if (validItems.isEmpty())
    return;

  if (validItems.size() == 1) {
    QRectF safe = getWorkingArea();
    qreal targetCenter = safe.center().x();
    qreal currentCenter = validItems.first()->sceneBoundingRect().center().x();
    validItems.first()->moveBy(targetCenter - currentCenter, 0);
    return;
  }

  QRectF totalRect;
  bool first = true;
  for (auto item : validItems) {
    if (first) {
      totalRect = item->sceneBoundingRect();
      first = false;
    } else
      totalRect = totalRect.united(item->sceneBoundingRect());
  }

  qreal centerX = totalRect.center().x();
  for (auto item : validItems) {
    qreal itemCenter = item->sceneBoundingRect().center().x();
    item->moveBy(centerX - itemCenter, 0);
  }
}

void LayoutScene::alignSelectionCenterV() {
  auto validItems = getAlignableItems(selectedItems());
  if (validItems.isEmpty())
    return;

  if (validItems.size() == 1) {
    QRectF safe = getWorkingArea();
    qreal targetCenter = safe.center().y();
    qreal currentCenter = validItems.first()->sceneBoundingRect().center().y();
    validItems.first()->moveBy(0, targetCenter - currentCenter);
    return;
  }

  QRectF totalRect;
  bool first = true;
  for (auto item : validItems) {
    if (first) {
      totalRect = item->sceneBoundingRect();
      first = false;
    } else
      totalRect = totalRect.united(item->sceneBoundingRect());
  }

  qreal centerY = totalRect.center().y();
  for (auto item : validItems) {
    qreal itemCenter = item->sceneBoundingRect().center().y();
    item->moveBy(0, centerY - itemCenter);
  }
}

void LayoutScene::distributeSelectionH() {
  auto sel = getAlignableItems(selectedItems());
  if (sel.size() < 3)
    return;

  std::sort(sel.begin(), sel.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->sceneBoundingRect().x() < b->sceneBoundingRect().x(); });

  QGraphicsItem* first = sel.first();
  QGraphicsItem* last = sel.last();

  qreal totalItemWidth = 0;
  for (auto item : sel)
    totalItemWidth += item->sceneBoundingRect().width();

  qreal startX = first->sceneBoundingRect().left();
  qreal endX = last->sceneBoundingRect().right();
  qreal totalSpan = endX - startX;

  qreal totalGapSpace = totalSpan - totalItemWidth;
  qreal gap = totalGapSpace / (sel.size() - 1);

  qreal currentX = startX;
  for (auto item : sel) {
    qreal currentItemX = item->sceneBoundingRect().x();
    item->moveBy(currentX - currentItemX, 0);
    currentX += item->sceneBoundingRect().width() + gap;
  }
}

void LayoutScene::distributeSelectionV() {
  auto sel = getAlignableItems(selectedItems());
  if (sel.size() < 3)
    return;

  std::sort(sel.begin(), sel.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->sceneBoundingRect().y() < b->sceneBoundingRect().y(); });

  QGraphicsItem* first = sel.first();
  QGraphicsItem* last = sel.last();

  qreal totalItemHeight = 0;
  for (auto item : sel)
    totalItemHeight += item->sceneBoundingRect().height();

  qreal startY = first->sceneBoundingRect().top();
  qreal endY = last->sceneBoundingRect().bottom();
  qreal totalSpan = endY - startY;

  qreal totalGapSpace = totalSpan - totalItemHeight;
  qreal gap = totalGapSpace / (sel.size() - 1);

  qreal currentY = startY;
  for (auto item : sel) {
    qreal currentItemY = item->sceneBoundingRect().y();
    item->moveBy(0, currentY - currentItemY);
    currentY += item->sceneBoundingRect().height() + gap;
  }
}
