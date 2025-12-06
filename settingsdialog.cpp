#include "settingsdialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle("Settings");
  resize(350, 200);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  QFormLayout* form = new QFormLayout();

  // Font Size
  m_fontSizeSpin = new QSpinBox();
  m_fontSizeSpin->setRange(8, 72);
  m_fontSizeSpin->setSuffix(" pt");
  m_fontSizeSpin->setValue(getAppFontSize());
  form->addRow("App Window Base Font Size:", m_fontSizeSpin);

  // Top Bar
  m_topBarSpin = new QSpinBox();
  m_topBarSpin->setRange(0, 500);
  m_topBarSpin->setSuffix(" px");
  m_topBarSpin->setValue(getTopBarHeight());
  form->addRow("Default Top Bar Height:", m_topBarSpin);

  // Bottom Bar
  m_botBarSpin = new QSpinBox();
  m_botBarSpin->setRange(0, 500);
  m_botBarSpin->setSuffix(" px");
  m_botBarSpin->setValue(getBottomBarHeight());
  form->addRow("Default Bottom Bar Height:", m_botBarSpin);

  mainLayout->addLayout(form);

  // Note Label
  QLabel* note = new QLabel("Note: Bar height changes apply to new layouts.", this);
  note->setStyleSheet("color: gray; font-size: 10px;");
  mainLayout->addWidget(note);

  mainLayout->addStretch();

  // Buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(buttons);
}

void SettingsDialog::saveSettings() {
  QSettings settings("MyCompany", "LayoutManager");
  settings.setValue("appFontSize", m_fontSizeSpin->value());
  settings.setValue("defaultTopBar", m_topBarSpin->value());
  settings.setValue("defaultBotBar", m_botBarSpin->value());
  accept();
}

int SettingsDialog::getAppFontSize() {
  QSettings settings("MyCompany", "LayoutManager");
  return settings.value("appFontSize", 20).toInt();
}

int SettingsDialog::getTopBarHeight() {
  QSettings settings("MyCompany", "LayoutManager");
  return settings.value("defaultTopBar", 30).toInt();
}

int SettingsDialog::getBottomBarHeight() {
  QSettings settings("MyCompany", "LayoutManager");
  return settings.value("defaultBotBar", 40).toInt();
}
