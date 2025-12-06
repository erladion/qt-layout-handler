#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>  // NEW: For closeEvent override
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

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

  // NEW: Handle application close request
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

  bool saveLayout();  // UPDATED: Returns success status
  void loadLayout();

  void setWallpaper();
  void applyTemplate(QAction* action);

  // Feature 6 & 7 Slots
  void toggleLock();
  void groupItems();
  void ungroupItems();

  // Properties Slots
  void toggleProperties();
  void onSelectionChanged();
  void onSceneChanged(const QList<QRectF>& region);

private:
  void createToolbar();
  void createMenuBar();
  void updateRulers();
  void updateInterfaceState();

  // NEW: Helper to check modification state and prompt user
  bool maybeSave();
  // NEW: Updates title with asterisk (*) and internal flag
  void setModified(bool modified);

  QString getTemplateXml(const QString& name);

  LayoutScene* scene;
  QGraphicsView* view;

  RulerBar* m_hRuler;
  RulerBar* m_vRuler;

  QSlider* gridSlider;
  QLabel* gridLabel;

  // Properties Window
  PropertiesDialog* m_propDialog;
  QAction* m_viewPropAct;

  // Ribbon Sections
  RibbonSection* m_secInsert;
  RibbonSection* m_secArrange;
  RibbonSection* m_secAlign;
  RibbonSection* m_secView;

  // NEW: Track unsaved changes
  bool m_isModified;
};

#endif  // MAINWINDOW_H
