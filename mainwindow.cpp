#include "mainwindow.h"
#include "guidelineitem.h"
#include "layoutscene.h"
#include "layoutserializer.h"
#include "officetoolbar.h"
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
#include <QHBoxLayout>
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
  resize(1400, 900);
  setWindowTitle("Layout Manager");

  scene = new LayoutScene(0, 0, 1920, 1080, this);
  scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  scene->setTopBarHeight(30);
  scene->setBottomBarHeight(40);

  connect(scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSelectionChanged);
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

  m_propDialog = new PropertiesDialog(view);
  m_propDialog->hide();
  m_propDialog->move(20, 20);

  createToolbar();
  createMenuBar();

  statusBar()->showMessage("Rulers active. Drag from rulers to create Guides.");

  QTimer::singleShot(100, this, [this]() {
    if (view && scene) {
      view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
      m_hRuler->update();
      m_vRuler->update();
    }
  });
}

MainWindow::~MainWindow() {}

// ... event filters ...
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

// ... selection logic ...
void MainWindow::toggleProperties() {
  if (m_propDialog->isVisible()) {
    m_propDialog->hide();
    if (m_viewPropAct)
      m_viewPropAct->setChecked(false);
  } else {
    m_propDialog->show();
    m_propDialog->raise();
    m_propDialog->activateWindow();
    if (m_viewPropAct)
      m_viewPropAct->setChecked(true);
    if (!scene->selectedItems().isEmpty())
      m_propDialog->setItem(scene->selectedItems().first());
    else
      m_propDialog->setItem(nullptr);
  }
}

void MainWindow::onSelectionChanged() {
  if (!m_propDialog->isVisible())
    return;
  QList<QGraphicsItem*> sel = scene->selectedItems();
  if (sel.isEmpty()) {
    QTimer::singleShot(50, this, [this]() {
      if (scene->selectedItems().isEmpty())
        m_propDialog->setItem(nullptr);
    });
  } else {
    m_propDialog->setItem(sel.first());
    m_propDialog->raise();
  }
}

void MainWindow::onSceneChanged(const QList<QRectF>& region) {
  Q_UNUSED(region);
  if (m_propDialog && m_propDialog->isVisible() && !scene->selectedItems().isEmpty()) {
    m_propDialog->refreshValues();
  }
}

// ... add/remove logic ...
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
  int startX = safe.left() + 20;
  int startY = safe.top() + 20;
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
  // ... (Same Group Logic) ...
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
  if (itemsToGroup.size() < 2)
    return;
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
    statusBar()->showMessage("Ungrouped items.", 2000);
}

void MainWindow::toggleLock() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  for (auto item : selected) {
    if (ResizableAppItem* app = dynamic_cast<ResizableAppItem*>(item)) {
      app->setLocked(!app->isLocked());
    }
  }
}

void MainWindow::setWallpaper() {
  QString fileName = QFileDialog::getOpenFileName(this, "Select Wallpaper", "", "Images (*.png *.jpg *.jpeg *.bmp)");
  if (fileName.isEmpty())
    return;
  QPixmap pix(fileName);
  if (pix.isNull())
    return;
  scene->setWallpaper(pix);
  statusBar()->showMessage("Wallpaper loaded.", 3000);
}

QString MainWindow::getTemplateXml(const QString& name) {
  if (name == "Coding") {
    return R"(<Layout><App name="IDE" x="0" y="30" width="1150" height="1010" /><App name="Terminal" x="1150" y="30" width="770" height="505" /><App name="Browser" x="1150" y="535" width="770" height="505" /></Layout>)";
  } else if (name == "Streaming") {
    return R"(<Layout><App name="OBS" x="0" y="30" width="480" height="1010" /><App name="Game" x="480" y="30" width="960" height="1010" /><App name="Chat" x="1440" y="30" width="480" height="1010" /></Layout>)";
  }
  return "";
}

void MainWindow::applyTemplate(QAction* action) {
  QString presetName = action->text();
  QString xmlContent = getTemplateXml(presetName);
  if (!xmlContent.isEmpty() && LayoutSerializer::loadFromXml(scene, xmlContent)) {
    statusBar()->showMessage("Applied template: " + presetName, 3000);
  }
}

void MainWindow::saveLayout() {
  QString fileName = QFileDialog::getSaveFileName(this, "Save Layout", "", "XML Files (*.xml)");
  if (!fileName.isEmpty())
    LayoutSerializer::save(scene, fileName);
}

void MainWindow::loadLayout() {
  QString fileName = QFileDialog::getOpenFileName(this, "Load Layout", "", "XML Files (*.xml)");
  if (!fileName.isEmpty())
    LayoutSerializer::load(scene, fileName);
}

void MainWindow::createMenuBar() {
  QMenu* viewMenu = menuBar()->addMenu("View");

  m_viewPropAct = viewMenu->addAction("Properties Window");
  m_viewPropAct->setCheckable(true);
  m_viewPropAct->setShortcut(QKeySequence("F2"));
  connect(m_viewPropAct, &QAction::triggered, this, &MainWindow::toggleProperties);
}

