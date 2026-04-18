#include "mainwindow.h"
#include "constants.h"
#include "guidelineitem.h"
#include "layoutscene.h"
#include "layoutserializer.h"
#include "mirroredappitem.h"
#include "newlayoutdialog.h"
#include "officetoolbar.h"
#include "projectorwindow.h"
#include "propertiesdialog.h"
#include "resizableappitem.h"
#include "rulerbar.h"
#include "settingsdialog.h"
#include "snappingitemgroup.h"
#include "windowselector.h"
#include "zoneitem.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
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
#include <QMouseEvent>
#include <QPushButton>
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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_pScene(nullptr), m_pToolbar(nullptr), m_isModified(false), m_pTopBarSpin(nullptr), m_pBotBarSpin(nullptr) {
  resize(1400, 900);
  setWindowTitle("Layout Editor");

  m_pView = new QGraphicsView(this);
  m_pView->setBackgroundBrush(QColor(Constants::Color::ViewBackgroundEmpty));
  m_pView->setRenderHint(QPainter::Antialiasing);
  m_pView->setFrameShape(QFrame::NoFrame);

  m_pView->setDragMode(QGraphicsView::RubberBandDrag);
  m_pView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
  m_pView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_pView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_pView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
  m_pView->setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);

  m_pHRuler = new RulerBar(RulerBar::Horizontal, m_pView, this);
  m_pVRuler = new RulerBar(RulerBar::Vertical, m_pView, this);

  m_pCorner = new QWidget(this);
  m_pCorner->setFixedSize(25, 25);
  m_pCorner->setStyleSheet(QString("background-color: %1;").arg(QColor(Constants::Color::CornerWidget).name()));

  QWidget* central = new QWidget(this);
  QGridLayout* grid = new QGridLayout(central);
  grid->setSpacing(0);
  grid->setContentsMargins(0, 0, 0, 0);

  grid->addWidget(m_pCorner, 0, 0);
  grid->addWidget(m_pHRuler, 0, 1);
  grid->addWidget(m_pVRuler, 1, 0);
  grid->addWidget(m_pView, 1, 1);

  setCentralWidget(central);

  m_pView->viewport()->setMouseTracking(true);

  m_pView->viewport()->installEventFilter(this);
  // FIX: Updated variable name to m_pHRuler
  m_pHRuler->installEventFilter(this);
  m_pVRuler->installEventFilter(this);

  m_pProperties = new PropertiesDialog(m_pView);
  m_pProperties->hide();
  m_pProperties->move(20, 20);

  createToolbar();
  createMenuBar();
  createFloatingToolbar();

  updateInterfaceState();
  statusBar()->showMessage("No active layout. Create a New Layout (File -> New) to begin.", Constants::StatusMessageDuration);

  QTimer::singleShot(100, this, [this]() {
    if (m_pView && m_pScene) {
      m_pView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
      m_pHRuler->update();
      m_pVRuler->update();
    }
  });

  m_selector = new WindowSelector(this);

  connect(m_selector, &WindowSelector::windowSelectedForGStreamer, this, [=](QString captureSource) {
    // Pass the OS-specific string into the item
    MirroredAppItem* item = new MirroredAppItem(captureSource);

    item->initActions();
    connect(item, &ResizableAppItem::propertiesRequested, this, &MainWindow::showProperties);

    m_pScene->addItem(item);
    m_pScene->clearSelection();
    item->setSelected(true);
  });
}

MainWindow::~MainWindow() {
  if (m_pScene) {
    m_pScene->disconnect(this);
  }
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (maybeSave()) {
    event->accept();
  } else {
    event->ignore();
  }
}

bool MainWindow::maybeSave() {
  if (!m_isModified) {
    return true;
  }

  const QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("Unsaved Changes"),
                                                               tr("The document has been modified.\n"
                                                                  "Do you want to save your changes?"),
                                                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

  switch (ret) {
    case QMessageBox::Save:
      return saveLayout();
    case QMessageBox::Cancel:
      return false;
    default:
      break;
  }
  return true;
}

void MainWindow::setModified(bool modified) {
  m_isModified = modified;
  QString title = "Layout Editor";
  if (m_isModified) {
    title += " *";
  }
  setWindowTitle(title);
}

void MainWindow::connectSceneSignals() {
  if (!m_pScene) {
    return;
  }

  connect(m_pScene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSelectionChanged);
  connect(m_pScene, &QGraphicsScene::changed, this, &MainWindow::onSceneChanged);
}

void MainWindow::updateInterfaceState() {
  bool hasScene = (m_pScene != nullptr);

  if (m_pSectionInsert) {
    m_pSectionInsert->setEnabled(hasScene);
  }
  if (m_pSectionArrange) {
    m_pSectionArrange->setEnabled(hasScene);
  }
  if (m_pSectionAlign) {
    m_pSectionAlign->setEnabled(hasScene);
  }
  if (m_pSectionView) {
    m_pSectionView->setEnabled(hasScene);
  }

  m_pHRuler->setVisible(hasScene);
  m_pVRuler->setVisible(hasScene);
  if (m_pCorner)
    m_pCorner->setVisible(hasScene);

  if (hasScene) {
    m_pView->setBackgroundBrush(this->palette().window());
  } else {
    m_pView->setBackgroundBrush(QColor(Constants::Color::ViewBackgroundEmpty));
  }
}

