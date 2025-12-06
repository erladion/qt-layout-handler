#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QSlider>
#include "snappingitemgroup.h"

class LayoutScene;
class RulerBar;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private slots:
  // UI Slots
  void toggleGrid(bool checked);
  void onGridSizeChanged(int val);
  void onTopBarChanged(int val);
  void onBotBarChanged(int val);

  // Actions
  void addWindow();
  void addZone();
  void removeWindow();
  void saveLayout();
  void loadLayout();

  void setWallpaper();
  void applyTemplate(QAction* action);

  // Feature 6 & 7 Slots
  void toggleLock();
  void groupItems();
  void ungroupItems();

private:
  void createToolbar();
  void updateRulers();

  QString getTemplateXml(const QString& name);

  LayoutScene* scene;
  QGraphicsView* view;

  RulerBar* m_hRuler;
  RulerBar* m_vRuler;

  QSlider* gridSlider;
  QLabel* gridLabel;
  QComboBox* typeCombo;
};

#endif  // MAINWINDOW_H
