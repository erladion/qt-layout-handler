#include "layoutscene.h"
#include "constants.h"
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "snappingutils.h"

#include <QGraphicsRectItem>
#include <QImage>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>

static QList<QGraphicsItem*> getAlignableItems(const QList<QGraphicsItem*>& selection) {
  QList<QGraphicsItem*> valid;
  valid.reserve(selection.size());
  for (auto item : selection) {
    if (SnappingUtils::isSnappableItem(item) || item->type() == Constants::Item::GuideItem) {
      valid.append(item);
    }
  }
  return valid;
}

class WorkspaceItem : public QGraphicsRectItem {
public:
  WorkspaceItem(const QRectF& rect) : QGraphicsRectItem(rect) {
    setZValue(-1000);
    setAcceptedMouseButtons(Qt::NoButton);
    setCacheMode(QGraphicsItem::NoCache);
  }

  int type() const override { return Constants::Item::Workspace; }

  void setWallpaper(const QPixmap& pix) {
    m_wallpaper = pix;
    update();
  }

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
    Q_UNUSED(option);

    if (widget == nullptr) {
      return;  // Skip drawing the background, grid, and bars entirely!
    }

    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    if (!layoutScene) {
      return;
    }

    QRectF fullRect = rect();

    if (!m_wallpaper.isNull()) {
      painter->drawPixmap(fullRect, m_wallpaper, m_wallpaper.rect());
    } else {
      painter->fillRect(fullRect, QBrush(QColor::fromRgba(Constants::Color::WorkspaceFill)));
    }

    if (layoutScene->isGridEnabled()) {
      QPen pen(QColor::fromRgba(Constants::Color::GridLines));
      pen.setWidth(0);
      pen.setCosmetic(true);
      painter->save();
      painter->setPen(pen);
      painter->drawLines(layoutScene->gridLines());
      painter->restore();
    }

    int topH = layoutScene->topBarHeight();
    int botH = layoutScene->bottomBarHeight();

