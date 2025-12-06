#include "mainwindow.h"
#include "guidelineitem.h"
#include "layoutscene.h"
#include "layoutserializer.h"
#include "propertiesdialog.h"
#include "resizableappitem.h"
#include "rulerbar.h"
#include "snappingitemgroup.h"
#include "zoneitem.h"

#include <QAction>
#include <QComboBox>
#include <QDomDocument>
#include <QFileDialog>
#include <QGraphicsItemGroup>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QGridLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <algorithm>
#include <cmath>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  resize(1200, 800);
  setWindowTitle("Layout Manager");

  scene = new LayoutScene(0, 0, 1920, 1080, this);
  scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  scene->setTopBarHeight(30);
  scene->setBottomBarHeight(40);

  // Connect Selection Logic for Properties Window
  connect(scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSelectionChanged);
  // Restore Scene Changed connection for live updates
  connect(scene, &QGraphicsScene::changed, this, &MainWindow::onSceneChanged);

  view = new QGraphicsView(scene, this);
  view->setBackgroundBrush(this->palette().window());
  view->setRenderHint(QPainter::Antialiasing);
  view->setFrameShape(QFrame::NoFrame);

  view->setDragMode(QGraphicsView::RubberBandDrag);
  view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
  view->setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);

  m_hRuler = new RulerBar(RulerBar::Horizontal, view, this);
  m_vRuler = new RulerBar(RulerBar::Vertical, view, this);

  QWidget* corner = new QWidget(this);
  corner->setFixedSize(25, 25);
  corner->setStyleSheet("background-color: #2b2b2b;");

  QWidget* central = new QWidget(this);
  QGridLayout* grid = new QGridLayout(central);
  grid->setSpacing(0);
  grid->setContentsMargins(0, 0, 0, 0);

  grid->addWidget(corner, 0, 0);
  grid->addWidget(m_hRuler, 0, 1);
  grid->addWidget(m_vRuler, 1, 0);
  grid->addWidget(view, 1, 1);

  setCentralWidget(central);

  view->viewport()->setMouseTracking(true);

  view->viewport()->installEventFilter(this);
  m_hRuler->installEventFilter(this);
  m_vRuler->installEventFilter(this);

  // Initialize Properties Dialog (Hidden by default)
  // FIX: Reverted parent to 'view' so it stays contained within the application
  // and behaves like an internal tool window.
  m_propDialog = new PropertiesDialog(view);
  m_propDialog->hide();
  m_propDialog->move(20, 20);  // Initial position inside the view

  createToolbar();
  createMenuBar();

  statusBar()->showMessage("Rulers active. Drag from rulers to create Guides.");

  // FIX: Startup Scale Fix
  QTimer::singleShot(100, this, [this]() {
    if (view && scene) {
      view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
      m_hRuler->update();
      m_vRuler->update();
    }
  });
}

MainWindow::~MainWindow() {
  // m_propDialog is automatically deleted because it is a child of 'view'
}

// ... event filters and existing logic ...

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::MouseMove) {
    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
    QPoint globalPos = mouseEvent->globalPos();
    QPoint viewPos = view->viewport()->mapFromGlobal(globalPos);
    m_hRuler->updateCursorPos(viewPos);
    m_vRuler->updateCursorPos(viewPos);
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  if (view && scene) {
    if (view->width() > 50 && view->height() > 50) {
      view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }
    m_hRuler->update();
    m_vRuler->update();
  }
}

void MainWindow::updateRulers() {
  m_hRuler->update();
  m_vRuler->update();
}

void MainWindow::toggleGrid(bool checked) {
  scene->setGridEnabled(checked);
  gridSlider->setEnabled(checked);
  updateRulers();
}

void MainWindow::onGridSizeChanged(int val) {
  scene->setGridSize(val);
  if (gridLabel)
    gridLabel->setText(QString::number(val) + "px");
  updateRulers();
}

