#ifndef WINDOWSELECTOR_H
#define WINDOWSELECTOR_H

#include <QIcon>
#include <QList>
#include <QObject>
#include <QString>

class WindowSelector : public QObject {
  Q_OBJECT
public:
  // A capturable top-level window discovered on the system.
  struct WindowEntry {
    QString title;
    QString captureSource;  // GStreamer source string, e.g. "ximagesrc xid=123".
    QIcon icon;             // The window's own icon (may be null if unavailable).
  };

  explicit WindowSelector(QObject* parent = nullptr);
  void captureWindowUnderCursor();

  // Enumerates the currently open top-level application windows so the user can
  // pick one from a list. Returns an empty list on platforms / window managers
  // that don't expose the window list.
  QList<WindowEntry> listWindows();

signals:
  void windowSelectedForGStreamer(const QString& pipelineString);
};

#endif  // WINDOWSELECTOR_H