void MainWindow::showProperties(QGraphicsItem* item) {
  if (!m_pProperties->isVisible())
    m_pProperties->show();
  m_pProperties->raise();
  m_pProperties->activateWindow();
  if (item)
    m_pProperties->setItem(item);
  else if (!m_pScene->selectedItems().isEmpty())
    m_pProperties->setItem(m_pScene->selectedItems().first());
}

void MainWindow::toggleFullScreen() {
  if (isFullScreen()) {
    if (m_wasMaximized)
      showMaximized();
    else
      showNormal();
    m_pToolbar->setVisible(true);
    menuBar()->setVisible(true);
    statusBar()->setVisible(true);
  } else {
    m_wasMaximized = isMaximized();
    m_pToolbar->setVisible(false);
    menuBar()->setVisible(false);
    statusBar()->setVisible(false);
    m_pHRuler->setVisible(false);
    m_pVRuler->setVisible(false);
    showFullScreen();
  }
}

void MainWindow::newLayout() {
  if (!maybeSave()) {
    return;
  }

  NewLayoutDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    if (m_pScene) {
      m_pScene->deleteLater();
    }

    int w = dlg.selectedWidth();
    int h = dlg.selectedHeight();

    m_pScene = new LayoutScene(0, 0, w, h, this);
    m_pScene->setItemIndexMethod(QGraphicsScene::NoIndex);

    m_pScene->setTopBarHeight(SettingsDialog::getTopBarHeight());
    m_pScene->setBottomBarHeight(SettingsDialog::getBottomBarHeight());

    connectSceneSignals();

    addBaseSceneItems();

    m_pView->setScene(m_pScene);
    updateInterfaceState();
    setModified(false);

    // Update UI controls
    if (m_pTopBarSpin) {
      m_pTopBarSpin->blockSignals(true);
      m_pTopBarSpin->setValue(SettingsDialog::getTopBarHeight());
      m_pTopBarSpin->blockSignals(false);
    }
    if (m_pBotBarSpin) {
      m_pBotBarSpin->blockSignals(true);
      m_pBotBarSpin->setValue(SettingsDialog::getBottomBarHeight());
      m_pBotBarSpin->blockSignals(false);
    }

    m_pView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    statusBar()->showMessage(QString("Created new layout: %1x%2").arg(w).arg(h), Constants::StatusMessageDuration);
  }
}

void MainWindow::addBaseSceneItems() {
  if (m_pScene == nullptr) {
    return;
  }

  m_laserDot = new LaserPointerItem();
  m_laserDot->hide();
  m_pScene->addItem(m_laserDot);

  // 2. Setup the Drawing Layer
  m_drawingLayer = new QGraphicsPathItem();
  QPen drawPen(Qt::yellow);
  drawPen.setWidth(4);
  drawPen.setCapStyle(Qt::RoundCap);
  drawPen.setJoinStyle(Qt::RoundJoin);
  m_drawingLayer->setPen(drawPen);
  m_drawingLayer->setZValue(9998);  // Just under the laser
  m_pScene->addItem(m_drawingLayer);

  // 3. Connect the buttons
  connect(m_pBtnEdit, &QPushButton::clicked, this, [=]() {
    m_currentMode = PresenterMode::EditLayout;
    m_laserDot->hide();
    m_pBtnDraw->setChecked(false);
    m_pBtnLaser->setChecked(false);
  });

  connect(m_pBtnDraw, &QPushButton::clicked, this, [=]() {
    m_currentMode = PresenterMode::Draw;
    m_laserDot->hide();
    m_pBtnEdit->setChecked(false);
    m_pBtnLaser->setChecked(false);
  });

  connect(m_pBtnLaser, &QPushButton::clicked, this, [=]() {
    m_currentMode = PresenterMode::Laser;
    m_laserDot->show();  // Turn the laser on!
    m_pBtnEdit->setChecked(false);
    m_pBtnDraw->setChecked(false);
  });

  connect(m_pBtnClear, &QPushButton::clicked, this, [=]() {
    // Instantly wipe the drawings from the projector
    m_currentPath = QPainterPath();
    m_drawingLayer->setPath(m_currentPath);
  });
}

void MainWindow::closeLayout() {
  if (!maybeSave()) {
    return;
  }

  if (!m_pScene) {
    return;
  }

  m_pView->setScene(nullptr);

  m_pScene->deleteLater();
  m_pScene = nullptr;

  if (m_pProperties) {
    m_pProperties->setItem(nullptr);
  }

  updateInterfaceState();
  setModified(false);
  statusBar()->showMessage("Layout closed.", Constants::StatusMessageDuration);
}

