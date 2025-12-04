#include "mainwindow.h"
#include "layoutscene.h"
#include "resizableappitem.h"

#include <QAction>
#include <QComboBox>
#include <QDomDocument>
#include <QFileDialog>
#include <QGraphicsView>
#include <QIcon>
#include <QKeyEvent>  // Added
#include <QLabel>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <cmath>

MainWindow::MainWindow() {
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

  view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
  view->setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);

  setCentralWidget(view);

  createToolbar();
  statusBar()->showMessage("Right-click items for options. Press Delete to remove.");
}

// NEW: Handle Keyboard Shortcuts
void MainWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    removeWindow();
  } else {
    QMainWindow::keyPressEvent(event);
  }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  if (view && scene) {
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
  }
}

void MainWindow::toggleGrid(bool checked) {
  scene->setGridEnabled(checked);
  gridSlider->setEnabled(checked);
}

void MainWindow::onGridSizeChanged(int val) {
  scene->setGridSize(val);
  gridLabel->setText(QString::number(val) + "px");
}

void MainWindow::onTopBarChanged(int val) {
  scene->setTopBarHeight(val);
}
void MainWindow::onBotBarChanged(int val) {
  scene->setBottomBarHeight(val);
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

void MainWindow::removeWindow() {
  QList<QGraphicsItem*> selected = scene->selectedItems();
  if (selected.isEmpty()) {
    statusBar()->showMessage("No item selected to remove.", 2000);
    return;
  }

  for (auto item : selected) {
    if (dynamic_cast<ResizableAppItem*>(item)) {
      scene->removeItem(item);
      delete item;
    }
  }
  statusBar()->showMessage("Item removed.", 2000);
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
      // Save Z-Value to preserve stacking order
      appEl.setAttribute("z", appItem->zValue());
      root.appendChild(appEl);
    }
  }
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream stream(&file);
    stream << doc.toString();
    file.close();
    statusBar()->showMessage("Layout saved.", 2000);
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

  // Clear existing items
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
  statusBar()->showMessage("Layout loaded.", 2000);
}

void MainWindow::createToolbar() {
  QToolBar* toolbar = addToolBar("Tools");
  toolbar->setIconSize(QSize(24, 24));

  QAction* gridAct = toolbar->addAction(QIcon(":/icons/grid.svg"), "Grid");
  gridAct->setCheckable(true);
  connect(gridAct, &QAction::toggled, this, &MainWindow::toggleGrid);

  gridSlider = new QSlider(Qt::Horizontal);
  gridSlider->setRange(10, 200);
  gridSlider->setValue(50);
  gridSlider->setFixedWidth(100);
  gridSlider->setEnabled(false);
  gridLabel = new QLabel("50px");
  gridLabel->setFixedWidth(40);
  connect(gridSlider, &QSlider::valueChanged, this, &MainWindow::onGridSizeChanged);
  toolbar->addWidget(gridSlider);
  toolbar->addWidget(gridLabel);

  toolbar->addSeparator();

  toolbar->addWidget(new QLabel(" Top:"));
  QSpinBox* topSpin = new QSpinBox();
  topSpin->setRange(0, 200);
  topSpin->setValue(30);
  topSpin->setSuffix("px");
  connect(topSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTopBarChanged);
  toolbar->addWidget(topSpin);

  toolbar->addWidget(new QLabel(" Bot:"));
  QSpinBox* botSpin = new QSpinBox();
  botSpin->setRange(0, 200);
  botSpin->setValue(40);
  botSpin->setSuffix("px");
  connect(botSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBotBarChanged);
  toolbar->addWidget(botSpin);

  toolbar->addSeparator();

  typeCombo = new QComboBox();
  typeCombo->addItems({"Browser", "Terminal", "Music Player", "File Manager"});
  toolbar->addWidget(typeCombo);

  QAction* addAct = toolbar->addAction(QIcon(":/icons/add.svg"), "Add");
  connect(addAct, &QAction::triggered, this, &MainWindow::addWindow);

  QAction* delAct = toolbar->addAction(QIcon(":/icons/remove.svg"), "Remove");
  connect(delAct, &QAction::triggered, this, &MainWindow::removeWindow);

  toolbar->addSeparator();

  QAction* saveAct = toolbar->addAction(QIcon(":/icons/save.svg"), "Save");
  connect(saveAct, &QAction::triggered, this, &MainWindow::saveLayout);

  QAction* loadAct = toolbar->addAction(QIcon(":/icons/load.svg"), "Load");
  connect(loadAct, &QAction::triggered, this, &MainWindow::loadLayout);
}
