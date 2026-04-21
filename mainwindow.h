#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QSlider>

#include "snappingitemgroup.h"

#include "drawingmanager.h"

class LayoutScene;
class RulerBar;
class PropertiesDialog;
class RibbonSection;
class QSpinBox;
class QPushButton;

class ProjectorWindow;
class WindowSelector;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

  void showProperties(QGraphicsItem* item = nullptr);
  void toggleFullScreen();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private slots:
  // UI Slots
  void toggleGrid(bool checked);
  void onGridSizeChanged(int val);
  void onTopBarChanged(int val);
  void onBotBarChanged(int val);

  // Actions
  void newLayout();
  void closeLayout();
  void addApp(QAction* action);
  void addZone();
  void removeWindow();

  bool saveLayout();
  void loadLayout();

  void setWallpaper();
  void applyTemplate(QAction* action);

  void toggleLock();
  void groupItems();
  void ungroupItems();

  void align();
  void distributeH();
  void distributeV();

  void toggleProperties();
  void onSelectionChanged();
  void onSceneChanged(const QList<QRectF>& region);

  void openSettings();

private:
  void createToolbar();
  void createMenuBar();
  void createFloatingToolbar();
  void updateRulers();
  void updateInterfaceState();
  bool maybeSave();
  void setModified(bool modified);

  void connectSceneSignals();

  QString getTemplateXml(const QString& name);

  // Define our three operating modes
  enum class PresenterMode { EditLayout, Draw, Laser };
  enum class DrawShape { Freehand, Marker, Rectangle, Ellipse };

  LayoutScene* m_pScene;
  QGraphicsView* m_pView;

  RulerBar* m_pHRuler;
  RulerBar* m_pVRuler;
  QWidget* m_pCorner;

  QSlider* m_pGridSlider;
  QLabel* m_pGridLabel;

  // Spinboxes for Top/Bottom bar height
  QSpinBox* m_pTopBarSpin;
  QSpinBox* m_pBotBarSpin;

  // Properties Window
  PropertiesDialog* m_pProperties;
  QAction* m_pViewPropertiesAction;

  // Toolbar Tracking
  QToolBar* m_pToolbar;

  // Ribbon Sections
  RibbonSection* m_pSectionInsert;
  RibbonSection* m_pSectionArrange;
  RibbonSection* m_pSectionAlign;
  RibbonSection* m_pSectionView;

  bool m_isModified;

  QPushButton* m_pBtnEdit;
  QPushButton* m_pBtnDraw;
  QPushButton* m_pBtnLaser;
  QPushButton* m_pBtnClear;

  PresenterMode m_currentMode = PresenterMode::EditLayout;

  // The floating UI
  QWidget* m_floatingToolbar = nullptr;

  bool m_isDraggingToolbar = false;
  QPoint m_dragOffset;

  bool m_wasMaximized = false;
  WindowSelector* m_selector = nullptr;
  QPointer<ProjectorWindow> m_pProjector = nullptr;

  bool m_isSelectingWindow = false;

  QWidget* m_laserSettingsWidget = nullptr;
  QWidget* m_drawSettingsWidget = nullptr;

  DrawShape m_currentShape = DrawShape::Freehand;
  QColor m_laserColor = Qt::red;
  int m_laserSize = 15;
  QColor m_drawColor = Qt::blue;
  int m_drawSize = 4;

  DrawingManager* m_pDrawingManager = nullptr;

  void updatePopoutPositions();
};

#endif  // MAINWINDOW_H