void MainWindow::openSettings() {
  SettingsDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    if (m_pScene) {
      int newSize = SettingsDialog::getAppFontSize();
      for (auto item : m_pScene->items()) {
        if (item->type() == Constants::Item::AppItem) {
          auto app = static_cast<ResizableAppItem*>(item);
          app->setBaseFontSize(newSize);
        }
      }
      // Also apply bar heights
      int newTop = SettingsDialog::getTopBarHeight();
      int newBot = SettingsDialog::getBottomBarHeight();
      m_pScene->setTopBarHeight(newTop);
      m_pScene->setBottomBarHeight(newBot);

      if (m_pTopBarSpin) {
        m_pTopBarSpin->blockSignals(true);
        m_pTopBarSpin->setValue(newTop);
        m_pTopBarSpin->blockSignals(false);
      }
      if (m_pBotBarSpin) {
        m_pBotBarSpin->blockSignals(true);
        m_pBotBarSpin->setValue(newBot);
        m_pBotBarSpin->blockSignals(false);
      }
    }
    statusBar()->showMessage("Settings applied.", Constants::StatusMessageDuration);
  }
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (m_isSelectingWindow && event->type() == QEvent::MouseButtonPress) {
    if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
      m_pView->viewport()->releaseMouse();
      m_isSelectingWindow = false;
      m_selector->captureWindowUnderCursor();
      return true;
    }
  }

  if (watched == m_floatingToolbar) {
    if (event->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
      m_isDraggingToolbar = true;
      m_dragOffset = static_cast<QMouseEvent*>(event)->pos();
      return true;
    } else if (event->type() == QEvent::MouseMove && m_isDraggingToolbar) {
      m_floatingToolbar->move(m_floatingToolbar->pos() + static_cast<QMouseEvent*>(event)->pos() - m_dragOffset);
      return true;
    } else if (event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
      m_isDraggingToolbar = false;
      return true;
    }
  }

  if (event->type() == QEvent::MouseMove) {
    QPoint viewPos = m_pView->viewport()->mapFromGlobal(static_cast<QMouseEvent*>(event)->globalPosition().toPoint());
    m_pHRuler->updateCursorPos(viewPos);
    m_pVRuler->updateCursorPos(viewPos);
  }

  if (watched == m_pView->viewport()) {
    if (m_currentMode == PresenterMode::Laser && event->type() == QEvent::MouseMove) {
      m_laserDot->updatePosition(m_pView->mapToScene(static_cast<QMouseEvent*>(event)->pos()));
      return true;
    }
    if (m_currentMode == PresenterMode::Draw) {
      QPointF scenePos = m_pView->mapToScene(static_cast<QMouseEvent*>(event)->pos());
      if (event->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
        m_currentPath.moveTo(scenePos);
        return true;
      } else if (event->type() == QEvent::MouseMove && (static_cast<QMouseEvent*>(event)->buttons() & Qt::LeftButton)) {
        m_currentPath.lineTo(scenePos);
        m_drawingLayer->setPath(m_currentPath);
        return true;
      }
    }
  }

  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  if (m_pView && m_pScene) {
    if (m_pView->width() > 50 && m_pView->height() > 50) {
      m_pView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    }
    m_pHRuler->update();
    m_pVRuler->update();
  }
}

void MainWindow::updateRulers() {
  m_pHRuler->update();
  m_pVRuler->update();
}

void MainWindow::toggleGrid(bool checked) {
  if (!m_pScene) {
    return;
  }

  m_pScene->setGridEnabled(checked);
  m_pGridSlider->setEnabled(checked);
  updateRulers();
}

void MainWindow::onGridSizeChanged(int val) {
  if (!m_pScene) {
    return;
  }

  m_pScene->setGridSize(val);
  if (m_pGridLabel) {
    m_pGridLabel->setText(QString::number(val) + "px");
  }

  updateRulers();
}

void MainWindow::onTopBarChanged(int val) {
  if (!m_pScene) {
    return;
  }

  m_pScene->setTopBarHeight(val);
  updateRulers();
}
void MainWindow::onBotBarChanged(int val) {
  if (!m_pScene) {
    return;
  }

  m_pScene->setBottomBarHeight(val);
  updateRulers();
}

void MainWindow::toggleProperties() {
  if (m_pProperties->isVisible()) {
    m_pProperties->hide();
    if (m_pViewPropertiesAction) {
      m_pViewPropertiesAction->setChecked(false);
    }
  } else {
    m_pProperties->show();
    m_pProperties->raise();
    m_pProperties->activateWindow();
    if (m_pViewPropertiesAction) {
      m_pViewPropertiesAction->setChecked(true);
    }
    if (m_pScene && !m_pScene->selectedItems().isEmpty()) {
      m_pProperties->setItem(m_pScene->selectedItems().first());
    } else {
      m_pProperties->setItem(nullptr);
    }
  }
}

