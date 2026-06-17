#include "capturecontroller.h"

#include <QCursor>
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
  connect(m_selector, &WindowSelector::windowPicked, this,
          [this](const WindowSelector::WindowEntry& entry) { addMirroredApp(entry.captureSource, entry.appClass, entry.title); });
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
      const QString appClass = win.appClass;
      const QString title = win.title;
      connect(act, &QAction::triggered, this, [this, source, appClass, title]() { addMirroredApp(source, appClass, title); });
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

void CaptureController::addMirroredApp(const QString& captureSource, const QString& appClass, const QString& appTitle) {
  if (!m_pScene) {
    return;
  }

  MirroredAppItem* item = new MirroredAppItem(captureSource, appClass, appTitle);
  item->initActions();
  wireMirror(item);

  m_pScene->addItem(item);
  m_pScene->clearSelection();
  item->setSelected(true);
}

MirroredAppItem* CaptureController::createSavedMirror(const QString& appClass, const QString& appTitle, const CaptureSettings& settings) {
  // Match the saved identity against currently open windows: prefer an exact
  // class+title match, fall back to the first window of the same class.
  QString source;  // Empty => disconnected placeholder.
  const QList<WindowSelector::WindowEntry> windows = m_selector->listWindows();
  const WindowSelector::WindowEntry* exact = nullptr;
  const WindowSelector::WindowEntry* classMatch = nullptr;
  for (const WindowSelector::WindowEntry& win : windows) {
    if (!appClass.isEmpty() && win.appClass == appClass) {
      if (win.title == appTitle) {
        exact = &win;
        break;
      }
      if (!classMatch) {
        classMatch = &win;
      }
    }
  }
  const WindowSelector::WindowEntry* match = exact ? exact : classMatch;
  if (match) {
    source = match->captureSource;
  }

  MirroredAppItem* item = new MirroredAppItem(source, appClass, appTitle, settings);
  item->initActions();
  wireMirror(item);
  return item;
}

void CaptureController::wireMirror(MirroredAppItem* item) {
  connect(item, &ResizableAppItem::propertiesRequested, this, &CaptureController::propertiesRequested);
  connect(item, &MirroredAppItem::rebindRequested, this, [this](MirroredAppItem* it) { showBindMenu(it); });
}

void CaptureController::showBindMenu(MirroredAppItem* item) {
  QMenu menu;
  const QList<WindowSelector::WindowEntry> windows = m_selector->listWindows();
  if (windows.isEmpty()) {
    menu.addAction("No open windows found")->setEnabled(false);
  } else {
    for (const WindowSelector::WindowEntry& win : windows) {
      QAction* act = menu.addAction(win.icon, win.title);
      const QString source = win.captureSource;
      const QString appClass = win.appClass;
      const QString title = win.title;
      connect(act, &QAction::triggered, this, [item, source, appClass, title]() {
        item->bindToSource(source);
        item->setIdentity(appClass, title);
      });
    }
  }
  menu.exec(QCursor::pos());
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
