#ifndef LAYOUTSCENE_H
#define LAYOUTSCENE_H

#include <QFutureWatcher>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPixmap>
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

  // New: Set wallpaper on the internal artboard
  void setWallpaper(const QPixmap& pix);

  int topBarHeight() const { return m_topBarHeight; }
  int bottomBarHeight() const { return m_bottomBarHeight; }

  QRectF getWorkingArea() const;

  // Getter for cached lines so ArtboardItem can read them
  const QVector<QLineF>& gridLines() const { return m_cachedGridLines; }

protected:
  void drawBackground(QPainter* painter, const QRectF& rect) override;

private slots:
  void onGridCalculationFinished();

private:
  void triggerGridUpdate();

  bool m_gridEnabled;
  int m_gridSize;
  int m_topBarHeight;
  int m_bottomBarHeight;

  // We need to cast this to ArtboardItem* internally,
  // but keeping it QGraphicsRectItem* in header avoids circular dep or extra includes if careful.
  // However, to be clean, let's keep it generic here and cast in cpp,
  // or forward declare ArtboardItem (but ArtboardItem is usually private to the cpp).
  QGraphicsRectItem* m_bgItem;

  QFutureWatcher<QVector<QLineF>> m_gridWatcher;
  QVector<QLineF> m_cachedGridLines;
};

#endif  // LAYOUTSCENE_H
