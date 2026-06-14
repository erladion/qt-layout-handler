#ifndef DRAWINGMANAGER_H
#define DRAWINGMANAGER_H

#include <QColor>
#include <QGraphicsScene>
#include <QList>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QPointer>

class QGraphicsView;
class QGraphicsItem;
class QGraphicsPathItem;
class QEvent;

class DrawingManager : public QObject {
  Q_OBJECT
public:
  enum class Shape { Freehand, Marker, Rectangle, Ellipse };

  explicit DrawingManager(QGraphicsScene* scene, QObject* parent = nullptr);
  ~DrawingManager();

  bool handleViewportEvent(QEvent* event, QGraphicsView* view);

  void setShape(Shape shape);
  void setColor(const QColor& color);
  void setSize(int size);

  void clearDrawings();

  // Removes a single drawn item (e.g. via Delete). Returns true if the item
  // belonged to this manager.
  bool removeItem(QGraphicsItem* item);

  void undo();
  void redo();
  bool canUndo() const;
  bool canRedo() const;

private:
  QPointer<QGraphicsScene> m_pScene;
  QGraphicsPathItem* m_drawingLayer;
  QGraphicsItem* m_activeDrawItem;

  QPainterPath m_currentPath;

  Shape m_currentShape;
  QColor m_drawColor;
  int m_drawSize;

  QPointF m_drawStartPos;

  QList<QGraphicsItem*> m_drawnItems;
  QList<QGraphicsItem*> m_undoneItems;
};

#endif  // DRAWINGMANAGER_H