void MainWindow::onSelectionChanged() {
  if (!m_pProperties->isVisible()) {
    return;
  }

  if (!m_pScene) {
    return;
  }

  QList<QGraphicsItem*> sel = m_pScene->selectedItems();
  if (sel.isEmpty()) {
    QTimer::singleShot(Constants::SelectionDebounceDelay, this, [this]() {
      if (m_pScene && m_pScene->selectedItems().isEmpty()) {
        m_pProperties->setItem(nullptr);
      }
    });
  } else {
    m_pProperties->setItem(sel.first());
    m_pProperties->raise();
  }
}

void MainWindow::onSceneChanged(const QList<QRectF>& region) {
  Q_UNUSED(region);
  if (m_pScene) {
    if (!m_isModified) {
      setModified(true);
    }
    if (m_pProperties && m_pProperties->isVisible() && !m_pScene->selectedItems().isEmpty()) {
      m_pProperties->refreshValues();
    }
  }
}

void MainWindow::addApp(QAction* action) {
  if (!m_pScene) {
    return;
  }

  QString type = action->text();
  double w = 400, h = 300;
  if (type == "Browser") {
    w = 800;
    h = 600;
  } else if (type == "Terminal") {
    w = 600;
    h = 400;
  }

  ResizableAppItem* item = m_pScene->addAppItem(type, QRectF(0, 0, w, h));
  QRectF safe = m_pScene->getWorkingArea();
  int startX = safe.left() + 20;
  int startY = safe.top() + 20;
  if (m_pScene->isGridEnabled()) {
    startX = std::round(startX / (double)m_pScene->gridSize()) * m_pScene->gridSize();
    startY = std::round(startY / (double)m_pScene->gridSize()) * m_pScene->gridSize();
  }
  item->setPos(startX, startY);
}

void MainWindow::addZone() {
  if (!m_pScene) {
    return;
  }

  ZoneItem* zone = new ZoneItem(QRectF(0, 0, 400, 400));
  QRectF safe = m_pScene->getWorkingArea();
  int startX = safe.left() + 50;
  int startY = safe.top() + 50;
  zone->setPos(startX, startY);
  m_pScene->addItem(zone);

  statusBar()->showMessage("Zone added.", Constants::StatusMessageDuration);
}

void MainWindow::removeWindow() {
  if (!m_pScene) {
    return;
  }

  QList<QGraphicsItem*> selected = m_pScene->selectedItems();

  if (selected.isEmpty()) {
    return;
  }

  for (auto item : selected) {
    if (item->type() == Constants::Item::AppItem || item->type() == Constants::Item::ZoneItem || item->type() == Constants::Item::GuideItem) {
      m_pScene->removeItem(item);
      delete item;
    }
  }
}

void MainWindow::groupItems() {
  if (!m_pScene) {
    return;
  }

  QList<QGraphicsItem*> selected = m_pScene->selectedItems();

  QList<QGraphicsItem*> itemsToGroup;
  QList<SnappingItemGroup*> groupsToMerge;

  for (QGraphicsItem* item : selected) {
    if (item->type() == Constants::Item::GroupItem) {
      SnappingItemGroup* grp = static_cast<SnappingItemGroup*>(item);
      groupsToMerge.append(grp);
    } else if (item->type() == Constants::Item::AppItem || item->type() == Constants::Item::ZoneItem) {
      itemsToGroup.append(item);
    }
  }

  if (groupsToMerge.size() == 1 && !itemsToGroup.isEmpty()) {
    SnappingItemGroup* existingGroup = groupsToMerge.first();
    for (QGraphicsItem* item : itemsToGroup) {
      item->setSelected(false);
      existingGroup->addToGroup(item);
    }
    existingGroup->update();
    statusBar()->showMessage("Added item(s) to existing group.", Constants::StatusMessageDuration);
    return;
  }

  if (groupsToMerge.size() > 1) {
    for (SnappingItemGroup* grp : groupsToMerge) {
      QList<QGraphicsItem*> children = grp->childItems();
      m_pScene->destroyItemGroup(grp);
      itemsToGroup.append(children);
    }
  }

  if (itemsToGroup.size() >= 2) {
    SnappingItemGroup* group = new SnappingItemGroup(m_pScene);
    for (QGraphicsItem* item : itemsToGroup) {
      item->setSelected(false);
      group->addToGroup(item);
    }
    m_pScene->addItem(group);
    group->setSelected(true);
    statusBar()->showMessage("Items grouped.", Constants::StatusMessageDuration);
  } else {
    statusBar()->showMessage("Select at least 2 items to group.", Constants::StatusMessageDuration);
  }
}

void MainWindow::ungroupItems() {
  if (!m_pScene) {
    return;
  }

  QList<QGraphicsItem*> selected = m_pScene->selectedItems();
  if (selected.isEmpty()) {
    return;
  }

  bool any = false;
  for (auto item : selected) {
    if (item->type() == Constants::Item::GroupItem) {
      SnappingItemGroup* group = static_cast<SnappingItemGroup*>(item);
      m_pScene->destroyItemGroup(group);
      any = true;
    }
  }

  if (any)
    statusBar()->showMessage("Ungrouped items.", Constants::StatusMessageDuration);
}

