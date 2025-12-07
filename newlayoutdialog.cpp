#include "newlayoutdialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

NewLayoutDialog::NewLayoutDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle("New Layout");
  resize(300, 150);

  QVBoxLayout* layout = new QVBoxLayout(this);

  // Presets
  QHBoxLayout* presetLayout = new QHBoxLayout();
  presetLayout->addWidget(new QLabel("Preset:"));
  m_pPresetCombo = new QComboBox();
  m_pPresetCombo->addItem("1920 x 1080 (FHD)", QSize(1920, 1080));
  m_pPresetCombo->addItem("1920 x 1200 (WUXGA)", QSize(1920, 1200));
  m_pPresetCombo->addItem("2560 x 1440 (QHD)", QSize(2560, 1440));
  m_pPresetCombo->addItem("3840 x 2160 (4K UHD)", QSize(3840, 2160));
  m_pPresetCombo->addItem("Custom...", QSize(0, 0));
  presetLayout->addWidget(m_pPresetCombo);
  layout->addLayout(presetLayout);

  // Custom Dimensions
  QHBoxLayout* dimLayout = new QHBoxLayout();

  m_pWidthSpin = new QSpinBox();
  m_pWidthSpin->setRange(100, 10000);
  m_pWidthSpin->setValue(1920);
  m_pWidthSpin->setSuffix(" px");

  m_pHeightSpin = new QSpinBox();
  m_pHeightSpin->setRange(100, 10000);
  m_pHeightSpin->setValue(1080);
  m_pHeightSpin->setSuffix(" px");

  dimLayout->addWidget(new QLabel("Width:"));
  dimLayout->addWidget(m_pWidthSpin);
  dimLayout->addWidget(new QLabel("Height:"));
  dimLayout->addWidget(m_pHeightSpin);
  layout->addLayout(dimLayout);

  // Buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  // Logic
  connect(m_pPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewLayoutDialog::onPresetChanged);

  // Initial state (disable spins if preset selected)
  onPresetChanged(0);
}

void NewLayoutDialog::onPresetChanged(int index) {
  QSize size = m_pPresetCombo->itemData(index).toSize();
  if (size.isValid() && !size.isEmpty()) {
    m_pWidthSpin->setValue(size.width());
    m_pHeightSpin->setValue(size.height());
    m_pWidthSpin->setEnabled(false);
    m_pHeightSpin->setEnabled(false);
  } else {
    m_pWidthSpin->setEnabled(true);
    m_pHeightSpin->setEnabled(true);
  }
}

int NewLayoutDialog::selectedWidth() const {
  return m_pWidthSpin->value();
}

int NewLayoutDialog::selectedHeight() const {
  return m_pHeightSpin->value();
}
