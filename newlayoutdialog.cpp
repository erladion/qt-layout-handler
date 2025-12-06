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
  m_presetCombo = new QComboBox();
  m_presetCombo->addItem("1920 x 1080 (FHD)", QSize(1920, 1080));
  m_presetCombo->addItem("1920 x 1200 (WUXGA)", QSize(1920, 1200));
  m_presetCombo->addItem("2560 x 1440 (QHD)", QSize(2560, 1440));
  m_presetCombo->addItem("3840 x 2160 (4K UHD)", QSize(3840, 2160));
  m_presetCombo->addItem("Custom...", QSize(0, 0));
  presetLayout->addWidget(m_presetCombo);
  layout->addLayout(presetLayout);

  // Custom Dimensions
  QHBoxLayout* dimLayout = new QHBoxLayout();

  m_widthSpin = new QSpinBox();
  m_widthSpin->setRange(100, 10000);
  m_widthSpin->setValue(1920);
  m_widthSpin->setSuffix(" px");

  m_heightSpin = new QSpinBox();
  m_heightSpin->setRange(100, 10000);
  m_heightSpin->setValue(1080);
  m_heightSpin->setSuffix(" px");

  dimLayout->addWidget(new QLabel("Width:"));
  dimLayout->addWidget(m_widthSpin);
  dimLayout->addWidget(new QLabel("Height:"));
  dimLayout->addWidget(m_heightSpin);
  layout->addLayout(dimLayout);

  // Buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  // Logic
  connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewLayoutDialog::onPresetChanged);

  // Initial state (disable spins if preset selected)
  onPresetChanged(0);
}

void NewLayoutDialog::onPresetChanged(int index) {
  QSize size = m_presetCombo->itemData(index).toSize();
  if (size.isValid() && !size.isEmpty()) {
    m_widthSpin->setValue(size.width());
    m_heightSpin->setValue(size.height());
    m_widthSpin->setEnabled(false);
    m_heightSpin->setEnabled(false);
  } else {
    m_widthSpin->setEnabled(true);
    m_heightSpin->setEnabled(true);
  }
}

int NewLayoutDialog::selectedWidth() const {
  return m_widthSpin->value();
}

int NewLayoutDialog::selectedHeight() const {
  return m_heightSpin->value();
}