void MainWindow::toggleLock() {
  if (!m_pScene) {
    return;
  }

  QList<QGraphicsItem*> selected = m_pScene->selectedItems();
  for (auto item : selected) {
    if (item->type() == Constants::Item::AppItem) {
      ResizableAppItem* app = static_cast<ResizableAppItem*>(item);
      app->setLocked(!app->isLocked());
    }
  }
}

void MainWindow::setWallpaper() {
  if (!m_pScene) {
    return;
  }

  QString fileName = QFileDialog::getOpenFileName(this, "Select Wallpaper", "", "Images (*.png *.jpg *.jpeg *.bmp)");
  if (fileName.isEmpty()) {
    return;
  }

  QPixmap pix(fileName);
  if (pix.isNull()) {
    return;
  }
  m_pScene->setWallpaper(pix);
  statusBar()->showMessage("Wallpaper loaded.", Constants::StatusMessageDuration);
}

QString MainWindow::getTemplateXml(const QString& name) {
  if (name == "Coding") {
    return R"(<Layout><App name="IDE" x="0" y="30" width="1150" height="1010" /><App name="Terminal" x="1150" y="30" width="770" height="505" /><App name="Browser" x="1150" y="535" width="770" height="505" /></Layout>)";
  } else if (name == "Streaming") {
    return R"(<Layout><App name="OBS" x="0" y="30" width="480" height="1010" /><App name="Game" x="480" y="30" width="960" height="1010" /><App name="Chat" x="1440" y="30" width="480" height="1010" /></Layout>)";
  }
  return QString();
}

void MainWindow::applyTemplate(QAction* action) {
  if (!m_pScene) {
    return;
  }

  if (!maybeSave()) {
    return;
  }

  QString presetName = action->text();
  QString xmlContent = getTemplateXml(presetName);
  if (!xmlContent.isEmpty() && LayoutSerializer::loadFromXml(m_pScene, xmlContent)) {
    setModified(false);
    statusBar()->showMessage("Applied template: " + presetName, Constants::StatusMessageDuration);
  }
}

bool MainWindow::saveLayout() {
  if (!m_pScene) {
    return false;
  }
  QString fileName = QFileDialog::getSaveFileName(this, "Save Layout", "", "XML Files (*.xml)");
  if (fileName.isEmpty()) {
    return false;
  }

  if (LayoutSerializer::save(m_pScene, fileName)) {
    setModified(false);
    statusBar()->showMessage("Layout saved.", Constants::StatusMessageDuration);
    return true;
  } else {
    statusBar()->showMessage("Failed to save layout.", Constants::StatusMessageDuration);
    return false;
  }
}

void MainWindow::loadLayout() {
  if (!maybeSave()) {
    return;
  }

  QString fileName = QFileDialog::getOpenFileName(this, "Load Layout", "", "XML Files (*.xml)");
  if (fileName.isEmpty()) {
    return;
  }

  if (!m_pScene) {
    m_pScene = new LayoutScene(0, 0, 1920, 1080, this);
    m_pScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    m_pView->setScene(m_pScene);
    connect(m_pScene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_pScene, &QGraphicsScene::changed, this, &MainWindow::onSceneChanged);
    updateInterfaceState();
  }

  connectSceneSignals();

  if (LayoutSerializer::load(m_pScene, fileName)) {
    setModified(false);
    statusBar()->showMessage("Layout loaded from file.", Constants::StatusMessageDuration);

    // Sync UI with loaded scene
    if (m_pTopBarSpin) {
      m_pTopBarSpin->blockSignals(true);
      m_pTopBarSpin->setValue(m_pScene->topBarHeight());
      m_pTopBarSpin->blockSignals(false);
    }
    if (m_pBotBarSpin) {
      m_pBotBarSpin->blockSignals(true);
      m_pBotBarSpin->setValue(m_pScene->bottomBarHeight());
      m_pBotBarSpin->blockSignals(false);
    }
  } else {
    statusBar()->showMessage("Failed to load layout.", Constants::StatusMessageDuration);
  }
}

void MainWindow::alignLeft() {
  if (m_pScene) {
    m_pScene->alignSelectionLeft();
  }
}

void MainWindow::alignRight() {
  if (m_pScene) {
    m_pScene->alignSelectionRight();
  }
}

void MainWindow::alignTop() {
  if (m_pScene) {
    m_pScene->alignSelectionTop();
  }
}

void MainWindow::alignBottom() {
  if (m_pScene) {
    m_pScene->alignSelectionBottom();
  }
}

void MainWindow::alignCenterH() {
  if (m_pScene) {
    m_pScene->alignSelectionCenterH();
  }
}

void MainWindow::alignCenterV() {
  if (m_pScene) {
    m_pScene->alignSelectionCenterV();
  }
}

