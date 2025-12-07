#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSpinBox;

class SettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit SettingsDialog(QWidget* parent = nullptr);

  static int getAppFontSize();
  static int getTopBarHeight();
  static int getBottomBarHeight();

private slots:
  void saveSettings();

private:
  QSpinBox* m_pFontSizeSpin;
  QSpinBox* m_pTopBarSpin;
  QSpinBox* m_pBotBarSpin;
};

#endif  // SETTINGSDIALOG_H
