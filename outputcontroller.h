#ifndef OUTPUTCONTROLLER_H
#define OUTPUTCONTROLLER_H

#include <QObject>
#include <QPointer>

class LayoutScene;
class OutputRecorder;
class ProjectorWindow;
class QAction;
class QMenu;
class QScreen;
class QWidget;

// Owns the audience-facing output: the projector window, the output recorder,
// and their ribbon actions. MainWindow places the actions/menu in the ribbon and
// keeps the controller pointed at the active scene via setScene().
class OutputController : public QObject {
  Q_OBJECT
public:
  // dialogParent parents the file-save dialog (e.g. the main window).
  explicit OutputController(QWidget* dialogParent, QObject* parent = nullptr);

  // The scene to project / record. Pass nullptr when the layout closes (stops
  // recording and closes the projector); otherwise an open projector follows the
  // new scene.
  void setScene(LayoutScene* scene);

  // Ribbon integration: the owned toggle/stop actions, and on-demand population
  // of the "Send to Output" screen menu.
  QAction* recordAction() const { return m_pRecordAction; }
  QAction* stopOutputAction() const { return m_pStopAction; }
  void populateScreenMenu(QMenu* menu);

  void stopOutput();     // Close the projector window.
  void stopRecording();  // Stop and finalize an active recording.

signals:
  void statusMessage(const QString& message);

private:
  void sendToScreen(QScreen* screen);
  void showWindowedOutput();
  void toggleRecording();

  QWidget* m_pDialogParent;
  LayoutScene* m_pScene = nullptr;
  QPointer<ProjectorWindow> m_pProjector;
  OutputRecorder* m_pRecorder = nullptr;

  QAction* m_pRecordAction = nullptr;
  QAction* m_pStopAction = nullptr;
};

#endif  // OUTPUTCONTROLLER_H