void MainWindow::distributeH() {
  if (m_pScene) {
    m_pScene->distributeSelectionH();
  }
}

void MainWindow::distributeV() {
  if (m_pScene) {
    m_pScene->distributeSelectionV();
  }
}

void MainWindow::createMenuBar() {
  QMenu* viewMenu = menuBar()->addMenu("View");

  m_pViewPropertiesAction = viewMenu->addAction("Properties Window");
  m_pViewPropertiesAction->setCheckable(true);
  m_pViewPropertiesAction->setShortcut(QKeySequence("F2"));
  connect(m_pViewPropertiesAction, &QAction::triggered, this, &MainWindow::toggleProperties);

  QAction* settAct = viewMenu->addAction("Settings...");
  connect(settAct, &QAction::triggered, this, &MainWindow::openSettings);
}

void MainWindow::createFloatingToolbar() {
  m_floatingToolbar = new QWidget(m_pView);  // Make the View its parent so it floats inside it!
  m_floatingToolbar->setObjectName("FloatingToolbar");
  m_floatingToolbar->setStyleSheet(
      "#FloatingToolbar { background-color: rgba(30, 30, 30, 200); border-radius: 10px; }"
      "QPushButton { color: white; background: transparent; border: 1px solid #555; border-radius: 5px; padding: 5px 10px; }"
      "QPushButton:checked { background: #E53935; border: 1px solid #ff5252; }");

  QHBoxLayout* toolLayout = new QHBoxLayout(m_floatingToolbar);
  toolLayout->setContentsMargins(10, 5, 10, 5);

  m_pBtnEdit = new QPushButton("Move/Crop");
  m_pBtnDraw = new QPushButton("Draw");
  m_pBtnLaser = new QPushButton("Laser");
  m_pBtnClear = new QPushButton("Clear");

  // Make them act like radio buttons
  m_pBtnEdit->setCheckable(true);
  m_pBtnDraw->setCheckable(true);
  m_pBtnLaser->setCheckable(true);
  m_pBtnEdit->setChecked(true);  // Default state

  toolLayout->addWidget(m_pBtnEdit);
  toolLayout->addWidget(m_pBtnDraw);
  toolLayout->addWidget(m_pBtnLaser);
  toolLayout->addWidget(m_pBtnClear);

  // Position it at the top center of the view
  m_floatingToolbar->setGeometry(m_pView->width() / 2 - 150, 20, 300, 40);
  m_floatingToolbar->raise();
  m_floatingToolbar->show();

  m_floatingToolbar->installEventFilter(this);
}

