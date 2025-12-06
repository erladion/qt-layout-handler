#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSpinBox;

class SettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit SettingsDialog(QWidget* parent = nullptr);

  // Static helpers to access settings globally
  static int getAppFontSize();
  static int getTopBarHeight();
  static int getBottomBarHeight();

private slots:
  void saveSettings();

private:
  QSpinBox* m_fontSizeSpin;
  QSpinBox* m_topBarSpin;
  QSpinBox* m_botBarSpin;
};

#endif  // SETTINGSDIALOG_H
