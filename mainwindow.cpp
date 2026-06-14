#include "mainwindow.h"
#include "constants.h"
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
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
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

#include <QOpenGLWidget>

#include <cmath>

#include "formatpanel.h"
#include "presentercontroller.h"
#include "outputrecorder.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_pScene(nullptr), m_pToolbar(nullptr), m_pTopBarSpin(nullptr), m_pBotBarSpin(nullptr), m_isModified(false) {
  resize(1400, 900);
  setWindowTitle("Layout Editor");

  m_pView = new QGraphicsView(this);
  m_pView->setViewport(new QOpenGLWidget(this));
  m_pView->setBackgroundBrush(QColor(Constants::Color::ViewBackgroundEmpty));
  m_pView->setRenderHint(QPainter::Antialiasing);
  m_pView->setFrameShape(QFrame::NoFrame);
  m_pView->setDragMode(QGraphicsView::RubberBandDrag);
  m_pView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
  m_pView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_pView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_pView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
  m_pView->setOptimizationFlags(QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing);

  // The OpenGL viewport is only painted when the view has a scene, so without
  // a layout it would stay transparent. Give it an empty placeholder scene.
  m_pEmptyScene = new QGraphicsScene(this);
  m_pView->setScene(m_pEmptyScene);

  m_pView->viewport()->setMouseTracking(true);
  m_pView->viewport()->installEventFilter(this);

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

  m_pHRuler->installEventFilter(this);
  m_pVRuler->installEventFilter(this);

  m_pProperties = new PropertiesDialog(m_pView);
  m_pProperties->hide();
  m_pProperties->move(20, 20);

  createToolbar();
  createMenuBar();
  m_pPresenter = new PresenterController(m_pView, this);
  m_pOutputRecorder = new OutputRecorder(this);

  // Escape cancels the active draw/laser tool and returns to the Move tool.
  QAction* resetToolAct = new QAction(this);
  resetToolAct->setShortcut(QKeySequence(Qt::Key_Escape));
  connect(resetToolAct, &QAction::triggered, this, [this]() {
    if (m_pPresenter) {
      m_pPresenter->resetToEditMode();
    }
  });
  addAction(resetToolAct);

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

  connect(m_selector, &WindowSelector::windowSelectedForGStreamer, this, [this](const QString& captureSource) { addMirroredApp(captureSource); });
}

void MainWindow::addMirroredApp(const QString& captureSource) {
  if (!m_pScene) {
    return;
  }

  // Pass the OS-specific capture string into the item.
  MirroredAppItem* item = new MirroredAppItem(captureSource);

  item->initActions();
  connect(item, &ResizableAppItem::propertiesRequested, this, &MainWindow::showProperties);

  m_pScene->addItem(item);
  m_pScene->clearSelection();
  item->setSelected(true);
}

MainWindow::~MainWindow() {
  if (m_pScene) {
    m_pScene->disconnect(this);
  }
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (maybeSave()) {
    if (m_pProjector) {
      m_pProjector->close();
    }

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
  connect(m_pScene, &LayoutScene::itemPropertiesRequested, this, &MainWindow::showProperties);
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
    m_pView->setBackgroundBrush(palette().window());
  } else {
    m_pView->setBackgroundBrush(QColor(Constants::Color::ViewBackgroundEmpty));
  }
}

void MainWindow::showProperties(QGraphicsItem* item) {
  if (!m_pProperties->isVisible()) {
    m_pProperties->show();
  }

  m_pProperties->raise();
  m_pProperties->activateWindow();
  if (item) {
    m_pProperties->setItem(item);
  } else if (!m_pScene->selectedItems().isEmpty()) {
    const QList<QGraphicsItem*> selectedItems = m_pScene->selectedItems();
    m_pProperties->setItem(selectedItems.first());
  }
}