void MainWindow::onTopBarChanged(int val) {
  scene->setTopBarHeight(val);
  updateRulers();
}
void MainWindow::onBotBarChanged(int val) {
  scene->setBottomBarHeight(val);
  updateRulers();
}

// --- Properties Logic ---

void MainWindow::toggleProperties() {
  if (m_propDialog->isVisible()) {
    m_propDialog->hide();
    m_viewPropAct->setChecked(false);
  } else {
    m_propDialog->show();
    m_propDialog->raise();  // Ensure it's on top of the canvas
    m_propDialog->activateWindow();
    m_viewPropAct->setChecked(true);

    // Force update immediately
    if (!scene->selectedItems().isEmpty()) {
      m_propDialog->setItem(scene->selectedItems().first());
    } else {
      m_propDialog->setItem(nullptr);
    }
  }
}

void MainWindow::onSelectionChanged() {
  if (!m_propDialog->isVisible())
    return;

  QList<QGraphicsItem*> sel = scene->selectedItems();

  if (sel.isEmpty()) {
    // Debounce selection clearing
    QTimer::singleShot(50, this, [this]() {
      if (scene->selectedItems().isEmpty()) {
        m_propDialog->setItem(nullptr);
      }
    });
  } else {
    m_propDialog->setItem(sel.first());
    m_propDialog->raise();
  }
}

void MainWindow::onSceneChanged(const QList<QRectF>& region) {
  Q_UNUSED(region);
  // Refresh properties if dialog is visible and something is selected
  if (m_propDialog && m_propDialog->isVisible() && !scene->selectedItems().isEmpty()) {
    m_propDialog->refreshValues();
  }
}

// -------------------------

void MainWindow::addApp(QAction* action) {
  QString type = action->text();
  double w = 400, h = 300;
  if (type == "Browser") {
    w = 800;
    h = 600;
  } else if (type == "Terminal") {
    w = 600;
    h = 400;
  }

  ResizableAppItem* item = scene->addAppItem(type, QRectF(0, 0, w, h));
  QRectF safe = scene->getWorkingArea();

  int maxX = static_cast<int>(safe.width()) - 200;
  int maxY = static_cast<int>(safe.height()) - 200;

  int startX = safe.left();
  if (maxX > 0)
    startX += QRandomGenerator::global()->bounded(maxX);

  int startY = safe.top();
  if (maxY > 0)
    startY += QRandomGenerator::global()->bounded(maxY);

  if (scene->isGridEnabled()) {
    startX = std::round(startX / (double)scene->gridSize()) * scene->gridSize();
    startY = std::round(startY / (double)scene->gridSize()) * scene->gridSize();
  }

  item->setPos(startX, startY);
}

void MainWindow::addZone() {
  ZoneItem* zone = new ZoneItem(QRectF(0, 0, 400, 400));
  QRectF safe = scene->getWorkingArea();
  zone->setPos(safe.center().x() - 200, safe.center().y() - 200);
  scene->addItem(zone);
  statusBar()->showMessage("Zone added.", 3000);
}

void MainWindow::removeWindow() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item) || dynamic_cast<QGraphicsItemGroup*>(item) ||
        dynamic_cast<GuideLineItem*>(item)) {
      scene->removeItem(item);
      delete item;
    }
  }
}

void MainWindow::groupItems() {
  QList<QGraphicsItem*> selected = scene->selectedItems();

  SnappingItemGroup* existingGroup = nullptr;
  QList<QGraphicsItem*> itemsToGroup;
  int groupCount = 0;

  for (QGraphicsItem* item : selected) {
    if (SnappingItemGroup* grp = dynamic_cast<SnappingItemGroup*>(item)) {
      existingGroup = grp;
      groupCount++;
    } else if (dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item)) {
      itemsToGroup.append(item);
    }
  }

  if (groupCount == 1 && !itemsToGroup.isEmpty()) {
    for (QGraphicsItem* item : itemsToGroup) {
      item->setSelected(false);
      existingGroup->addToGroup(item);
    }
    existingGroup->update();
    statusBar()->showMessage("Added item(s) to existing group.", 2000);
    return;
  }

  if (itemsToGroup.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to group.", 2000);
    return;
  }

  SnappingItemGroup* group = new SnappingItemGroup(scene);
  for (QGraphicsItem* item : itemsToGroup) {
    item->setSelected(false);
    group->addToGroup(item);
  }

  scene->addItem(group);
  group->setSelected(true);
  statusBar()->showMessage("Items grouped.", 2000);
}

