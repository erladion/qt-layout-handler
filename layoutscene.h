#ifndef LAYOUTSCENE_H
#define LAYOUTSCENE_H

#include <QDomDocument>
#include <QDomElement>
#include <QFutureWatcher>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPixmap>
#include <QTimer>
#include <QVector>

#include <functional>

class ResizableAppItem;

class LayoutScene : public QGraphicsScene {
  Q_OBJECT
public:
  enum Alignment { Left, Top, Right, Bottom, CenterH, CenterV };

  explicit LayoutScene(qreal x, qreal y, qreal w, qreal h, QObject* parent = nullptr);

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

  void setSnapGuides(const QList<QLineF>& guides);
  void clearSnapGuides();

  void setLaserActive(bool active);
  void updateLaserPosition(const QPointF& pos);
  void setLaserColor(const QColor& color);
  void setLaserSize(int size);

  void alignSelection(Alignment alignment);

public slots:
  void distributeSelectionH();
  void distributeSelectionV();

protected:
  void drawForeground(QPainter* painter, const QRectF& rect) override;

private slots:
  void onGridCalculationFinished();

  void fadeLaserTrail();

private:
  void triggerGridUpdate();

  void applyAlignment(std::function<qreal(const QRectF&)> getEdge, bool isHorizontal);

  bool m_gridEnabled;
  int m_gridSize;
  int m_topBarHeight;
  int m_bottomBarHeight;

  QGraphicsRectItem* m_pBgItem;

  QFutureWatcher<QVector<QLineF>> m_gridWatcher;
  QVector<QLineF> m_cachedGridLines;

  QList<QLineF> m_snapGuides;

  struct TrailPoint {
    QPointF pos;
    int age;
  };
  QList<TrailPoint> m_laserTrail;
  QPointF m_laserPos;
  QTimer m_laserTimer;
  bool m_laserActive = false;
  QColor m_laserColor = Qt::red;
  int m_laserSize = 15;
};

#endif  // LAYOUTSCENE_H
