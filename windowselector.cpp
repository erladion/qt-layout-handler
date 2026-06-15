#include "windowselector.h"
#include <QCursor>
#include <QDebug>
#include <QImage>
#include <QPixmap>

#include <climits>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <unistd.h>
#endif

WindowSelector::WindowSelector(QObject* parent) : QObject(parent) {}

#if defined(Q_OS_LINUX)
namespace {

// Parses a _NET_WM_ICON property (a sequence of [width, height, w*h ARGB
// pixels] blocks, each element a 32-bit value stored in a long) and returns the
// image whose width is closest to preferredSize.
QImage iconFromNetWmIcon(const unsigned long* data, unsigned long count, int preferredSize) {
  const unsigned long* p = data;
  const unsigned long* end = data + count;
  QImage best;
  int bestScore = INT_MAX;

  while (p + 2 <= end) {
    const int w = static_cast<int>(p[0]);
    const int h = static_cast<int>(p[1]);
    p += 2;
    if (w <= 0 || h <= 0) {
      break;
    }
    if (p + static_cast<unsigned long>(w) * h > end) {
      break;
    }

    QImage img(w, h, QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y) {
      QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
      for (int x = 0; x < w; ++x) {
        line[x] = static_cast<QRgb>(p[static_cast<unsigned long>(y) * w + x] & 0xffffffffUL);
      }
    }
    p += static_cast<unsigned long>(w) * h;

    const int score = qAbs(w - preferredSize);
    if (score < bestScore) {
      bestScore = score;
      best = img;
    }
  }
  return best;
}

}  // namespace
#endif

QList<WindowSelector::WindowEntry> WindowSelector::listWindows() {
  QList<WindowEntry> result;

#if defined(Q_OS_LINUX)
  Display* display = XOpenDisplay(nullptr);
  if (!display) {
    return result;
  }

  Window root = DefaultRootWindow(display);

  const Atom netClientList = XInternAtom(display, "_NET_CLIENT_LIST", True);
  const Atom netWmName = XInternAtom(display, "_NET_WM_NAME", True);
  const Atom utf8 = XInternAtom(display, "UTF8_STRING", True);
  const Atom netWmIcon = XInternAtom(display, "_NET_WM_ICON", True);
  const Atom netWmPid = XInternAtom(display, "_NET_WM_PID", True);

  const long ownPid = static_cast<long>(getpid());

  Atom actualType = None;
  int actualFormat = 0;
  unsigned long nItems = 0;
  unsigned long bytesAfter = 0;
  unsigned char* data = nullptr;

  QList<Window> windows;
  if (netClientList != None &&
      XGetWindowProperty(display, root, netClientList, 0, 1024, False, XA_WINDOW, &actualType, &actualFormat, &nItems, &bytesAfter, &data) == Success &&
      data) {
    Window* wlist = reinterpret_cast<Window*>(data);
    for (unsigned long i = 0; i < nItems; ++i) {
      windows.append(wlist[i]);
    }
    XFree(data);
  }

  for (Window w : windows) {
    // Skip our own windows so the editor doesn't try to capture itself.
    if (netWmPid != None) {
      unsigned char* pidData = nullptr;
      if (XGetWindowProperty(display, w, netWmPid, 0, 1, False, XA_CARDINAL, &actualType, &actualFormat, &nItems, &bytesAfter, &pidData) == Success &&
          pidData) {
        const long pid = static_cast<long>(*reinterpret_cast<unsigned long*>(pidData));
        XFree(pidData);
        if (pid == ownPid) {
          continue;
        }
      }
    }

    // Title: prefer _NET_WM_NAME (UTF-8), fall back to WM_NAME.
    QString title;
    if (netWmName != None && utf8 != None) {
      unsigned char* nameData = nullptr;
      if (XGetWindowProperty(display, w, netWmName, 0, 1024, False, utf8, &actualType, &actualFormat, &nItems, &bytesAfter, &nameData) == Success &&
          nameData) {
        title = QString::fromUtf8(reinterpret_cast<char*>(nameData));
        XFree(nameData);
      }
    }
    if (title.isEmpty()) {
      char* wmName = nullptr;
      if (XFetchName(display, w, &wmName) && wmName) {
        title = QString::fromLocal8Bit(wmName);
        XFree(wmName);
      }
    }
    if (title.isEmpty()) {
      continue;  // Untitled windows are usually utility/dock surfaces.
    }

    WindowEntry entry;
    entry.title = title;
    // use-damage=false grabs the full window each frame instead of relying on
    // XDamage regions, which is steadier for GPU-composited windows (e.g. VS
    // Code / Electron) at the cost of a little more CPU on static windows.
    entry.captureSource = QString("ximagesrc use-damage=false xid=%1").arg(static_cast<qulonglong>(w));

    // Icon from _NET_WM_ICON (request up to 4MB to cover large icons).
    if (netWmIcon != None) {
      unsigned char* iconData = nullptr;
      if (XGetWindowProperty(display, w, netWmIcon, 0, 1024 * 1024, False, XA_CARDINAL, &actualType, &actualFormat, &nItems, &bytesAfter, &iconData) ==
              Success &&
          iconData) {
        const QImage img = iconFromNetWmIcon(reinterpret_cast<unsigned long*>(iconData), nItems, 32);
        if (!img.isNull()) {
          entry.icon = QIcon(QPixmap::fromImage(img));
        }
        XFree(iconData);
      }
    }

    result.append(entry);
  }

  XCloseDisplay(display);
#endif

  return result;
}

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

    // We only grab the title now to print it to your debug console.
    // GetWindowTextW's count is in characters, not bytes — using sizeof here
    // would claim a 512-char buffer and overflow the 256-WCHAR array.
    constexpr int titleMaxLen = 256;
    WCHAR windowTitle[titleMaxLen];
    GetWindowTextW(bestHwnd, windowTitle, titleMaxLen);
    QString titleStr = QString::fromWCharArray(windowTitle);
    titleStr.replace("\"", "");
    qDebug() << "Captured Windows HWND:" << bestHwnd << "Title:" << windowTitle << titleStr;

    // Build the modern hardware-accelerated pipeline!
    // Note: capture-api=wgc (Windows Graphics Capture) is required to target
    // specific application windows rather than the whole monitor.
    // quint64 hwndInt = static_cast<quint64>(reinterpret_cast<quintptr>(bestHwnd));
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

    // Build the Linux-specific GStreamer capture element. use-damage=false grabs
    // the full window each frame instead of XDamage regions: steadier for
    // GPU-composited windows (VS Code / Electron) and avoids X11 event starvation.
    QString captureSource = QString("ximagesrc use-damage=false xid=%1").arg(targetWindow);
    emit windowSelectedForGStreamer(captureSource);
  }

  XCloseDisplay(display);
#endif
}