void MainWindow::ungroupItems() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.isEmpty())
    return;

  int count = 0;
  for (auto item : selected) {
    if (QGraphicsItemGroup* group = dynamic_cast<QGraphicsItemGroup*>(item)) {
      scene->destroyItemGroup(group);
      count++;
    }
  }

  if (count > 0)
    statusBar()->showMessage("Ungrouped " + QString::number(count) + " items.", 2000);
}

void MainWindow::toggleLock() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.isEmpty())
    return;

  bool anyLocked = false;
  for (auto item : selected) {
    if (ResizableAppItem* app = dynamic_cast<ResizableAppItem*>(item)) {
      bool newState = !app->isLocked();
      app->setLocked(newState);
      if (newState)
        anyLocked = true;
    }
  }
  statusBar()->showMessage(anyLocked ? "Items locked." : "Items unlocked.", 2000);
}

void MainWindow::setWallpaper() {
  QString fileName = QFileDialog::getOpenFileName(this, "Select Wallpaper", "", "Images (*.png *.jpg *.jpeg *.bmp)");
  if (fileName.isEmpty())
    return;

  QPixmap pix(fileName);
  if (pix.isNull()) {
    statusBar()->showMessage("Failed to load image.", 3000);
    return;
  }

  scene->setWallpaper(pix);
  statusBar()->showMessage("Wallpaper loaded and scaled.", 3000);
}

QString MainWindow::getTemplateXml(const QString& name) {
  if (name == "Coding") {
    return R"(
            <Layout>
                <App name="IDE" x="0" y="30" width="1150" height="1010" />
                <App name="Terminal" x="1150" y="30" width="770" height="505" />
                <App name="Browser" x="1150" y="535" width="770" height="505" />
            </Layout>
        )";
  } else if (name == "Streaming") {
    return R"(
            <Layout>
                <App name="OBS" x="0" y="30" width="480" height="1010" />
                <App name="Game" x="480" y="30" width="960" height="1010" />
                <App name="Chat" x="1440" y="30" width="480" height="1010" />
            </Layout>
        )";
  }
  return "";
}

void MainWindow::applyTemplate(QAction* action) {
  QString presetName = action->text();
  QString xmlContent = getTemplateXml(presetName);
  if (xmlContent.isEmpty())
    return;

  if (LayoutSerializer::loadFromXml(scene, xmlContent)) {
    statusBar()->showMessage("Applied template: " + presetName, 3000);
  } else {
    statusBar()->showMessage("Error parsing template XML.", 3000);
  }
}

void MainWindow::saveLayout() {
  QString fileName = QFileDialog::getSaveFileName(this, "Save Layout", "", "XML Files (*.xml)");
  if (fileName.isEmpty())
    return;

  if (LayoutSerializer::save(scene, fileName)) {
    statusBar()->showMessage("Layout saved.", 3000);
  } else {
    statusBar()->showMessage("Failed to save layout.", 3000);
  }
}

void MainWindow::loadLayout() {
  QString fileName = QFileDialog::getOpenFileName(this, "Load Layout", "", "XML Files (*.xml)");
  if (fileName.isEmpty())
    return;

  if (LayoutSerializer::load(scene, fileName)) {
    statusBar()->showMessage("Layout loaded from file.", 3000);
  } else {
    statusBar()->showMessage("Failed to load layout.", 3000);
  }
}