void MainWindow::createToolbar() {
  if (m_pToolbar) {
    removeToolBar(m_pToolbar);
    delete m_pToolbar;
    m_pToolbar = nullptr;
  }

  OfficeToolbar* ribbon = new OfficeToolbar(this);
  m_pToolbar = addToolBar("Ribbon");
  m_pToolbar->setMovable(false);
  m_pToolbar->addWidget(ribbon);

  QString controlStyle =
      QString(Constants::Style::Menu)
          .arg(QColor(Constants::Color::RibbonBg).name(), QColor(Constants::Color::RibbonBorder).name(), QColor(Constants::Color::RibbonText).name(),
               QColor(Constants::Color::RibbonHighlight).name(), QColor(Constants::Color::RibbonHighlightText).name());

  // --- SECTION: FILE ---
  RibbonSection* fileSec = ribbon->addSection("File", QIcon(":/icons/section-file.svg"));

  QAction* newAct = new QAction(QIcon(":/icons/add.svg"), "New", this);
  newAct->setShortcut(QKeySequence::New);
  connect(newAct, &QAction::triggered, this, &MainWindow::newLayout);
  fileSec->addLargeButton(new RibbonButton(newAct, RibbonButton::Large));

  QAction* saveAct = new QAction(QIcon(":/icons/save.svg"), "Save", this);
  saveAct->setShortcut(QKeySequence::Save);
  connect(saveAct, &QAction::triggered, this, &MainWindow::saveLayout);
  fileSec->addLargeButton(new RibbonButton(saveAct, RibbonButton::Large));

  QAction* loadAct = new QAction(QIcon(":/icons/load.svg"), "Load", this);
  loadAct->setShortcut(QKeySequence::Open);
  connect(loadAct, &QAction::triggered, this, &MainWindow::loadLayout);
  fileSec->addLargeButton(new RibbonButton(loadAct, RibbonButton::Large));

  QAction* closeAct = new QAction(QIcon(":/icons/close.svg"), "Close", this);
  closeAct->setShortcut(QKeySequence::Close);
  connect(closeAct, &QAction::triggered, this, &MainWindow::closeLayout);
  fileSec->addLargeButton(new RibbonButton(closeAct, RibbonButton::Large));

  // --- SECTION: INSERT ---
  m_pSectionInsert = ribbon->addSection("Insert", QIcon(":/icons/section-insert.svg"));

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
  m_pSectionInsert->addLargeButton((RibbonButton*)addBtn);

  QAction* zoneAct = new QAction(QIcon(":/icons/zone.svg"), "Zone", this);
  connect(zoneAct, &QAction::triggered, this, &MainWindow::addZone);
  m_pSectionInsert->addLargeButton(new RibbonButton(zoneAct, RibbonButton::Large));

  QAction* rmAct = new QAction(QIcon(":/icons/remove.svg"), "Delete", this);
  rmAct->setShortcuts({QKeySequence::Delete, QKeySequence(Qt::Key_Backspace)});
  connect(rmAct, &QAction::triggered, this, &MainWindow::removeWindow);
  m_pSectionInsert->addLargeButton(new RibbonButton(rmAct, RibbonButton::Large));

  // ==========================================
  // 1. MAIN RIBBON: MIRROR STREAM BUTTON
  // ==========================================
  QAction* mirrorAct = new QAction(QIcon(":/icons/mirror.svg"), "Mirror Stream", this);
  connect(mirrorAct, &QAction::triggered, this, [this]() {
    if (!m_pScene)
      return;

    // Hijack the mouse globally and change to a crosshair.
    // The unified eventFilter will catch the very next click and route it
    // to m_selector->captureWindowUnderCursor();
    m_pView->viewport()->grabMouse(Qt::CrossCursor);
    m_isSelectingWindow = true;
  });
  // Add to your specific Ribbon section
  m_pSectionInsert->addLargeButton(new RibbonButton(mirrorAct, RibbonButton::Large));

  // ==========================================
  // 2. MAIN RIBBON: PROJECTOR/OUTPUT BUTTON
  // ==========================================
  QAction* projAct = new QAction(QIcon(":/icons/output.svg"), "Send to Output", this);
  connect(projAct, &QAction::triggered, this, [this]() {
    if (!m_pScene)
      return;

    if (!m_pProjector) {
      m_pProjector = new ProjectorWindow(m_pScene);
    }

    QList<QScreen*> screens = QApplication::screens();
    if (screens.size() > 1) {
      // Move to the second monitor and fullscreen
      QRect screenGeo = screens[1]->geometry();
      m_pProjector->move(screenGeo.topLeft());
      m_pProjector->resize(screenGeo.size());
      m_pProjector->showFullScreen();
    } else {
      // Fallback for single-monitor testing
      m_pProjector->resize(1280, 720);
      m_pProjector->show();
    }
  });
  m_pSectionInsert->addLargeButton(new RibbonButton(projAct, RibbonButton::Large));

  QAction* wallAct = new QAction(QIcon(":/icons/image.svg"), "Wallpaper", this);
  connect(wallAct, &QAction::triggered, this, &MainWindow::setWallpaper);
  m_pSectionInsert->addWidget(new RibbonButton(wallAct, RibbonButton::Small), 0, 5);

  QToolButton* tempBtn = new RibbonButton(new QAction(QIcon(":/icons/template.svg"), "Templates", this), RibbonButton::Small);
  tempBtn->setPopupMode(QToolButton::InstantPopup);
  QMenu* tempMenu = new QMenu(tempBtn);
  tempMenu->addAction("Coding");
  tempMenu->addAction("Streaming");
  tempMenu->setStyleSheet(controlStyle);
  tempBtn->setMenu(tempMenu);
  connect(tempMenu, &QMenu::triggered, this, &MainWindow::applyTemplate);
  m_pSectionInsert->addWidget(tempBtn, 1, 5);

  // --- SECTION: ARRANGE ---
  m_pSectionArrange = ribbon->addSection("Arrange", QIcon(":/icons/section-arrange.svg"));

  QAction* grpAct = new QAction(QIcon(":/icons/group.svg"), "Group", this);
  grpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
  connect(grpAct, &QAction::triggered, this, &MainWindow::groupItems);
  m_pSectionArrange->addWidget(new RibbonButton(grpAct, RibbonButton::Small), 0, 0);

  QAction* ungrpAct = new QAction(QIcon(":/icons/ungroup.svg"), "Ungroup", this);
  ungrpAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
  connect(ungrpAct, &QAction::triggered, this, &MainWindow::ungroupItems);
  m_pSectionArrange->addWidget(new RibbonButton(ungrpAct, RibbonButton::Small), 1, 0);

  QAction* lockAct = new QAction(QIcon(":/icons/lock.svg"), "Lock", this);
  lockAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
  connect(lockAct, &QAction::triggered, this, &MainWindow::toggleLock);
  m_pSectionArrange->addWidget(new RibbonButton(lockAct, RibbonButton::Small), 2, 0);

  // --- SECTION: ALIGNMENT ---
  m_pSectionAlign = ribbon->addSection("Alignment", QIcon(":/icons/section-alignment.svg"));

  QAction* al = new QAction(QIcon(":/icons/align-left.svg"), "Left", this);
  connect(al, &QAction::triggered, this, &MainWindow::alignLeft);
  m_pSectionAlign->addWidget(new RibbonButton(al, RibbonButton::Small), 0, 0);

  QAction* ac = new QAction(QIcon(":/icons/align-center-h.svg"), "Center", this);
  connect(ac, &QAction::triggered, this, &MainWindow::alignCenterH);
  m_pSectionAlign->addWidget(new RibbonButton(ac, RibbonButton::Small), 1, 0);

  QAction* ar = new QAction(QIcon(":/icons/align-right.svg"), "Right", this);
  connect(ar, &QAction::triggered, this, &MainWindow::alignRight);
  m_pSectionAlign->addWidget(new RibbonButton(ar, RibbonButton::Small), 2, 0);

  QAction* at = new QAction(QIcon(":/icons/align-top.svg"), "Top", this);
  connect(at, &QAction::triggered, this, &MainWindow::alignTop);
  m_pSectionAlign->addWidget(new RibbonButton(at, RibbonButton::Small), 0, 1);

  QAction* am = new QAction(QIcon(":/icons/align-center-v.svg"), "Middle", this);
  connect(am, &QAction::triggered, this, &MainWindow::alignCenterV);
  m_pSectionAlign->addWidget(new RibbonButton(am, RibbonButton::Small), 1, 1);

  QAction* ab = new QAction(QIcon(":/icons/align-bottom.svg"), "Bottom", this);
  connect(ab, &QAction::triggered, this, &MainWindow::alignBottom);
  m_pSectionAlign->addWidget(new RibbonButton(ab, RibbonButton::Small), 2, 1);

  // --- SECTION: VIEW ---
  m_pSectionView = ribbon->addSection("View", QIcon(":/icons/section-view.svg"));

  QWidget* gridContainer = new QWidget();
  gridContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* gridLayout = new QHBoxLayout(gridContainer);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setSpacing(5);

  QAction* gridAct = new QAction(QIcon(":/icons/grid.svg"), "Grid", this);
  gridAct->setCheckable(true);
  connect(gridAct, &QAction::toggled, this, &MainWindow::toggleGrid);
  RibbonButton* gridBtn = new RibbonButton(gridAct, RibbonButton::Small);
  gridLayout->addWidget(gridBtn);

  m_pGridSlider = new QSlider(Qt::Horizontal);
  m_pGridSlider->setRange(10, 200);
  m_pGridSlider->setValue(50);
  m_pGridSlider->setFixedWidth(80);
  m_pGridSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_pGridSlider->setEnabled(false);

  connect(m_pGridSlider, &QSlider::valueChanged, this, &MainWindow::onGridSizeChanged);
  gridLayout->addWidget(m_pGridSlider);

  m_pGridLabel = new QLabel("50px");
  gridLayout->addWidget(m_pGridLabel);

  m_pSectionView->addWidget(gridContainer, 0, 0);

  QString spinStyle =
      QString(Constants::Style::SpinBox)
          .arg(QColor(Constants::Color::SpinBoxLightBg).name(), QColor(Constants::Color::SpinBoxLightText).name(),
               QColor(Constants::Color::SpinBoxLightBorder).name(), QColor(Constants::Color::SpinBoxLightSelection).name(),
               QColor(Constants::Color::SpinBoxLightButton).name(), QColor(Constants::Color::SpinBoxLightHover).name(),
               QColor(Constants::Color::SpinBoxLightPressed).name(), Constants::Color::IconSpinUpDark, Constants::Color::IconSpinDownDark,
               QColor(Constants::Color::SpinBoxLightDisabledText).name(), QColor(Constants::Color::SpinBoxLightDisabledBg).name());

  m_pTopBarSpin = new QSpinBox();
  m_pTopBarSpin->setRange(0, 200);
  m_pTopBarSpin->setValue(30);
  m_pTopBarSpin->setSuffix(" px");
  m_pTopBarSpin->setFixedWidth(60);
  m_pTopBarSpin->setStyleSheet(spinStyle);
  m_pTopBarSpin->setKeyboardTracking(false);
  connect(m_pTopBarSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTopBarChanged);

  m_pBotBarSpin = new QSpinBox();
  m_pBotBarSpin->setRange(0, 200);
  m_pBotBarSpin->setValue(40);
  m_pBotBarSpin->setSuffix(" px");
  m_pBotBarSpin->setFixedWidth(60);
  m_pBotBarSpin->setStyleSheet(spinStyle);
  m_pBotBarSpin->setKeyboardTracking(false);
  connect(m_pBotBarSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBotBarChanged);

  QWidget* barsContainer = new QWidget();
  barsContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* barsLayout = new QHBoxLayout(barsContainer);
  barsLayout->setContentsMargins(0, 0, 0, 0);
  barsLayout->setSpacing(5);

  barsLayout->addWidget(new QLabel("Top:"));
  barsLayout->addWidget(m_pTopBarSpin);
  barsLayout->addWidget(new QLabel("Bot:"));
  barsLayout->addWidget(m_pBotBarSpin);

  m_pSectionView->addWidget(barsContainer, 1, 0);

  ribbon->addSpacer();
}
