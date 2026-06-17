#include "outputcontroller.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QScreen>
#include <QWidget>

#include "layoutscene.h"
#include "outputrecorder.h"
#include "projectorwindow.h"

OutputController::OutputController(QWidget* dialogParent, QObject* parent) : QObject(parent), m_pDialogParent(dialogParent) {
  m_pRecorder = new OutputRecorder(this);

  m_pStopAction = new QAction(QIcon(":/icons/stop_output.svg"), "Stop Output", this);
  connect(m_pStopAction, &QAction::triggered, this, [this]() {
    if (m_pProjector) {
      stopOutput();
      emit statusMessage("Projector output stopped.");
    }
  });

  m_pRecordAction = new QAction(QIcon(":/icons/record.svg"), "Record Output", this);
  connect(m_pRecordAction, &QAction::triggered, this, [this]() { toggleRecording(); });

  m_pRecordAudioAction = new QAction(QIcon(":/icons/mic.svg"), "Record Audio", this);
  m_pRecordAudioAction->setToolTip("Capture system/app audio into recordings (falls back to mic)");
  m_pRecordAudioAction->setCheckable(true);
  m_pRecordAudioAction->setChecked(m_pRecorder->audioEnabled());
  connect(m_pRecordAudioAction, &QAction::toggled, this, [this](bool on) { m_pRecorder->setAudioEnabled(on); });
}

void OutputController::setScene(LayoutScene* scene) {
  // Recording is bound to a specific scene, so a swap/close invalidates it.
  stopRecording();
  m_pScene = scene;

  if (!m_pScene) {
    stopOutput();  // No layout left to show.
    return;
  }
  if (m_pProjector) {
    m_pProjector->setScene(m_pScene);
  }
}

void OutputController::populateScreenMenu(QMenu* menu) {
  menu->clear();

  // Build the monitor list fresh each time the menu opens.
  const QList<QScreen*> screens = QApplication::screens();
  for (int i = 0; i < screens.size(); ++i) {
    QScreen* screen = screens[i];

    // Format looks like: "Screen 2: DELL U2720Q (3840x2160)"
    const QString screenName =
        QStringLiteral("Screen %1: %2 (%3x%4)")
            .arg(QString::number(i + 1), screen->name(), QString::number(screen->geometry().width()), QString::number(screen->geometry().height()));

    QAction* screenAct = menu->addAction(screenName);
    connect(screenAct, &QAction::triggered, this, [this, screen]() { sendToScreen(screen); });
  }

  menu->addSeparator();

  // Keep a windowed fallback for easy local testing.
  QAction* windowedAct = menu->addAction("Windowed Mode (Local Test)");
  connect(windowedAct, &QAction::triggered, this, [this]() { showWindowedOutput(); });
}

void OutputController::sendToScreen(QScreen* screen) {
  if (!m_pScene) {
    return;
  }
  if (!m_pProjector) {
    m_pProjector = new ProjectorWindow(m_pScene);
    m_pProjector->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  }

  // Pull it out of fullscreen before moving it to another monitor.
  m_pProjector->showNormal();

  const QRect screenGeo = screen->geometry();
  m_pProjector->move(screenGeo.topLeft());
  m_pProjector->resize(screenGeo.size());
  m_pProjector->showFullScreen();
}

void OutputController::showWindowedOutput() {
  if (!m_pScene) {
    return;
  }
  if (!m_pProjector) {
    m_pProjector = new ProjectorWindow(m_pScene);
  }
  m_pProjector->showNormal();
  m_pProjector->resize(1280, 720);
  m_pProjector->show();
}

void OutputController::stopOutput() {
  if (m_pProjector) {
    m_pProjector->close();        // WA_DeleteOnClose tears it down.
    m_pProjector->deleteLater();  // Belt-and-braces; reset so it can relaunch.
    m_pProjector = nullptr;
  }
}

void OutputController::stopRecording() {
  if (m_pRecorder && m_pRecorder->isRecording()) {
    m_pRecorder->stop();
    m_pRecordAction->setIcon(QIcon(":/icons/record.svg"));
    m_pRecordAction->setText("Record Output");
    emit statusMessage("Recording stopped.");
  }
}

void OutputController::toggleRecording() {
  if (!m_pRecorder) {
    return;
  }

  if (m_pRecorder->isRecording()) {
    stopRecording();
    return;
  }

  if (!m_pScene) {
    emit statusMessage("Create or open a layout before recording.");
    return;
  }

  const QString fileName = QFileDialog::getSaveFileName(m_pDialogParent, "Record Output To", "", "Video (*.mkv)", nullptr, QFileDialog::DontUseNativeDialog);
  if (fileName.isEmpty()) {
    return;
  }

  if (m_pRecorder->start(m_pScene, fileName)) {
    m_pRecordAction->setIcon(QIcon(":/icons/stop-record.svg"));
    m_pRecordAction->setText("Stop Recording");
    emit statusMessage("Recording output...");
  } else {
    emit statusMessage("Failed to start recording.");
  }
}
