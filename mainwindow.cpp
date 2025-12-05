#include "mainwindow.h"
#include "guidelineitem.h"
#include "layoutscene.h"
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

  view = new QGraphicsView(scene, this);
  view->setBackgroundBrush(this->palette().window());
  view->setRenderHint(QPainter::Antialiasing);
  view->setFrameShape(QFrame::NoFrame);

  // Removed explicit alignment to restore default centering behavior
  // view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

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

  createToolbar();
  statusBar()->showMessage("Rulers active. Drag from rulers to create Guides.");

  // Keep the startup fix to ensure fitInView works correctly on launch
  QTimer::singleShot(100, this, [this]() {
    if (view && scene) {
      view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
      m_hRuler->update();
      m_vRuler->update();
    }
  });
}

MainWindow::~MainWindow() {}

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

// REMOVED keyPressEvent implementation

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

void MainWindow::addWindow() {
  QString type = typeCombo->currentText();
  double w = 400, h = 300;
  if (type == "Browser") {
    w = 800;
    h = 600;
  } else if (type == "Terminal") {
    w = 600;
    h = 400;
  }

  ResizableAppItem* item = new ResizableAppItem(type, QRectF(0, 0, w, h));
  QRectF safe = scene->getWorkingArea();

  int startX = safe.left() + QRandomGenerator::global()->bounded((int)safe.width() - 200);
  int startY = safe.top() + QRandomGenerator::global()->bounded((int)safe.height() - 200);

  if (scene->isGridEnabled()) {
    startX = std::round(startX / (double)scene->gridSize()) * scene->gridSize();
    startY = std::round(startY / (double)scene->gridSize()) * scene->gridSize();
  }
  item->setPos(startX, startY);
  scene->addItem(item);
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
  QList<QGraphicsItem*> itemsToGroup;
  for (QGraphicsItem* item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item)) {
      itemsToGroup.append(item);
    }
  }

  if (itemsToGroup.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to group.", 2000);
    return;
  }

  SnappingItemGroup* group = new SnappingItemGroup(scene);
  for (QGraphicsItem* item : itemsToGroup) {
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

void MainWindow::loadLayoutFromDoc(const QDomDocument& doc) {
  QList<QGraphicsItem*> items = scene->items();
  for (auto item : items) {
    if (dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item)) {
      scene->removeItem(item);
      delete item;
    }
  }

  QDomElement root = doc.documentElement();
  QDomNode node = root.firstChild();
  while (!node.isNull()) {
    QDomElement el = node.toElement();
    if (!el.isNull() && el.tagName() == "App") {
      ResizableAppItem* item =
          new ResizableAppItem(el.attribute("name"), QRectF(0, 0, el.attribute("width").toDouble(), el.attribute("height").toDouble()));

      item->setPos(el.attribute("x").toDouble(), el.attribute("y").toDouble());
      if (el.hasAttribute("z")) {
        item->setZValue(el.attribute("z").toDouble());
      }
      scene->addItem(item);
    }
    node = node.nextSibling();
  }
}

void MainWindow::applyTemplate(QAction* action) {
  QString presetName = action->text();
  QString xmlContent = getTemplateXml(presetName);
  if (xmlContent.isEmpty())
    return;

  QDomDocument doc;
  if (doc.setContent(xmlContent)) {
    loadLayoutFromDoc(doc);
    statusBar()->showMessage("Applied template: " + presetName, 3000);
  } else {
    statusBar()->showMessage("Error parsing template XML.", 3000);
  }
}

void MainWindow::saveLayout() {
  QString fileName = QFileDialog::getSaveFileName(this, "Save Layout", "", "XML Files (*.xml)");
  if (fileName.isEmpty())
    return;
  QDomDocument doc;
  QDomElement root = doc.createElement("Layout");
  doc.appendChild(root);
  for (auto item : scene->items()) {
    if (auto appItem = dynamic_cast<ResizableAppItem*>(item)) {
      QDomElement appEl = doc.createElement("App");
      appEl.setAttribute("name", appItem->name());
      appEl.setAttribute("x", appItem->pos().x());
      appEl.setAttribute("y", appItem->pos().y());
      appEl.setAttribute("width", appItem->rect().width());
      appEl.setAttribute("height", appItem->rect().height());
      appEl.setAttribute("z", appItem->zValue());
      root.appendChild(appEl);
    }
  }
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream stream(&file);
    stream << doc.toString();
    file.close();
  }
}