void MainWindow::createMenuBar() {
  QMenu* viewMenu = menuBar()->addMenu("View");

  m_viewPropAct = viewMenu->addAction("Properties Window");
  m_viewPropAct->setCheckable(true);
  m_viewPropAct->setShortcut(QKeySequence("F2"));
  connect(m_viewPropAct, &QAction::triggered, this, &MainWindow::toggleProperties);
}

void MainWindow::createToolbar() {
  QToolBar* toolbar = addToolBar("Tools");
  toolbar->setIconSize(QSize(24, 24));

  // ... existing toolbar actions (Save, Load, Wallpaper, Templates, Add, Remove, Lock, Group, Ungroup, Alignments, Grid) ...
  QAction* saveAct = toolbar->addAction(QIcon(":/icons/save.svg"), "Save");
  saveAct->setShortcut(QKeySequence::Save);
  connect(saveAct, &QAction::triggered, this, &MainWindow::saveLayout);

  QAction* loadAct = toolbar->addAction(QIcon(":/icons/load.svg"), "Load");
  loadAct->setShortcut(QKeySequence::Open);
  connect(loadAct, &QAction::triggered, this, &MainWindow::loadLayout);

  connect(toolbar->addAction(QIcon(":/icons/image.svg"), "Wallpaper"), &QAction::triggered, this, &MainWindow::setWallpaper);

  QToolButton* tempBtn = new QToolButton();
  tempBtn->setText("Templates");
  tempBtn->setIcon(QIcon(":/icons/template.svg"));
  tempBtn->setPopupMode(QToolButton::InstantPopup);
  QMenu* tempMenu = new QMenu(tempBtn);
  tempMenu->addAction("Coding");
  tempMenu->addAction("Streaming");
  tempBtn->setMenu(tempMenu);
  connect(tempMenu, &QMenu::triggered, this, &MainWindow::applyTemplate);
  toolbar->addWidget(tempBtn);

  toolbar->addSeparator();

  QToolButton* addBtn = new QToolButton();
  addBtn->setText("Add App");
  addBtn->setIcon(QIcon(":/icons/add.svg"));
  addBtn->setPopupMode(QToolButton::InstantPopup);
  QMenu* addMenu = new QMenu(addBtn);
  addMenu->addAction("Browser");
  addMenu->addAction("Terminal");
  addMenu->addAction("Music Player");
  addMenu->addAction("File Manager");
  addBtn->setMenu(addMenu);
  connect(addMenu, &QMenu::triggered, this, &MainWindow::addApp);
  toolbar->addWidget(addBtn);

  connect(toolbar->addAction(QIcon(":/icons/zone.svg"), "Add Zone"), &QAction::triggered, this, &MainWindow::addZone);

  QAction* removeAct = toolbar->addAction(QIcon(":/icons/remove.svg"), "Remove");
  removeAct->setShortcuts({QKeySequence::Delete, QKeySequence(Qt::Key_Backspace)});
  connect(removeAct, &QAction::triggered, this, &MainWindow::removeWindow);

  toolbar->addSeparator();
  QAction* lockAct = toolbar->addAction(QIcon(":/icons/lock.svg"), "Lock");
  lockAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
  connect(lockAct, &QAction::triggered, this, &MainWindow::toggleLock);

  QAction* grpAct = toolbar->addAction(QIcon(":/icons/group.svg"), "Group");
  grpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
  connect(grpAct, &QAction::triggered, this, &MainWindow::groupItems);

  QAction* ungrpAct = toolbar->addAction(QIcon(":/icons/ungroup.svg"), "Ungroup");
  ungrpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
  connect(ungrpAct, &QAction::triggered, this, &MainWindow::ungroupItems);

  toolbar->addSeparator();

  connect(toolbar->addAction(QIcon(":/icons/align-left.svg"), "L"), &QAction::triggered, scene, &LayoutScene::alignSelectionLeft);
  connect(toolbar->addAction(QIcon(":/icons/align-center-h.svg"), "CH"), &QAction::triggered, scene, &LayoutScene::alignSelectionCenterH);
  connect(toolbar->addAction(QIcon(":/icons/align-right.svg"), "R"), &QAction::triggered, scene, &LayoutScene::alignSelectionRight);

  connect(toolbar->addAction(QIcon(":/icons/align-top.svg"), "T"), &QAction::triggered, scene, &LayoutScene::alignSelectionTop);
  connect(toolbar->addAction(QIcon(":/icons/align-center-v.svg"), "CV"), &QAction::triggered, scene, &LayoutScene::alignSelectionCenterV);
  connect(toolbar->addAction(QIcon(":/icons/align-bottom.svg"), "B"), &QAction::triggered, scene, &LayoutScene::alignSelectionBottom);

  connect(toolbar->addAction(QIcon(":/icons/distribute-h.svg"), "DH"), &QAction::triggered, scene, &LayoutScene::distributeSelectionH);
  connect(toolbar->addAction(QIcon(":/icons/distribute-v.svg"), "DV"), &QAction::triggered, scene, &LayoutScene::distributeSelectionV);

  toolbar->addSeparator();

  QAction* gridAct = toolbar->addAction(QIcon(":/icons/grid.svg"), "Grid");
  gridAct->setCheckable(true);
  connect(gridAct, &QAction::toggled, this, &MainWindow::toggleGrid);

  gridSlider = new QSlider(Qt::Horizontal);
  gridSlider->setRange(10, 200);
  gridSlider->setValue(50);
  gridSlider->setFixedWidth(80);
  gridSlider->setEnabled(false);
  connect(gridSlider, &QSlider::valueChanged, this, &MainWindow::onGridSizeChanged);
  toolbar->addWidget(gridSlider);

  gridLabel = new QLabel("50px");
  toolbar->addWidget(gridLabel);

  // Common Stylesheet for Light Mode Spinboxes in Toolbar
  QString spinStyle =
      "QSpinBox { background-color: #ffffff; color: #333333; border: 1px solid #cccccc; padding: 2px; selection-background-color: #3399ff; }"
      "QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 16px; border-left: 1px solid #cccccc; border-bottom: "
      "1px solid #cccccc; background: #f5f5f5; }"
      "QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 16px; border-left: 1px solid #cccccc; "
      "border-top: 0px solid #cccccc; background: #f5f5f5; }"
      "QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: #e0e0e0; }"
      "QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background: #d0d0d0; }"
      "QSpinBox::up-arrow { width: 10px; height: 10px; image: url(:/resources/spin-up-dark.svg); }"
      "QSpinBox::down-arrow { width: 10px; height: 10px; image: url(:/resources/spin-down-dark.svg); }"
      "QSpinBox:disabled { color: #aaaaaa; background-color: #f0f0f0; }";

  toolbar->addWidget(new QLabel(" T:"));
  QSpinBox* topSpin = new QSpinBox();
  topSpin->setRange(0, 200);
  topSpin->setValue(30);
  topSpin->setSuffix("px");
  // Apply Style
  topSpin->setStyleSheet(spinStyle);
  // Disable keyboard tracking for safer typing
  topSpin->setKeyboardTracking(false);
  connect(topSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTopBarChanged);
  toolbar->addWidget(topSpin);

  toolbar->addWidget(new QLabel(" B:"));
  QSpinBox* botSpin = new QSpinBox();
  botSpin->setRange(0, 200);
  botSpin->setValue(40);
  botSpin->setSuffix("px");
  // Apply Style
  botSpin->setStyleSheet(spinStyle);
  // Disable keyboard tracking
  botSpin->setKeyboardTracking(false);
  connect(botSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBotBarChanged);
  toolbar->addWidget(botSpin);
}
