#ifndef WINDOWSELECTOR_H
#define WINDOWSELECTOR_H

#include <QObject>
#include <QString>

class WindowSelector : public QObject {
  Q_OBJECT
public:
  explicit WindowSelector(QObject* parent = nullptr);
  void captureWindowUnderCursor();

signals:
  void windowSelectedForGStreamer(const QString& pipelineString);
};

#endif // WINDOWSELECTOR_H
