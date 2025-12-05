#include "mainwindow.h"
#include "layoutscene.h"
#include "resizableappitem.h"
#include "rulerbar.h"
#include "zoneitem.h"

#include <QAction>
#include <QComboBox>
#include <QDomDocument>
#include <QFileDialog>
#include <QGraphicsView>
#include <QGridLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <algorithm>
#include <cmath>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  resize(1200, 800);
  setWindowTitle("Layout Manager");

  // Create the Custom Scene
  scene = new LayoutScene(0, 0, 1920, 1080, this);
  scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  scene->setTopBarHeight(30);
  scene->setBottomBarHeight(40);

  // Setup View
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

  // --- RULER SETUP ---
  m_hRuler = new RulerBar(RulerBar::Horizontal, view, this);
  m_vRuler = new RulerBar(RulerBar::Vertical, view, this);

  // Corner widget (Top-Left void)
  QWidget* corner = new QWidget(this);
  corner->setFixedSize(25, 25);
  corner->setStyleSheet("background-color: #2b2b2b;");

  // Layout: Grid with Rulers surrounding the View
  QWidget* central = new QWidget(this);
  QGridLayout* grid = new QGridLayout(central);
  grid->setSpacing(0);
  grid->setContentsMargins(0, 0, 0, 0);

  grid->addWidget(corner, 0, 0);
  grid->addWidget(m_hRuler, 0, 1);
  grid->addWidget(m_vRuler, 1, 0);
  grid->addWidget(view, 1, 1);

  setCentralWidget(central);

  // Track Mouse for Ruler Markers
  view->viewport()->setMouseTracking(true);
  view->viewport()->installEventFilter(this);

  createToolbar();
  statusBar()->showMessage("Rulers active. Click 'Add Zone' to create snap targets.");
}

MainWindow::~MainWindow() {
  // Basic cleanup if needed, QObject hierarchy handles most deletes
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched == view->viewport() && event->type() == QEvent::MouseMove) {
    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
    QPoint pos = mouseEvent->pos();
    m_hRuler->updateCursorPos(pos);
    m_vRuler->updateCursorPos(pos);
  }
  return QMainWindow::eventFilter(watched, event);
}

// Update rulers when zooming/panning occurs during resize
void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  if (view && scene) {
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    // Force rulers to repaint with new scale
    m_hRuler->update();
    m_vRuler->update();
  }
}

void MainWindow::updateRulers() {
  m_hRuler->update();
  m_vRuler->update();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    removeWindow();
  } else {
    QMainWindow::keyPressEvent(event);
  }
}

void MainWindow::toggleGrid(bool checked) {
  scene->setGridEnabled(checked);
  gridSlider->setEnabled(checked);
  updateRulers();  // Grid might affect visual clarity, good to redraw
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
  statusBar()->showMessage("Zone added. Drop apps onto it to snap.", 3000);
}

void MainWindow::removeWindow() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item)) {
      scene->removeItem(item);
      delete item;
    }
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

  for (auto item : scene->items()) {
    if (dynamic_cast<ResizableAppItem*>(item)) {
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
      if (el.hasAttribute("z"))
        item->setZValue(el.attribute("z").toDouble());
      scene->addItem(item);
    }
    node = node.nextSibling();
  }
}

void MainWindow::alignLeft() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to align.", 2000);
    return;
  }
  qreal minX = std::numeric_limits<qreal>::max();
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item))
      minX = std::min(minX, item->pos().x());
  }
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item))
      item->setPos(minX, item->pos().y());
  }
}

void MainWindow::alignRight() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to align.", 2000);
    return;
  }
  qreal maxRight = -std::numeric_limits<qreal>::max();
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      maxRight = std::max(maxRight, app->pos().x() + app->rect().width());
  }
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      app->setPos(maxRight - app->rect().width(), app->pos().y());
  }
}

void MainWindow::alignTop() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to align.", 2000);
    return;
  }
  qreal minY = std::numeric_limits<qreal>::max();
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item))
      minY = std::min(minY, item->pos().y());
  }
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item))
      item->setPos(item->pos().x(), minY);
  }
}

void MainWindow::alignBottom() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to align.", 2000);
    return;
  }
  qreal maxBottom = -std::numeric_limits<qreal>::max();
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      maxBottom = std::max(maxBottom, app->pos().y() + app->rect().height());
  }
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      app->setPos(app->pos().x(), maxBottom - app->rect().height());
  }
}

void MainWindow::alignCenterH() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to align.", 2000);
    return;
  }
  QRectF totalRect;
  bool first = true;
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item)) {
      if (first) {
        totalRect = item->sceneBoundingRect();
        first = false;
      } else
        totalRect = totalRect.united(item->sceneBoundingRect());
    }
  }
  qreal centerX = totalRect.center().x();
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      app->setPos(centerX - (app->rect().width() / 2), app->pos().y());
  }
}

