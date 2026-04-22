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
  QPoint mousePos = QCursor::pos();
  POINT pt = {mousePos.x(), mousePos.y()};
  HWND targetHwnd = WindowFromPoint(pt);

  if (targetHwnd) {
    // Climb the tree to find the top-most parent that is VISIBLE
    HWND currentHwnd = targetHwnd;
    HWND bestHwnd = NULL;

    while (currentHwnd != NULL) {
      int length = GetWindowTextLengthA(currentHwnd);
      if (IsWindowVisible(currentHwnd) && length > 0) {
        bestHwnd = currentHwnd;
      }
      currentHwnd = GetParent(currentHwnd);
    }

    if (!bestHwnd) {
      bestHwnd = GetAncestor(targetHwnd, GA_ROOT);
    }

    // We only grab the title now to print it to your debug console
    WCHAR windowTitle[256];
    GetWindowTextW(bestHwnd, windowTitle, sizeof(windowTitle));
    QString titleStr = QString::fromWCharArray(windowTitle);
    titleStr.replace("\"", "");
    qDebug() << "Captured Windows HWND:" << bestHwnd << "Title:" << windowTitle << titleStr;

    // THE UPGRADE:
    // We cast the HWND pointer into a 64-bit integer to feed it to GStreamer
    quint64 hwndInt = static_cast<quint64>(reinterpret_cast<quintptr>(bestHwnd));

    // Build the modern hardware-accelerated pipeline!
    // Note: capture-api=wgc (Windows Graphics Capture) is required to target
    // specific application windows rather than the whole monitor.
    // QString captureSource = QString("d3d11screencapturesrc capture-api=wgc window-handle=%1").arg(hwndInt);
    // emit windowSelectedForGStreamer(captureSource);

    QString captureSource = QStringLiteral("gdiscreencapsrc");
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
    QString captureSource = QString("ximagesrc xid=%1").arg(targetWindow);
    emit windowSelectedForGStreamer(captureSource);
  }

  XCloseDisplay(display);
#endif
}
