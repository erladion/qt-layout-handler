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
  m_pFontSizeSpin = new QSpinBox();
  m_pFontSizeSpin->setRange(8, 72);
  m_pFontSizeSpin->setSuffix(" pt");
  m_pFontSizeSpin->setValue(getAppFontSize());
  form->addRow("App Window Base Font Size:", m_pFontSizeSpin);

  // Top Bar
  m_pTopBarSpin = new QSpinBox();
  m_pTopBarSpin->setRange(0, 500);
  m_pTopBarSpin->setSuffix(" px");
  m_pTopBarSpin->setValue(getTopBarHeight());
  form->addRow("Default Top Bar Height:", m_pTopBarSpin);

  // Bottom Bar
  m_pBotBarSpin = new QSpinBox();
  m_pBotBarSpin->setRange(0, 500);
  m_pBotBarSpin->setSuffix(" px");
  m_pBotBarSpin->setValue(getBottomBarHeight());
  form->addRow("Default Bottom Bar Height:", m_pBotBarSpin);

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
  QSettings settings;
  settings.setValue("appFontSize", m_pFontSizeSpin->value());
  settings.setValue("defaultTopBar", m_pTopBarSpin->value());
  settings.setValue("defaultBotBar", m_pBotBarSpin->value());
  accept();
}

int SettingsDialog::getAppFontSize() {
  QSettings settings;
  return settings.value("appFontSize", 20).toInt();
}

int SettingsDialog::getTopBarHeight() {
  QSettings settings;
  return settings.value("defaultTopBar", 30).toInt();
}

int SettingsDialog::getBottomBarHeight() {
  QSettings settings;
  return settings.value("defaultBotBar", 40).toInt();
}