    auto drawBar = [&](const QRectF& barRect, const QString& label) {
      painter->save();
      painter->fillRect(barRect, QBrush(QColor::fromRgba(Constants::Color::SystemBars)));
      painter->setPen(QColor::fromRgba(Constants::Color::SystemBarText));

      QFont font = painter->font();
      int pixelSize = qMax(10, static_cast<int>(barRect.height() * 0.5));
      font.setPixelSize(pixelSize);
      painter->setFont(font);

      painter->drawText(barRect.adjusted(10, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
      painter->restore();
    };

    if (topH > 0) {
      drawBar(QRectF(fullRect.left(), fullRect.top(), fullRect.width(), topH), "Top Bar");
    }

    if (botH > 0) {
      drawBar(QRectF(fullRect.left(), fullRect.bottom() - botH, fullRect.width(), botH), "Bottom Bar");
    }

    QPen borderPen(Qt::white, 2);
    borderPen.setCosmetic(true);
    painter->save();
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(fullRect);
    painter->restore();
  }

private:
  QPixmap m_wallpaper;
};

static QVector<QLineF> calculateGridLines(int gridSize, QRectF workArea) {
  QVector<QLineF> lines;
  int estimated = (workArea.width() / gridSize) + (workArea.height() / gridSize) + 2;
  lines.reserve(estimated);

  long long startXIndex = std::ceil(workArea.left() / (double)gridSize);
  long long endXIndex = std::floor(workArea.right() / (double)gridSize);

  for (long long i = startXIndex; i <= endXIndex; ++i) {
    qreal x = i * gridSize;
    if (x >= workArea.left() && x <= workArea.right()) {
      lines.append(QLineF(x, workArea.top(), x, workArea.bottom()));
    }
  }

  long long startYIndex = std::ceil(workArea.top() / (double)gridSize);
  long long endYIndex = std::floor(workArea.bottom() / (double)gridSize);

  for (long long i = startYIndex; i <= endYIndex; ++i) {
    qreal y = i * gridSize;
    if (y >= workArea.top() && y <= workArea.bottom()) {
      lines.append(QLineF(workArea.left(), y, workArea.right(), y));
    }
  }

  return lines;
}

LayoutScene::LayoutScene(qreal x, qreal y, qreal w, qreal h, QObject* parent)
    : QGraphicsScene(parent), m_gridEnabled(false), m_gridSize(Constants::DefaultGridSize), m_topBarHeight(0), m_bottomBarHeight(0),
      m_pBgItem(nullptr) {
  setSceneRect(x, y, w, h);
  WorkspaceItem* bg = new WorkspaceItem(QRectF(x, y, w, h));
  addItem(bg);
  m_pBgItem = bg;

  connect(&m_gridWatcher, &QFutureWatcher<QVector<QLineF>>::finished, this, &LayoutScene::onGridCalculationFinished);

  connect(&m_laserTimer, &QTimer::timeout, this, &LayoutScene::fadeLaserTrail);
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
  if (m_pBgItem) {
    static_cast<WorkspaceItem*>(m_pBgItem)->setWallpaper(pix);
  }
}

QRectF LayoutScene::getWorkingArea() const {
  return QRectF(sceneRect().left(), sceneRect().top() + m_topBarHeight, sceneRect().width(),
                sceneRect().height() - m_topBarHeight - m_bottomBarHeight);
}

void LayoutScene::setSnapGuides(const QList<QLineF>& guides) {
  m_snapGuides = guides;
  update();
}

void LayoutScene::clearSnapGuides() {
  if (!m_snapGuides.isEmpty()) {
    m_snapGuides.clear();
    update();
  }
}

void LayoutScene::triggerGridUpdate() {
  if (m_gridEnabled) {
    QFuture<QVector<QLineF>> future = QtConcurrent::run(calculateGridLines, m_gridSize, getWorkingArea());
    m_gridWatcher.setFuture(future);
  } else {
    m_cachedGridLines.clear();
    if (m_pBgItem) {
      m_pBgItem->update();
    }
    update();
  }
}

void LayoutScene::applyAlignment(std::function<qreal(const QRectF&)> getEdge, bool isHorizontal) {
  auto validItems = getAlignableItems(selectedItems());
  if (validItems.isEmpty()) {
    return;
  }

  qreal targetValue;

  // 1. Determine the target edge/center to align to
  if (validItems.size() == 1) {
    // Single item snaps to the workspace bounds
    targetValue = getEdge(getWorkingArea());
  } else {
    // Multiple items snap to the outer bounds of their united group
    QRectF totalRect;
    bool first = true;
    for (auto item : std::as_const(validItems)) {
      if (first) {
        totalRect = item->sceneBoundingRect();
        first = false;
      } else {
        totalRect = totalRect.united(item->sceneBoundingRect());
      }
    }
    targetValue = getEdge(totalRect);
  }

  // 2. Apply the calculated difference to every valid item
  for (auto item : std::as_const(validItems)) {
    qreal diff = targetValue - getEdge(item->sceneBoundingRect());
    if (isHorizontal) {
      item->moveBy(diff, 0);
    } else {
      item->moveBy(0, diff);
    }
  }
}

void LayoutScene::onGridCalculationFinished() {
  m_cachedGridLines = m_gridWatcher.result();
  if (m_pBgItem) {
    m_pBgItem->update();
  }

  update();
}

void LayoutScene::drawForeground(QPainter* painter, const QRectF& rect) {
  Q_UNUSED(rect);

  // 1. Draw Snap Guides
  if (!m_snapGuides.isEmpty()) {
    painter->save();

    QPen pen(QColor::fromRgba(Constants::Color::SmartSnapGuide), 1, Qt::DashLine);
    painter->setPen(pen);

    for (const QLineF& line : std::as_const(m_snapGuides)) {
      QLineF drawnLine = line;
      QRectF bounds = sceneRect();

      if (line.dx() == 0) {
        drawnLine.setP1(QPointF(line.x1(), bounds.top()));
        drawnLine.setP2(QPointF(line.x1(), bounds.bottom()));
      } else {
        drawnLine.setP1(QPointF(bounds.left(), line.y1()));
        drawnLine.setP2(QPointF(bounds.right(), line.y1()));
      }

      painter->drawLine(drawnLine);
    }

    painter->restore();
  }

  // 2. Draw Laser Pointer
  if (m_laserActive || !m_laserTrail.isEmpty()) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    if (!m_laserTrail.isEmpty()) {
      QPointF prev = m_laserPos;
      for (int i = 0; i < m_laserTrail.size(); ++i) {
        int age = m_laserTrail[i].age;
        int alpha = qMax(0, 180 - (age * 12));
        qreal thickness = qMax(1.0, m_laserSize - (age / 2.0));

        QColor c(m_laserColor);
        c.setAlpha(alpha);
        QPen pen(c);
        pen.setWidthF(thickness);
        pen.setCapStyle(Qt::RoundCap);

        painter->setPen(pen);
        painter->drawLine(prev, m_laserTrail[i].pos);
        prev = m_laserTrail[i].pos;
      }
    }

    if (m_laserActive) {
      QRadialGradient grad(m_laserPos, 12);
      grad.setColorAt(0.0, Qt::white);
      grad.setColorAt(0.3, m_laserColor);
      QColor fadeColor(m_laserColor);
      fadeColor.setAlpha(0);
      grad.setColorAt(1.0, fadeColor);

      painter->setPen(Qt::NoPen);
      painter->setBrush(grad);
      painter->drawEllipse(m_laserPos, 12, 12);
    }
    painter->restore();
  }