void MainWindow::alignCenterV() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 2) {
    statusBar()->showMessage("Select at least 2 items to align.", 2000);
    return;
  }
  QRectF totalRect;
  bool first = true;
  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item)) {
      if (first) {
        totalRect = item->sceneBoundingRect();
        first = false;
      } else
        totalRect = totalRect.united(item->sceneBoundingRect());
    }
  }
  qreal centerY = totalRect.center().y();
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      app->setPos(app->pos().x(), centerY - (app->rect().height() / 2));
  }
}

void MainWindow::distributeH() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 3) {
    statusBar()->showMessage("Select at least 3 items to distribute.", 2000);
    return;
  }
  std::sort(selected.begin(), selected.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->pos().x() < b->pos().x(); });
  QGraphicsItem* first = selected.first();
  QGraphicsItem* last = selected.last();
  ResizableAppItem* firstApp = dynamic_cast<ResizableAppItem*>(first);
  ResizableAppItem* lastApp = dynamic_cast<ResizableAppItem*>(last);
  if (!firstApp || !lastApp)
    return;
  qreal totalItemWidth = 0;
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      totalItemWidth += app->rect().width();
  }
  qreal totalSpan = (lastApp->pos().x() + lastApp->rect().width()) - firstApp->pos().x();
  qreal totalGap = totalSpan - totalItemWidth;
  qreal gap = totalGap / (selected.size() - 1);
  qreal currentX = firstApp->pos().x();
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item)) {
      app->setPos(currentX, app->pos().y());
      currentX += app->rect().width() + gap;
    }
  }
}

void MainWindow::distributeV() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.size() < 3) {
    statusBar()->showMessage("Select at least 3 items to distribute.", 2000);
    return;
  }
  std::sort(selected.begin(), selected.end(), [](QGraphicsItem* a, QGraphicsItem* b) { return a->pos().y() < b->pos().y(); });
  QGraphicsItem* first = selected.first();
  QGraphicsItem* last = selected.last();
  ResizableAppItem* firstApp = dynamic_cast<ResizableAppItem*>(first);
  ResizableAppItem* lastApp = dynamic_cast<ResizableAppItem*>(last);
  if (!firstApp || !lastApp)
    return;
  qreal totalItemHeight = 0;
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item))
      totalItemHeight += app->rect().height();
  }
  qreal totalSpan = (lastApp->pos().y() + lastApp->rect().height()) - firstApp->pos().y();
  qreal totalGap = totalSpan - totalItemHeight;
  qreal gap = totalGap / (selected.size() - 1);
  qreal currentY = firstApp->pos().y();
  for (auto item : selected) {
    if (auto app = dynamic_cast<ResizableAppItem*>(item)) {
      app->setPos(app->pos().x(), currentY);
      currentY += app->rect().height() + gap;
    }
  }
}

void MainWindow::createToolbar() {
  QToolBar* toolbar = addToolBar("Tools");
  toolbar->setIconSize(QSize(24, 24));

  connect(toolbar->addAction(QIcon(":/icons/save.svg"), "Save"), &QAction::triggered, this, &MainWindow::saveLayout);
  connect(toolbar->addAction(QIcon(":/icons/load.svg"), "Load"), &QAction::triggered, this, &MainWindow::loadLayout);

  toolbar->addSeparator();

  connect(toolbar->addAction(QIcon(":/icons/add.svg"), "Add Window"), &QAction::triggered, this, &MainWindow::addWindow);
  connect(toolbar->addAction(QIcon(":/icons/zone.svg"), "Add Zone"), &QAction::triggered, this, &MainWindow::addZone);
  connect(toolbar->addAction(QIcon(":/icons/remove.svg"), "Remove"), &QAction::triggered, this, &MainWindow::removeWindow);

  toolbar->addSeparator();

  connect(toolbar->addAction(QIcon(":/icons/align-left.svg"), "Align Left"), &QAction::triggered, this, &MainWindow::alignLeft);
  connect(toolbar->addAction(QIcon(":/icons/align-center-h.svg"), "Align Center H"), &QAction::triggered, this, &MainWindow::alignCenterH);
  connect(toolbar->addAction(QIcon(":/icons/align-right.svg"), "Align Right"), &QAction::triggered, this, &MainWindow::alignRight);

  connect(toolbar->addAction(QIcon(":/icons/align-top.svg"), "Align Top"), &QAction::triggered, this, &MainWindow::alignTop);
  connect(toolbar->addAction(QIcon(":/icons/align-center-v.svg"), "Align Center V"), &QAction::triggered, this, &MainWindow::alignCenterV);
  connect(toolbar->addAction(QIcon(":/icons/align-bottom.svg"), "Align Bottom"), &QAction::triggered, this, &MainWindow::alignBottom);

  connect(toolbar->addAction(QIcon(":/icons/distribute-h.svg"), "Distribute H"), &QAction::triggered, this, &MainWindow::distributeH);
  connect(toolbar->addAction(QIcon(":/icons/distribute-v.svg"), "Distribute V"), &QAction::triggered, this, &MainWindow::distributeV);

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

  // FIX: Added gridLabel initialization which was missing in original snippet
  // but required for onGridSizeChanged to work without crashing.
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
