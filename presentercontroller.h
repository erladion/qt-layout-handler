#ifndef PRESENTERCONTROLLER_H
#define PRESENTERCONTROLLER_H

#include <QColor>
#include <QObject>
#include <QPoint>

#include "drawingmanager.h"

class QGraphicsView;
class LayoutScene;
class QWidget;
class QPushButton;
class QEvent;

// Owns the presenter overlay: the floating mode toolbar, the laser/draw
// settings popouts, the presenter mode state, and the per-scene DrawingManager.
class PresenterController : public QObject {
  Q_OBJECT
public:
  enum class Mode { EditLayout, Draw, Laser };

  explicit PresenterController(QGraphicsView* view, QObject* parent = nullptr);

  // Bind to the active scene, recreating the per-scene drawing manager. Pass
  // nullptr when the layout is closed (tears the manager down while the scene
  // is still alive).
  void setScene(LayoutScene* scene);

  // Routes viewport mouse events for laser/draw mode; returns true if consumed.
  bool handleViewportEvent(QObject* watched, QEvent* event);

  // Cancels the active draw/laser tool and returns to the Move (Edit) tool.
  void resetToEditMode();

  // Deletes a drawn item if it belongs to the drawing manager; returns true if so.
  bool removeDrawnItem(QGraphicsItem* item);

protected:
  // Handles dragging of the floating toolbar (installed on it as a filter).
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  void createFloatingToolbar();
  void updatePopoutPositions();

  QGraphicsView* m_pView;
  LayoutScene* m_pScene = nullptr;
  DrawingManager* m_pDrawingManager = nullptr;

  Mode m_currentMode = Mode::EditLayout;
  DrawingManager::Shape m_currentShape = DrawingManager::Shape::Freehand;

  QColor m_laserColor = Qt::red;
  int m_laserSize = 15;
  QColor m_drawColor = Qt::blue;
  int m_drawSize = 4;

  QWidget* m_floatingToolbar = nullptr;
  QPushButton* m_pBtnEdit = nullptr;
  QPushButton* m_pBtnDraw = nullptr;
  QPushButton* m_pBtnLaser = nullptr;
  QPushButton* m_pBtnClear = nullptr;

  QWidget* m_laserSettingsWidget = nullptr;
  QWidget* m_drawSettingsWidget = nullptr;

  bool m_isDraggingToolbar = false;
  QPoint m_dragOffset;
};

#endif  // PRESENTERCONTROLLER_H