  // 3. Draw Magnifier Lens. The guard stops the nested render() below from
  // re-entering this block (which would recurse infinitely).
  if (m_magnifierActive && !m_renderingMagnifier) {
    const qreal r = m_magnifierRadius;
    const QPointF center = m_magnifierPos;
    const QRectF lensRect(center.x() - r, center.y() - r, r * 2.0, r * 2.0);

    // The (smaller) scene region that gets blown up to fill the lens.
    const qreal srcHalf = r / m_magnifierZoom;
    const QRectF sourceRect(center.x() - srcHalf, center.y() - srcHalf, srcHalf * 2.0, srcHalf * 2.0);

    // Render that region into an offscreen image, magnified to the lens size.
    // The workspace background item skips painting in offscreen renders (widget
    // == nullptr), so seed the lens with an opaque fill; otherwise the original
    // 1x image shows through the transparent areas as a ghosted double image.
    const int px = qMax(2, qRound(r * 2.0));
    QImage lens(px, px, QImage::Format_ARGB32_Premultiplied);
    QColor lensBg = QColor::fromRgba(Constants::Color::WorkspaceFill);
    lensBg.setAlpha(255);
    lens.fill(lensBg);
    {
      QPainter lensPainter(&lens);
      lensPainter.setRenderHint(QPainter::Antialiasing);
      lensPainter.setRenderHint(QPainter::SmoothPixmapTransform);
      m_renderingMagnifier = true;
      render(&lensPainter, QRectF(0, 0, px, px), sourceRect, Qt::IgnoreAspectRatio);
      m_renderingMagnifier = false;
    }

    // Mask the content to an antialiased circle by applying a soft circular
    // alpha mask to the image itself. setClipPath produces a hard (aliased)
    // clip in the raster engine, which would leave the lens edge jagged.
    {
      QPainter maskPainter(&lens);
      maskPainter.setRenderHint(QPainter::Antialiasing, true);
      maskPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
      maskPainter.setPen(Qt::NoPen);
      maskPainter.setBrush(Qt::black);
      maskPainter.drawEllipse(QRectF(0, 0, px, px));
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    painter->drawImage(lensRect, lens);

    // Lens rim: a dark outer ring with a thin light inner edge.
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(20, 20, 20, 210), 3.0));
    painter->drawEllipse(lensRect);
    painter->setPen(QPen(QColor(255, 255, 255, 180), 1.0));
    painter->drawEllipse(lensRect.adjusted(2, 2, -2, -2));

