#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QDomDocument>
#include <QEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QSlider>

// Forward declarations
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
  void keyPressEvent(QKeyEvent* event) override;

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

  // Alignment
  void alignLeft();
  void alignRight();
  void alignTop();
  void alignBottom();
  void alignCenterH();
  void alignCenterV();

  // Distribution
  void distributeH();
  void distributeV();

private:
  void createToolbar();
  void updateRulers();

  // Helper to generate template XML
  QString getTemplateXml(const QString& name);

  // Shared Helper: Parses a QDomDocument and populates the scene
  void loadLayoutFromDoc(const QDomDocument& doc);

  // Scene & View
  LayoutScene* scene;
  QGraphicsView* view;

  // Rulers
  RulerBar* m_hRuler;
  RulerBar* m_vRuler;

  // REMOVED: QGraphicsPixmapItem* wallpaperItem = nullptr;
  // Wallpaper is now handled by the Scene/Artboard directly

  // UI Controls
  QSlider* gridSlider;
  QLabel* gridLabel;
  QComboBox* typeCombo;
};

#endif  // MAINWINDOW_H