void MainWindow::createToolbar() {
  OfficeToolbar* ribbon = new OfficeToolbar(this);

  // FIX: Styling for Menus only. Removed palette overrides.
  QString controlStyle = R"(
      QMenu {
          background-color: #ffffff;
          border: 1px solid #cccccc;
          color: #333333;
          padding: 2px;
      }
      QMenu::item {
          padding: 5px 25px 5px 25px;
          background: transparent;
          color: #333333;
      }
      QMenu::item:selected {
          background-color: #3399ff;
          color: #ffffff;
      }
  )";

  // --- SECTION: FILE ---
  RibbonSection* fileSec = ribbon->addSection("File", QIcon(":/icons/section-file.svg"));
  QAction* saveAct = new QAction(QIcon(":/icons/save.svg"), "Save", this);
  saveAct->setShortcut(QKeySequence::Save);
  connect(saveAct, &QAction::triggered, this, &MainWindow::saveLayout);
  fileSec->addLargeButton(new RibbonButton(saveAct, RibbonButton::Large));

  QAction* loadAct = new QAction(QIcon(":/icons/load.svg"), "Load", this);
  loadAct->setShortcut(QKeySequence::Open);
  connect(loadAct, &QAction::triggered, this, &MainWindow::loadLayout);
  fileSec->addLargeButton(new RibbonButton(loadAct, RibbonButton::Large));

  // --- SECTION: INSERT ---
  RibbonSection* insSec = ribbon->addSection("Insert", QIcon(":/icons/section-insert.svg"));

  QToolButton* addBtn = new RibbonButton(new QAction(QIcon(":/icons/add.svg"), "Add App", this), RibbonButton::Large);
  addBtn->setPopupMode(QToolButton::InstantPopup);
  QMenu* addMenu = new QMenu(addBtn);
  addMenu->addAction("Browser");
  addMenu->addAction("Terminal");
  addMenu->addAction("Music Player");
  addMenu->addAction("File Manager");

  addMenu->setStyleSheet(controlStyle);

  addBtn->setMenu(addMenu);
  connect(addMenu, &QMenu::triggered, this, &MainWindow::addApp);
  insSec->addLargeButton((RibbonButton*)addBtn);

  QAction* zoneAct = new QAction(QIcon(":/icons/zone.svg"), "Zone", this);
  connect(zoneAct, &QAction::triggered, this, &MainWindow::addZone);
  insSec->addLargeButton(new RibbonButton(zoneAct, RibbonButton::Large));

  QAction* rmAct = new QAction(QIcon(":/icons/remove.svg"), "Delete", this);
  rmAct->setShortcuts({QKeySequence::Delete, QKeySequence(Qt::Key_Backspace)});
  connect(rmAct, &QAction::triggered, this, &MainWindow::removeWindow);
  insSec->addLargeButton(new RibbonButton(rmAct, RibbonButton::Large));

  QAction* wallAct = new QAction(QIcon(":/icons/image.svg"), "Wallpaper", this);
  connect(wallAct, &QAction::triggered, this, &MainWindow::setWallpaper);
  insSec->addWidget(new RibbonButton(wallAct, RibbonButton::Small), 0, 3);

  QToolButton* tempBtn = new RibbonButton(new QAction(QIcon(":/icons/template.svg"), "Templates", this), RibbonButton::Small);
  tempBtn->setPopupMode(QToolButton::InstantPopup);
  QMenu* tempMenu = new QMenu(tempBtn);
  tempMenu->addAction("Coding");
  tempMenu->addAction("Streaming");

  tempMenu->setStyleSheet(controlStyle);

  tempBtn->setMenu(tempMenu);
  connect(tempMenu, &QMenu::triggered, this, &MainWindow::applyTemplate);
  insSec->addWidget(tempBtn, 1, 3);

  // --- SECTION: ARRANGE ---
  RibbonSection* arrSec = ribbon->addSection("Arrange", QIcon(":/icons/section-arrange.svg"));

  QAction* grpAct = new QAction(QIcon(":/icons/group.svg"), "Group", this);
  grpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
  connect(grpAct, &QAction::triggered, this, &MainWindow::groupItems);
  arrSec->addWidget(new RibbonButton(grpAct, RibbonButton::Small), 0, 0);

  QAction* ungrpAct = new QAction(QIcon(":/icons/ungroup.svg"), "Ungroup", this);
  ungrpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
  connect(ungrpAct, &QAction::triggered, this, &MainWindow::ungroupItems);
  arrSec->addWidget(new RibbonButton(ungrpAct, RibbonButton::Small), 1, 0);

  QAction* lockAct = new QAction(QIcon(":/icons/lock.svg"), "Lock", this);
  lockAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
  connect(lockAct, &QAction::triggered, this, &MainWindow::toggleLock);
  arrSec->addWidget(new RibbonButton(lockAct, RibbonButton::Small), 2, 0);

  // --- SECTION: ALIGNMENT ---
  RibbonSection* alignSec = ribbon->addSection("Alignment", QIcon(":/icons/section-alignment.svg"));

  QAction* al = new QAction(QIcon(":/icons/align-left.svg"), "Left", this);
  connect(al, &QAction::triggered, scene, &LayoutScene::alignSelectionLeft);
  alignSec->addWidget(new RibbonButton(al, RibbonButton::Small), 0, 0);

  QAction* ac = new QAction(QIcon(":/icons/align-center-h.svg"), "Center", this);
  connect(ac, &QAction::triggered, scene, &LayoutScene::alignSelectionCenterH);
  alignSec->addWidget(new RibbonButton(ac, RibbonButton::Small), 1, 0);

  QAction* ar = new QAction(QIcon(":/icons/align-right.svg"), "Right", this);
  connect(ar, &QAction::triggered, scene, &LayoutScene::alignSelectionRight);
  alignSec->addWidget(new RibbonButton(ar, RibbonButton::Small), 2, 0);

  QAction* at = new QAction(QIcon(":/icons/align-top.svg"), "Top", this);
  connect(at, &QAction::triggered, scene, &LayoutScene::alignSelectionTop);
  alignSec->addWidget(new RibbonButton(at, RibbonButton::Small), 0, 1);

  QAction* am = new QAction(QIcon(":/icons/align-center-v.svg"), "Middle", this);
  connect(am, &QAction::triggered, scene, &LayoutScene::alignSelectionCenterV);
  alignSec->addWidget(new RibbonButton(am, RibbonButton::Small), 1, 1);

  QAction* ab = new QAction(QIcon(":/icons/align-bottom.svg"), "Bottom", this);
  connect(ab, &QAction::triggered, scene, &LayoutScene::alignSelectionBottom);
  alignSec->addWidget(new RibbonButton(ab, RibbonButton::Small), 2, 1);

  // --- SECTION: VIEW ---
  RibbonSection* viewSec = ribbon->addSection("View", QIcon(":/icons/section-view.svg"));

  QAction* gridAct = new QAction(QIcon(":/icons/grid.svg"), "Grid", this);
  gridAct->setCheckable(true);
  connect(gridAct, &QAction::toggled, this, &MainWindow::toggleGrid);
  viewSec->addWidget(new RibbonButton(gridAct, RibbonButton::Small), 0, 0);

  gridSlider = new QSlider(Qt::Horizontal);
  gridSlider->setRange(10, 200);
  gridSlider->setValue(50);
  gridSlider->setFixedWidth(80);
  gridSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  gridSlider->setEnabled(false);

  // FIX: Removed gridSlider->setPalette(lightPal); to allow default system colors
  connect(gridSlider, &QSlider::valueChanged, this, &MainWindow::onGridSizeChanged);
  viewSec->addWidget(gridSlider, 1, 0);

  gridLabel = new QLabel("50px");
  viewSec->addWidget(gridLabel, 2, 0);

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

  QSpinBox* topSpin = new QSpinBox();
  topSpin->setRange(0, 200);
  topSpin->setValue(30);
  topSpin->setSuffix(" px");
  topSpin->setFixedWidth(60);
  topSpin->setStyleSheet(spinStyle);
  topSpin->setKeyboardTracking(false);
  connect(topSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTopBarChanged);

  QSpinBox* botSpin = new QSpinBox();
  botSpin->setRange(0, 200);
  botSpin->setValue(40);
  botSpin->setSuffix(" px");
  botSpin->setFixedWidth(60);
  botSpin->setStyleSheet(spinStyle);
  botSpin->setKeyboardTracking(false);
  connect(botSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBotBarChanged);

  QWidget* topContainer = new QWidget();
  // FIX: Force fixed size policy on container to prevent stretching and early collapse
  topContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* topLayout = new QHBoxLayout(topContainer);
  topLayout->setContentsMargins(0, 0, 0, 0);
  topLayout->setSpacing(5);
  topLayout->addWidget(new QLabel("Top Bar:"));
  topLayout->addWidget(topSpin);

  QWidget* botContainer = new QWidget();
  // FIX: Force fixed size policy on container to prevent stretching and early collapse
  botContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* botLayout = new QHBoxLayout(botContainer);
  botLayout->setContentsMargins(0, 0, 0, 0);
  botLayout->setSpacing(5);
  botLayout->addWidget(new QLabel("Bot Bar:"));
  botLayout->addWidget(botSpin);

  viewSec->addWidget(topContainer, 0, 1);  // Col 1
  viewSec->addWidget(botContainer, 1, 1);  // Col 1

  ribbon->addSpacer();

  QToolBar* tb = addToolBar("Ribbon");
  tb->setMovable(false);
  tb->addWidget(ribbon);
}