    // Center crosshair, drawn as a white halo under a dark line so it stays
    // visible over any content. A small gap keeps the exact center point clear.
    const qreal arm = 9.0;
    const qreal gap = 2.5;
    auto drawCrosshair = [&](const QColor& color, qreal width) {
      QPen pen(color, width);
      pen.setCapStyle(Qt::FlatCap);
      painter->setPen(pen);
      painter->drawLine(QPointF(center.x() - arm, center.y()), QPointF(center.x() - gap, center.y()));
      painter->drawLine(QPointF(center.x() + gap, center.y()), QPointF(center.x() + arm, center.y()));
      painter->drawLine(QPointF(center.x(), center.y() - arm), QPointF(center.x(), center.y() - gap));
      painter->drawLine(QPointF(center.x(), center.y() + gap), QPointF(center.x(), center.y() + arm));
    };
    drawCrosshair(QColor(255, 255, 255, 200), 2.6);
    drawCrosshair(QColor(20, 20, 20, 220), 1.0);

    painter->restore();
  }
}

void LayoutScene::clearLayout() {
  QList<QGraphicsItem*> itemsToDelete;

  // First pass: collect only top-level items to avoid double-deletion
  for (auto item : items()) {
    if (item == m_pBgItem) {
      continue;
    }

    if (item->parentItem() == nullptr) {
      if (SnappingUtils::isSnappableItem(item) || item->type() == Constants::Item::GuideItem) {
        itemsToDelete.append(item);
      }
    }
  }
  qDeleteAll(itemsToDelete);
  clearSnapGuides();
}

ResizableAppItem* LayoutScene::addAppItem(const QString& name, const QRectF& rect) {
  ResizableAppItem* item = new ResizableAppItem(name, rect);
  qreal scale = sceneRect().height() / 1080.0;
  item->setFontScale(scale);
  item->initActions();
  connect(item, &ResizableAppItem::propertiesRequested, this, &LayoutScene::itemPropertiesRequested);
  addItem(item);
  return item;
}

void LayoutScene::distributeSelectionH() {
  auto sel = getAlignableItems(selectedItems());
  if (sel.size() < 3) {
    return;
  }

  std::sort(sel.begin(), sel.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->sceneBoundingRect().x() < b->sceneBoundingRect().x(); });
  QGraphicsItem* first = sel.first();
  QGraphicsItem* last = sel.last();
  qreal totalItemWidth = 0;
  for (auto item : std::as_const(sel)) {
    totalItemWidth += item->sceneBoundingRect().width();
  }

  qreal startX = first->sceneBoundingRect().left();
  qreal endX = last->sceneBoundingRect().right();
  qreal totalSpan = endX - startX;
  qreal totalGapSpace = totalSpan - totalItemWidth;
  qreal gap = totalGapSpace / (sel.size() - 1);
  qreal currentX = startX;
  for (auto item : std::as_const(sel)) {
    qreal currentItemX = item->sceneBoundingRect().x();
    item->moveBy(currentX - currentItemX, 0);
    currentX += item->sceneBoundingRect().width() + gap;
  }
}

void LayoutScene::distributeSelectionV() {
  auto sel = getAlignableItems(selectedItems());
  if (sel.size() < 3) {
    return;
  }

  std::sort(sel.begin(), sel.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->sceneBoundingRect().y() < b->sceneBoundingRect().y(); });
  QGraphicsItem* first = sel.first();
  QGraphicsItem* last = sel.last();
  qreal totalItemHeight = 0;
  for (auto item : std::as_const(sel)) {
    totalItemHeight += item->sceneBoundingRect().height();
  }

  qreal startY = first->sceneBoundingRect().top();
  qreal endY = last->sceneBoundingRect().bottom();
  qreal totalSpan = endY - startY;
  qreal totalGapSpace = totalSpan - totalItemHeight;
  qreal gap = totalGapSpace / (sel.size() - 1);
  qreal currentY = startY;
  for (auto item : std::as_const(sel)) {
    qreal currentItemY = item->sceneBoundingRect().y();
    item->moveBy(0, currentY - currentItemY);
    currentY += item->sceneBoundingRect().height() + gap;
  }
}

void LayoutScene::setLaserActive(bool active) {
  m_laserActive = active;
  // The timer only needs to run while the laser is active (to animate the
  // trail fade); otherwise it would be idle work for the scene's whole life.
  if (active) {
    if (!m_laserTimer.isActive()) {
      m_laserTimer.start(16);
    }
  } else {
    m_laserTrail.clear();
    m_laserTimer.stop();
  }
  invalidate(sceneRect(), QGraphicsScene::ForegroundLayer);
}