void MainWindow::loadLayout() {
  QString fileName = QFileDialog::getOpenFileName(this, "Load Layout", "", "XML Files (*.xml)");
  if (fileName.isEmpty())
    return;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    return;
  }
  file.close();

  loadLayoutFromDoc(doc);
  statusBar()->showMessage("Layout loaded from file.", 3000);
}

// --- DELEGATE ALIGNMENT TO SCENE ---
void MainWindow::alignLeft() {
  scene->alignSelectionLeft();
}
void MainWindow::alignRight() {
  scene->alignSelectionRight();
}
void MainWindow::alignTop() {
  scene->alignSelectionTop();
}
void MainWindow::alignBottom() {
  scene->alignSelectionBottom();
}
void MainWindow::alignCenterH() {
  scene->alignSelectionCenterH();
}
void MainWindow::alignCenterV() {
  scene->alignSelectionCenterV();
}
void MainWindow::distributeH() {
  scene->distributeSelectionH();
}
void MainWindow::distributeV() {
  scene->distributeSelectionV();
}

void MainWindow::createToolbar() {
  QToolBar* toolbar = addToolBar("Tools");
  toolbar->setIconSize(QSize(24, 24));

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

  connect(toolbar->addAction(QIcon(":/icons/add.svg"), "Add Window"), &QAction::triggered, this, &MainWindow::addWindow);
  connect(toolbar->addAction(QIcon(":/icons/zone.svg"), "Add Zone"), &QAction::triggered, this, &MainWindow::addZone);

  QAction* removeAct = toolbar->addAction(QIcon(":/icons/remove.svg"), "Remove");
  // Set both Delete and Backspace as shortcuts
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

  connect(toolbar->addAction(QIcon(":/icons/align-left.svg"), "L"), &QAction::triggered, this, &MainWindow::alignLeft);
  connect(toolbar->addAction(QIcon(":/icons/align-center-h.svg"), "CH"), &QAction::triggered, this, &MainWindow::alignCenterH);
  connect(toolbar->addAction(QIcon(":/icons/align-right.svg"), "R"), &QAction::triggered, this, &MainWindow::alignRight);

  connect(toolbar->addAction(QIcon(":/icons/align-top.svg"), "T"), &QAction::triggered, this, &MainWindow::alignTop);
  connect(toolbar->addAction(QIcon(":/icons/align-center-v.svg"), "CV"), &QAction::triggered, this, &MainWindow::alignCenterV);
  connect(toolbar->addAction(QIcon(":/icons/align-bottom.svg"), "B"), &QAction::triggered, this, &MainWindow::alignBottom);

  connect(toolbar->addAction(QIcon(":/icons/distribute-h.svg"), "DH"), &QAction::triggered, this, &MainWindow::distributeH);
  connect(toolbar->addAction(QIcon(":/icons/distribute-v.svg"), "DV"), &QAction::triggered, this, &MainWindow::distributeV);

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

  toolbar->addWidget(new QLabel(" T:"));
  QSpinBox* topSpin = new QSpinBox();
  topSpin->setRange(0, 200);
  topSpin->setValue(30);
  topSpin->setSuffix("px");
  connect(topSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTopBarChanged);
  toolbar->addWidget(topSpin);

  toolbar->addWidget(new QLabel(" B:"));
  QSpinBox* botSpin = new QSpinBox();
  botSpin->setRange(0, 200);
  botSpin->setValue(40);
  botSpin->setSuffix("px");
  connect(botSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBotBarChanged);
  toolbar->addWidget(botSpin);

  typeCombo = new QComboBox();
  typeCombo->addItems({"Browser", "Terminal", "Music Player", "File Manager"});
  toolbar->addWidget(typeCombo);
}
