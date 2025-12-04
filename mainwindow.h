#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class LayoutScene;
class QGraphicsView;
class QComboBox;
class QSlider;
class QLabel;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow();

protected:
  void resizeEvent(QResizeEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private slots:
  void toggleGrid(bool checked);
  void onGridSizeChanged(int val);
  void onTopBarChanged(int val);
  void onBotBarChanged(int val);
  void addWindow();
  void removeWindow();
  void saveLayout();
  void loadLayout();

private:
  void createToolbar();

  LayoutScene* scene;
  QGraphicsView* view;
  QComboBox* typeCombo;
  QSlider* gridSlider;
  QLabel* gridLabel;
};

#endif  // MAINWINDOW_H