void LayoutScene::updateLaserPosition(const QPointF& pos) {
  if (!m_laserActive) {
    return;
  }

  // Repaint only the region spanning the old and new head positions (plus the
  // glow radius and line half-width), instead of invalidating the whole scene.
  const qreal margin = m_laserSize + 12.0;
  QRectF dirtyRect = QRectF(m_laserPos, pos).normalized().adjusted(-margin, -margin, margin, margin);

  if (m_laserTrail.isEmpty() || QLineF(m_laserPos, pos).length() > 2.0) {
    m_laserTrail.prepend({m_laserPos, 0});

    if (m_laserTrail.size() > 15) {
      m_laserTrail.removeLast();
    }
  }
  m_laserPos = pos;
  invalidate(dirtyRect, QGraphicsScene::ForegroundLayer);
}

void LayoutScene::setLaserColor(const QColor& color) {
  m_laserColor = color;
}
void LayoutScene::setLaserSize(int size) {
  m_laserSize = size;
}

void LayoutScene::setMagnifierActive(bool active) {
  if (m_magnifierActive == active) {
    return;
  }
  m_magnifierActive = active;

  const qreal margin = m_magnifierRadius + 4.0;
  const QRectF lensRect(m_magnifierPos - QPointF(margin, margin), QSizeF(margin * 2, margin * 2));
  invalidate(lensRect, QGraphicsScene::ForegroundLayer);
}

void LayoutScene::updateMagnifierPosition(const QPointF& pos) {
  if (!m_magnifierActive) {
    return;
  }

  // Repaint both the lens's old and new footprints (plus the rim margin).
  const qreal margin = m_magnifierRadius + 4.0;
  QRectF dirtyRect(m_magnifierPos - QPointF(margin, margin), QSizeF(margin * 2, margin * 2));
  m_magnifierPos = pos;
  dirtyRect = dirtyRect.united(QRectF(m_magnifierPos - QPointF(margin, margin), QSizeF(margin * 2, margin * 2)));
  invalidate(dirtyRect, QGraphicsScene::ForegroundLayer);
}

void LayoutScene::setMagnifierZoom(double zoom) {
  m_magnifierZoom = qBound(1.2, zoom, 8.0);
  if (m_magnifierActive) {
    const qreal margin = m_magnifierRadius + 4.0;
    const QRectF lensRect(m_magnifierPos - QPointF(margin, margin), QSizeF(margin * 2, margin * 2));
    invalidate(lensRect, QGraphicsScene::ForegroundLayer);
  }
}

void LayoutScene::alignSelection(Alignment alignment) {
  switch (alignment) {
    case Left:
      applyAlignment([](const QRectF& r) { return r.left(); }, true);
      break;
    case Right:
      applyAlignment([](const QRectF& r) { return r.right(); }, true);
      break;
    case CenterH:
      applyAlignment([](const QRectF& r) { return r.center().x(); }, true);
      break;
    case Top:
      applyAlignment([](const QRectF& r) { return r.top(); }, false);
      break;
    case Bottom:
      applyAlignment([](const QRectF& r) { return r.bottom(); }, false);
      break;
    case CenterV:
      applyAlignment([](const QRectF& r) { return r.center().y(); }, false);
      break;
  }
}

void LayoutScene::fadeLaserTrail() {
  if (!m_laserActive && m_laserTrail.isEmpty()) {
    return;
  }

  bool needsUpdate = false;

  QRectF dirtyRect(m_laserPos, QSizeF(0, 0));
  dirtyRect.adjust(-20, -20, 20, 20);

  for (int i = 0; i < m_laserTrail.size(); ++i) {
    m_laserTrail[i].age++;

    dirtyRect = dirtyRect.united(QRectF(m_laserTrail[i].pos, QSizeF(1, 1)).adjusted(-10, -10, 10, 10));

    if (m_laserTrail[i].age > 15) {
      m_laserTrail.removeAt(i);
      i--;
    } else {
      needsUpdate = true;
    }
  }
  if (needsUpdate || m_laserActive) {
    invalidate(dirtyRect, QGraphicsScene::ForegroundLayer);
  }
}
