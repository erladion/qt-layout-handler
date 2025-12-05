#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class LayoutScene;
class QGraphicsView;
class QComboBox;
class QSlider;
class QLabel;
class RulerBar;  // Forward Declaration

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

protected:
  void resizeEvent(QResizeEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  // We use eventFilter to capture mouse moves in the view viewport
  bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
  void toggleGrid(bool checked);
  void onGridSizeChanged(int val);
  void onTopBarChanged(int val);
  void onBotBarChanged(int val);
  void addWindow();
  void addZone();
  void removeWindow();
  void saveLayout();
  void loadLayout();

  void alignLeft();
  void alignCenterH();
  void alignRight();
  void alignTop();
  void alignCenterV();
  void alignBottom();
  void distributeH();
  void distributeV();

  // Update rulers when view changes (zoom/pan)
  void updateRulers();

private:
  void createToolbar();

  LayoutScene* scene;
  QGraphicsView* view;

  // Rulers
  RulerBar* m_hRuler;
  RulerBar* m_vRuler;

  QComboBox* typeCombo;
  QSlider* gridSlider;
  QLabel* gridLabel;
};

#endif  // MAINWINDOW_H