void MainWindow::toggleFullScreen() {
  if (isFullScreen()) {
    if (m_wasMaximized) {
      showMaximized();
    } else {
      showNormal();
    }
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

void MainWindow::stopOutputRecording() {
  if (m_pOutputRecorder && m_pOutputRecorder->isRecording()) {
    m_pOutputRecorder->stop();
    if (m_pRecordAction) {
      m_pRecordAction->setIcon(QIcon(":/icons/record.svg"));
      m_pRecordAction->setText("Record Output");
    }
    statusBar()->showMessage("Recording stopped.", Constants::StatusMessageDuration);
  }
}

void MainWindow::newLayout() {
  if (!maybeSave()) {
    return;
  }

  NewLayoutDialog dlg(this);
  if (dlg.exec() == QDialog::Accepted) {
    if (m_pScene) {
      stopOutputRecording();
      m_pScene->deleteLater();
    }

    const int w = dlg.selectedWidth();
    const int h = dlg.selectedHeight();

    m_pScene = new LayoutScene(0, 0, w, h, this);
    m_pScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    m_pScene->setTopBarHeight(SettingsDialog::getTopBarHeight());
    m_pScene->setBottomBarHeight(SettingsDialog::getBottomBarHeight());

    m_pPresenter->setScene(m_pScene);

    connectSceneSignals();

    m_pView->setScene(m_pScene);
    if (m_pProjector) {
      m_pProjector->setScene(m_pScene);
    }
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

void MainWindow::closeLayout() {
  if (!maybeSave()) {
    return;
  }

  if (m_pProjector) {
    m_pProjector->close();
  }

  if (!m_pScene) {
    return;
  }

  stopOutputRecording();

  m_pView->setScene(m_pEmptyScene);

  // Tear the drawing manager down while its scene is still alive.
  m_pPresenter->setScene(nullptr);

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
      const int newTop = SettingsDialog::getTopBarHeight();
      const int newBot = SettingsDialog::getBottomBarHeight();
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

  if (event->type() == QEvent::MouseMove) {
    QPoint viewPos = m_pView->viewport()->mapFromGlobal(static_cast<QMouseEvent*>(event)->globalPosition().toPoint());
    m_pHRuler->updateCursorPos(viewPos);
    m_pVRuler->updateCursorPos(viewPos);
  }

  if (m_pPresenter && m_pPresenter->handleViewportEvent(watched, event)) {
    return true;
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
      const QList<QGraphicsItem*> selectedItems = m_pScene->selectedItems();
      m_pProperties->setItem(selectedItems.first());
    } else {
      m_pProperties->setItem(nullptr);
    }
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

  for (auto item : std::as_const(selected)) {
    if (item->type() == Constants::Item::AppItem || item->type() == Constants::Item::ZoneItem || item->type() == Constants::Item::GuideItem ||
        item->type() == Constants::Item::MirroredAppItem) {
      m_pScene->removeItem(item);
      delete item;
    } else if (item->type() == QGraphicsPathItem::Type || item->type() == QGraphicsRectItem::Type || item->type() == QGraphicsEllipseItem::Type) {
      m_pPresenter->removeDrawnItem(item);
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

  for (QGraphicsItem* item : std::as_const(selected)) {
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
  for (auto item : std::as_const(selected)) {
    if (item->type() == Constants::Item::GroupItem) {
      SnappingItemGroup* group = static_cast<SnappingItemGroup*>(item);
      m_pScene->destroyItemGroup(group);
      any = true;
    }
  }

  if (any) {
    statusBar()->showMessage("Ungrouped items.", Constants::StatusMessageDuration);
  }
}

void MainWindow::align() {
  if (m_pScene) {
    m_pScene->alignSelection(static_cast<LayoutScene::Alignment>(sender()->property("alignment").toInt()));
  }
}

void MainWindow::toggleLock() {
  if (!m_pScene) {
    return;
  }

  QList<QGraphicsItem*> selected = m_pScene->selectedItems();
  for (auto item : std::as_const(selected)) {
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

  QString fileName =
      QFileDialog::getOpenFileName(this, "Select Wallpaper", "", "Images (*.png *.jpg *.jpeg *.bmp)", nullptr, QFileDialog::DontUseNativeDialog);
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

  const QString presetName = action->text();
  const QString xmlContent = getTemplateXml(presetName);
  if (!xmlContent.isEmpty() && LayoutSerializer::loadFromXml(m_pScene, xmlContent)) {
    setModified(false);
    statusBar()->showMessage("Applied template: " + presetName, Constants::StatusMessageDuration);
  }
}

bool MainWindow::saveLayout() {
  if (!m_pScene) {
    return false;
  }
  QString fileName = QFileDialog::getSaveFileName(this, "Save Layout", "", "XML Files (*.xml)", nullptr, QFileDialog::DontUseNativeDialog);
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

  QString fileName = QFileDialog::getOpenFileName(this, "Load Layout", "", "XML Files (*.xml)", nullptr, QFileDialog::DontUseNativeDialog);
  if (fileName.isEmpty()) {
    return;
  }

  if (!m_pScene) {
    m_pScene = new LayoutScene(0, 0, 1920, 1080, this);
    m_pScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    m_pView->setScene(m_pScene);
    m_pPresenter->setScene(m_pScene);
    connectSceneSignals();
    updateInterfaceState();
  }

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
  addMenu->setStyleSheet(controlStyle);
  addBtn->setMenu(addMenu);

  // Rebuild the list of currently open windows each time the menu opens, so the
  // user can mirror a live app picked from the list (with its own icon).
  connect(addMenu, &QMenu::aboutToShow, this, [this, addMenu]() {
    addMenu->clear();

    const QList<WindowSelector::WindowEntry> windows = m_selector->listWindows();
    if (windows.isEmpty()) {
      QAction* none = addMenu->addAction("No open windows found");
      none->setEnabled(false);
    } else {
      for (const WindowSelector::WindowEntry& win : windows) {
        QAction* act = addMenu->addAction(win.icon, win.title);
        const QString source = win.captureSource;
        connect(act, &QAction::triggered, this, [this, source]() { addMirroredApp(source); });
      }
    }

    addMenu->addSeparator();

    // Generic placeholders for designing a layout before the real apps are open.
    QMenu* placeholderMenu = addMenu->addMenu("Placeholder");
    const QStringList placeholders = {"Browser", "Terminal", "Music Player", "File Manager"};
    for (const QString& name : placeholders) {
      QAction* act = placeholderMenu->addAction(name);
      connect(act, &QAction::triggered, this, [this, act]() { addApp(act); });
    }
  });

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
  QToolButton* projBtn = new RibbonButton(new QAction(QIcon(":/icons/output.svg"), "Send to Output", this), RibbonButton::Large);
  projBtn->setPopupMode(QToolButton::InstantPopup);

  QMenu* projMenu = new QMenu(projBtn);
  projMenu->setStyleSheet(controlStyle);
  projBtn->setMenu(projMenu);

  // Dynamically build the monitor list right before the menu opens
  connect(projMenu, &QMenu::aboutToShow, this, [this, projMenu]() {
    projMenu->clear();

    QList<QScreen*> screens = QApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
      QScreen* screen = screens[i];

      // Format looks like: "Screen 2: DELL U2720Q (3840x2160)"
      const QString screenName =
          QStringLiteral("Screen %1: %2 (%3x%4)")
              .arg(QString::number(i + 1), screen->name(), QString::number(screen->geometry().width()), QString::number(screen->geometry().height()));

      QAction* screenAct = projMenu->addAction(screenName);
      connect(screenAct, &QAction::triggered, this, [this, screen]() {
        if (!m_pScene) {
          return;
        }
        if (!m_pProjector) {
          m_pProjector = new ProjectorWindow(m_pScene);
          m_pProjector->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        }

        // CRITICAL: Pull it out of fullscreen first before moving it to a new monitor!
        m_pProjector->showNormal();

        QRect screenGeo = screen->geometry();
        m_pProjector->move(screenGeo.topLeft());
        m_pProjector->resize(screenGeo.size());
        m_pProjector->showFullScreen();
      });
    }

    projMenu->addSeparator();

    // Keep a fallback windowed mode for easy local testing
    QAction* windowedAct = projMenu->addAction("Windowed Mode (Local Test)");
    connect(windowedAct, &QAction::triggered, this, [this]() {
      if (!m_pScene) {
        return;
      }
      if (!m_pProjector) {
        m_pProjector = new ProjectorWindow(m_pScene);
      }
      m_pProjector->showNormal();
      m_pProjector->resize(1280, 720);
      m_pProjector->show();
    });
  });

  m_pSectionInsert->addLargeButton((RibbonButton*)projBtn);

  // ==========================================
  // 3. MAIN RIBBON: STOP OUTPUT BUTTON
  // ==========================================
  QAction* stopProjAct = new QAction(QIcon(":/icons/stop_output.svg"), "Stop Output", this);

  connect(stopProjAct, &QAction::triggered, this, [this]() {
    if (m_pProjector) {
      m_pProjector->close();        // Safely close the window
      m_pProjector->deleteLater();  // Queue it for memory deletion
      m_pProjector = nullptr;       // Reset the pointer so we can launch it again later

      statusBar()->showMessage("Projector output stopped.", Constants::StatusMessageDuration);
    }
  });

  m_pSectionInsert->addLargeButton(new RibbonButton(stopProjAct, RibbonButton::Large));

  // ==========================================
  // 4. MAIN RIBBON: RECORD OUTPUT BUTTON
  // ==========================================
  m_pRecordAction = new QAction(QIcon(":/icons/record.svg"), "Record Output", this);

  connect(m_pRecordAction, &QAction::triggered, this, [this]() {
    if (!m_pOutputRecorder) {
      return;
    }

    if (m_pOutputRecorder->isRecording()) {
      m_pOutputRecorder->stop();
      m_pRecordAction->setIcon(QIcon(":/icons/record.svg"));
      m_pRecordAction->setText("Record Output");
      statusBar()->showMessage("Recording stopped.", Constants::StatusMessageDuration);
      return;
    }

    if (!m_pScene) {
      statusBar()->showMessage("Create or open a layout before recording.", Constants::StatusMessageDuration);
      return;
    }

    const QString fileName = QFileDialog::getSaveFileName(this, "Record Output To", "", "Video (*.mkv)", nullptr, QFileDialog::DontUseNativeDialog);
    if (fileName.isEmpty()) {
      return;
    }

    if (m_pOutputRecorder->start(m_pScene, fileName)) {
      m_pRecordAction->setIcon(QIcon(":/icons/stop-record.svg"));
      m_pRecordAction->setText("Stop Recording");
      statusBar()->showMessage("Recording output...", Constants::StatusMessageDuration);
    } else {
      statusBar()->showMessage("Failed to start recording.", Constants::StatusMessageDuration);
    }
  });

  m_pSectionInsert->addLargeButton(new RibbonButton(m_pRecordAction, RibbonButton::Large));

  QAction* wallAct = new QAction(QIcon(":/icons/image.svg"), "Wallpaper", this);
  connect(wallAct, &QAction::triggered, this, &MainWindow::setWallpaper);
  m_pSectionInsert->addWidget(new RibbonButton(wallAct, RibbonButton::Small), 0, 7);

  QToolButton* tempBtn = new RibbonButton(new QAction(QIcon(":/icons/template.svg"), "Templates", this), RibbonButton::Small);
  tempBtn->setPopupMode(QToolButton::InstantPopup);
  QMenu* tempMenu = new QMenu(tempBtn);
  tempMenu->addAction("Coding");
  tempMenu->addAction("Streaming");
  tempMenu->setStyleSheet(controlStyle);
  tempBtn->setMenu(tempMenu);
  connect(tempMenu, &QMenu::triggered, this, &MainWindow::applyTemplate);
  m_pSectionInsert->addWidget(tempBtn, 1, 7);

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
  al->setProperty("alignment", QVariant::fromValue(static_cast<int>(LayoutScene::Alignment::Left)));
  connect(al, &QAction::triggered, this, &MainWindow::align);
  m_pSectionAlign->addWidget(new RibbonButton(al, RibbonButton::Small), 0, 0);

  QAction* ac = new QAction(QIcon(":/icons/align-center-h.svg"), "Center", this);
  ac->setProperty("alignment", QVariant::fromValue(static_cast<int>(LayoutScene::Alignment::CenterH)));
  connect(ac, &QAction::triggered, this, &MainWindow::align);
  m_pSectionAlign->addWidget(new RibbonButton(ac, RibbonButton::Small), 1, 0);

  QAction* ar = new QAction(QIcon(":/icons/align-right.svg"), "Right", this);
  ar->setProperty("alignment", QVariant::fromValue(static_cast<int>(LayoutScene::Alignment::Right)));
  connect(ar, &QAction::triggered, this, &MainWindow::align);
  m_pSectionAlign->addWidget(new RibbonButton(ar, RibbonButton::Small), 2, 0);

  QAction* at = new QAction(QIcon(":/icons/align-top.svg"), "Top", this);
  at->setProperty("alignment", QVariant::fromValue(static_cast<int>(LayoutScene::Alignment::Top)));
  connect(at, &QAction::triggered, this, &MainWindow::align);
  m_pSectionAlign->addWidget(new RibbonButton(at, RibbonButton::Small), 0, 1);

  QAction* am = new QAction(QIcon(":/icons/align-center-v.svg"), "Middle", this);
  am->setProperty("alignment", QVariant::fromValue(static_cast<int>(LayoutScene::Alignment::CenterV)));
  connect(am, &QAction::triggered, this, &MainWindow::align);
  m_pSectionAlign->addWidget(new RibbonButton(am, RibbonButton::Small), 1, 1);

  QAction* ab = new QAction(QIcon(":/icons/align-bottom.svg"), "Bottom", this);
  ab->setProperty("alignment", QVariant::fromValue(static_cast<int>(LayoutScene::Alignment::Bottom)));
  connect(ab, &QAction::triggered, this, &MainWindow::align);
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

  const QString spinStyle =
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
  QVBoxLayout* barsLayout = new QVBoxLayout(barsContainer);
  barsLayout->setContentsMargins(0, 0, 0, 0);
  barsLayout->setSpacing(5);

  QHBoxLayout* topLayout = new QHBoxLayout();
  topLayout->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout* bottomLayout = new QHBoxLayout();
  bottomLayout->setContentsMargins(0, 0, 0, 0);

  topLayout->addWidget(new QLabel("Top:"));
  topLayout->addWidget(m_pTopBarSpin);
  bottomLayout->addWidget(new QLabel("Bot:"));
  bottomLayout->addWidget(m_pBotBarSpin);

  barsLayout->addLayout(topLayout);
  barsLayout->addLayout(bottomLayout);

  m_pSectionView->addWidget(barsContainer, 1, 0);

  m_pFormatPanel = new FormatPanel(ribbon, spinStyle, this);

  ribbon->addSpacer();
}

void MainWindow::onSelectionChanged() {
  if (!m_pScene) {
    return;
  }

  QList<QGraphicsItem*> sel = m_pScene->selectedItems();

  // The format panel shows itself for drawn shapes and reports whether it took
  // over, so we can suppress the properties dialog for those.
  const bool showFormat = m_pFormatPanel->updateForSelection(sel.isEmpty() ? nullptr : sel.first());

  // --- Existing Properties Dialog Logic ---
  if (!m_pProperties->isVisible())
    return;

  if (sel.isEmpty()) {
    QTimer::singleShot(Constants::SelectionDebounceDelay, this, [this]() {
      if (m_pScene && m_pScene->selectedItems().isEmpty()) {
        m_pProperties->setItem(nullptr);
      }
    });
  } else {
    // Only pass it to properties if it is not a shape (or pass it anyway if you want Geometry data)
    if (!showFormat) {
      m_pProperties->setItem(sel.first());
      m_pProperties->raise();
    } else {
      m_pProperties->setItem(nullptr);  // Prevent modifying shape geometry in properties if desired
    }
  }
}

