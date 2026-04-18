#include "windowselector.h"
#include <QCursor>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#endif

WindowSelector::WindowSelector(QObject* parent) : QObject(parent) {}

void WindowSelector::captureWindowUnderCursor() {
#ifdef Q_OS_WIN
  // ==========================================
  // WINDOWS IMPLEMENTATION
  // ==========================================
  QPoint mousePos = QCursor::pos();
  POINT pt = {mousePos.x(), mousePos.y()};
  HWND targetHwnd = WindowFromPoint(pt);

  if (targetHwnd) {
    // Get the main parent window, not just a sub-widget
    HWND rootHwnd = GetAncestor(targetHwnd, GA_ROOT);

    char windowTitle[256];
    GetWindowTextA(rootHwnd, windowTitle, sizeof(windowTitle));

    qDebug() << "Captured Windows HWND:" << rootHwnd << "Title:" << windowTitle;

    // Build the Windows-specific GStreamer capture element
    QString captureSource = QString("gdiscreencapsrc window-name=\"%1\"").arg(windowTitle);
    emit windowSelectedForGStreamer(captureSource);
  }

#elif defined(Q_OS_LINUX)
  // ==========================================
  // LINUX X11 IMPLEMENTATION
  // ==========================================
  Display* display = XOpenDisplay(nullptr);
  if (!display)
    return;

  Window root = DefaultRootWindow(display);
  Window rootReturn, childReturn;
  int rootX, rootY, winX, winY;
  unsigned int mask;

  if (XQueryPointer(display, root, &rootReturn, &childReturn, &rootX, &rootY, &winX, &winY, &mask)) {
    Window targetWindow = (childReturn != None) ? childReturn : rootReturn;

    Window parentReturn;
    Window* childrenReturn;
    unsigned int numChildren;

    while (targetWindow != root && targetWindow != 0) {
      if (XQueryTree(display, targetWindow, &rootReturn, &parentReturn, &childrenReturn, &numChildren)) {
        if (childrenReturn)
          XFree(childrenReturn);
        if (parentReturn == root)
          break;
        targetWindow = parentReturn;
      } else {
        break;
      }
    }

    qDebug() << "Captured Top-Level Linux XID:" << targetWindow;

    // Build the Linux-specific GStreamer capture element
    // (use-damage=false prevents X11 event loop starvation)
    QString captureSource = QString("ximagesrc xid=%1 use-damage=false").arg(targetWindow);
    emit windowSelectedForGStreamer(captureSource);
  }

  XCloseDisplay(display);
#endif
}
