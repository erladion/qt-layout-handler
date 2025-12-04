#ifndef LAYOUTSCENE_H
#define LAYOUTSCENE_H

#include <QFutureWatcher>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QVector>

class LayoutScene : public QGraphicsScene {
  Q_OBJECT
public:
  explicit LayoutScene(qreal x, qreal y, qreal w, qreal h, QObject* parent = nullptr);

  bool isGridEnabled() const;
  int gridSize() const;

  void setGridEnabled(bool enabled);
  void setGridSize(int size);

  void setTopBarHeight(int h);
  void setBottomBarHeight(int h);

  int topBarHeight() const { return m_topBarHeight; }
  int bottomBarHeight() const { return m_bottomBarHeight; }

  QRectF getWorkingArea() const;

  // New: Getter for the cached lines so ArtboardItem can read them
  const QVector<QLineF>& gridLines() const { return m_cachedGridLines; }

protected:
  void drawBackground(QPainter* painter, const QRectF& rect) override;

private slots:
  // Slot called when background thread finishes calculating lines
  void onGridCalculationFinished();

private:
  // Helper to start the background thread
  void triggerGridUpdate();

  bool m_gridEnabled;
  int m_gridSize;
  int m_topBarHeight;
  int m_bottomBarHeight;

  QGraphicsRectItem* m_bgItem;

  // Threading members
  QFutureWatcher<QVector<QLineF>> m_gridWatcher;
  QVector<QLineF> m_cachedGridLines;
};

#endif  // LAYOUTSCENE_H
