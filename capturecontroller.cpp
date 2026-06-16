#include "capturecontroller.h"

#include <QGraphicsView>
#include <QMenu>
#include <QMouseEvent>
#include <QStringList>

#include <cmath>

#include "layoutscene.h"
#include "mirroredappitem.h"
#include "resizableappitem.h"
#include "windowselector.h"

CaptureController::CaptureController(QGraphicsView* view, QObject* parent) : QObject(parent), m_pView(view) {
  m_selector = new WindowSelector(this);
  connect(m_selector, &WindowSelector::windowSelectedForGStreamer, this, [this](const QString& captureSource) { addMirroredApp(captureSource); });
}

void CaptureController::setScene(LayoutScene* scene) {
  m_pScene = scene;
}

void CaptureController::populateAddAppMenu(QMenu* menu) {
  menu->clear();

  // Live windows: mirror the picked one (shown with its own icon).
  const QList<WindowSelector::WindowEntry> windows = m_selector->listWindows();
  if (windows.isEmpty()) {
    QAction* none = menu->addAction("No open windows found");
    none->setEnabled(false);
  } else {
    for (const WindowSelector::WindowEntry& win : windows) {
      QAction* act = menu->addAction(win.icon, win.title);
      const QString source = win.captureSource;
      connect(act, &QAction::triggered, this, [this, source]() { addMirroredApp(source); });
    }
  }

  menu->addSeparator();

  // Generic placeholders for designing a layout before the real apps are open.
  QMenu* placeholderMenu = menu->addMenu("Placeholder");
  const QStringList placeholders = {"Browser", "Terminal", "Music Player", "File Manager"};
  for (const QString& name : placeholders) {
    QAction* act = placeholderMenu->addAction(name);
    connect(act, &QAction::triggered, this, [this, name]() { addPlaceholderApp(name); });
  }
}

void CaptureController::beginWindowPick() {
  if (!m_pScene) {
    return;
  }
  // Grab the mouse and switch to a crosshair; handleViewportEvent catches the
  // next click and resolves the window under the cursor.
  m_pView->viewport()->grabMouse(Qt::CrossCursor);
  m_isSelectingWindow = true;
}

bool CaptureController::handleViewportEvent(QObject* watched, QEvent* event) {
  if (!m_isSelectingWindow || watched != m_pView->viewport()) {
    return false;
  }
  if (event->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
    m_pView->viewport()->releaseMouse();
    m_isSelectingWindow = false;
    m_selector->captureWindowUnderCursor();
    return true;
  }
  return false;
}

void CaptureController::addMirroredApp(const QString& captureSource) {
  if (!m_pScene) {
    return;
  }

  // Pass the OS-specific capture string into the item.
  MirroredAppItem* item = new MirroredAppItem(captureSource);
  item->initActions();
  connect(item, &ResizableAppItem::propertiesRequested, this, &CaptureController::propertiesRequested);

  m_pScene->addItem(item);
  m_pScene->clearSelection();
  item->setSelected(true);
}

void CaptureController::addPlaceholderApp(const QString& type) {
  if (!m_pScene) {
    return;
  }

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
