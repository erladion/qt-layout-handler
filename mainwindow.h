#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QSlider>
#include "snappingitemgroup.h"

class LayoutScene;
class RulerBar;
class PropertiesDialog;
class RibbonSection;
class QSpinBox;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

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

  void alignLeft();
  void alignRight();
  void alignTop();
  void alignBottom();
  void alignCenterH();
  void alignCenterV();
  void distributeH();
  void distributeV();

  void toggleProperties();
  void onSelectionChanged();
  void onSceneChanged(const QList<QRectF>& region);

  void openSettings();

private:
  void createToolbar();
  void createMenuBar();
  void updateRulers();
  void updateInterfaceState();
  bool maybeSave();
  void setModified(bool modified);

  void connectSceneSignals();

  QString getTemplateXml(const QString& name);

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
};

#endif  // MAINWINDOW_H
