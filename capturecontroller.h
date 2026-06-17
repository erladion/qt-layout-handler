#ifndef CAPTURECONTROLLER_H
#define CAPTURECONTROLLER_H

#include <QObject>
#include <QString>

class LayoutScene;
class WindowSelector;
class MirroredAppItem;
struct CaptureSettings;
class QEvent;
class QGraphicsItem;
class QGraphicsView;
class QMenu;

// Owns window-capture creation: the WindowSelector, the "Add App" menu (live
// windows + placeholders), the Mirror Stream point-and-click pick, adding the
// resulting items to the active scene, and restoring/re-binding saved captures.
class CaptureController : public QObject {
  Q_OBJECT
public:
  explicit CaptureController(QGraphicsView* view, QObject* parent = nullptr);

  // The scene new capture/app items are added to (nullptr when no layout).
  void setScene(LayoutScene* scene);

  // Fills the "Add App" menu: one entry per open window (mirror it), then a
  // placeholder submenu.
  void populateAddAppMenu(QMenu* menu);

  // Mirror Stream: grab the mouse so the next click picks a window to capture.
  void beginWindowPick();

  // Routes the viewport's selection click while a window pick is in progress;
  // returns true if consumed.
  bool handleViewportEvent(QObject* watched, QEvent* event);

  // Restores a saved mirrored capture: matches the app identity (WM_CLASS +
  // title) against open windows and binds to the best match, otherwise returns a
  // disconnected placeholder. The returned item is wired but NOT yet added to a
  // scene (the caller positions it).
  MirroredAppItem* createSavedMirror(const QString& appClass, const QString& appTitle, const CaptureSettings& settings);

signals:
  // Re-emitted from created items so MainWindow can show the properties dialog.
  void propertiesRequested(QGraphicsItem* item);

private:
  void addMirroredApp(const QString& captureSource, const QString& appClass, const QString& appTitle);
  void addPlaceholderApp(const QString& type);
  void wireMirror(MirroredAppItem* item);
  void showBindMenu(MirroredAppItem* item);

  QGraphicsView* m_pView;
  LayoutScene* m_pScene = nullptr;
  WindowSelector* m_selector = nullptr;
  bool m_isSelectingWindow = false;
};

#endif  // CAPTURECONTROLLER_H
