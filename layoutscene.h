#ifndef LAYOUTSCENE_H
#define LAYOUTSCENE_H

#include <QDomDocument>
#include <QDomElement>
#include <QFutureWatcher>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPixmap>
#include <QVector>

class ResizableAppItem;

class LayoutScene : public QGraphicsScene {
  Q_OBJECT
 public:
  explicit LayoutScene(qreal x, qreal y, qreal w, qreal h,
                       QObject* parent = nullptr);

  bool isGridEnabled() const;
  int gridSize() const;

  void setGridEnabled(bool enabled);
  void setGridSize(int size);

  void setTopBarHeight(int h);
  void setBottomBarHeight(int h);

  void setWallpaper(const QPixmap& pix);

  int topBarHeight() const { return m_topBarHeight; }
  int bottomBarHeight() const { return m_bottomBarHeight; }

  QRectF getWorkingArea() const;
  const QVector<QLineF>& gridLines() const { return m_cachedGridLines; }

  void clearLayout();

  ResizableAppItem* addAppItem(const QString& name, const QRectF& rect);

  // NEW: Snap Guide Management
  void setSnapGuides(const QList<QLineF>& guides);
  void clearSnapGuides();

 public slots:
  void alignSelectionLeft();
  void alignSelectionRight();
  void alignSelectionTop();
  void alignSelectionBottom();
  void alignSelectionCenterH();
  void alignSelectionCenterV();

  void distributeSelectionH();
  void distributeSelectionV();

 protected:
  void drawBackground(QPainter* painter, const QRectF& rect) override;
  // NEW: Draw snap lines on top
  void drawForeground(QPainter* painter, const QRectF& rect) override;

 private slots:
  void onGridCalculationFinished();

 private:
  void triggerGridUpdate();

  bool m_gridEnabled;
  int m_gridSize;
  int m_topBarHeight;
  int m_bottomBarHeight;

  QGraphicsRectItem* m_bgItem;

  QFutureWatcher<QVector<QLineF>> m_gridWatcher;
  QVector<QLineF> m_cachedGridLines;

  // NEW: Active snap lines to render
  QList<QLineF> m_snapGuides;
};

#endif  // LAYOUTSCENE_H
