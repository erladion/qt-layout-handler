#include "formatpanel.h"

#include <QBrush>
#include <QColorDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPen>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include "officetoolbar.h"

FormatPanel::FormatPanel(OfficeToolbar* ribbon, const QString& spinStyle, QObject* parent) : QObject(parent) {
  m_pSection = ribbon->addSection("Format", QIcon(":/icons/draw.svg"));

  QWidget* formatContainer = new QWidget();
  formatContainer->setFixedWidth(180);
  QVBoxLayout* formatGrid = new QVBoxLayout(formatContainer);
  formatGrid->setContentsMargins(0, 0, 0, 0);
  formatGrid->setSpacing(5);

  m_pLineWidthSpin = new QSpinBox();
  m_pLineWidthSpin->setRange(1, 100);
  m_pLineWidthSpin->setSuffix(" px");
  m_pLineWidthSpin->setStyleSheet(spinStyle);

  QHBoxLayout* sizeLayout = new QHBoxLayout();
  QLabel* sizeLabel = new QLabel("Size:");
  sizeLabel->setStyleSheet("QLabel {color: black;}");
  sizeLayout->addWidget(sizeLabel);
  sizeLayout->addWidget(m_pLineWidthSpin);

  QHBoxLayout* lineColorLayout = new QHBoxLayout();
  QLabel* lineColorLabel = new QLabel("Line color:");
  m_pLineColorBtn = new QPushButton();
  m_pLineColorBtn->setFixedSize(20, 20);
  m_pLineColorBtn->setCursor(Qt::PointingHandCursor);
  lineColorLayout->addWidget(lineColorLabel);
  lineColorLayout->addWidget(m_pLineColorBtn);

  // Fill container (hidden for paths).
  m_pFillContainer = new QWidget();
  QHBoxLayout* fillLayout = new QHBoxLayout(m_pFillContainer);
  fillLayout->setContentsMargins(0, 0, 0, 0);
  fillLayout->setSpacing(5);
  QLabel* fillLabel = new QLabel("Fill:");
  fillLayout->addWidget(fillLabel);
  m_pFillColorBtn = new QPushButton();
  m_pFillColorBtn->setFixedSize(20, 20);
  m_pFillColorBtn->setCursor(Qt::PointingHandCursor);
  fillLayout->addWidget(m_pFillColorBtn);

  formatGrid->addLayout(sizeLayout);
  formatGrid->addLayout(lineColorLayout);
  formatGrid->addLayout(fillLayout);

  m_pSection->addWidget(formatContainer, 0, 0);
  m_pSection->setVisible(false);  // Hidden until a shape is selected.

  connect(m_pLineWidthSpin, qOverload<int>(&QSpinBox::valueChanged), this, &FormatPanel::onLineWidthChanged);
  connect(m_pLineColorBtn, &QPushButton::clicked, this, &FormatPanel::onLineColorClicked);
  connect(m_pFillColorBtn, &QPushButton::clicked, this, &FormatPanel::onFillColorClicked);
}

bool FormatPanel::updateForSelection(QGraphicsItem* item) {
  m_pCurrentShape = nullptr;
  bool showFormat = false;

  // Only drawn shapes (path/rect/ellipse), not custom App/Zone items.
  if (item != nullptr &&
      (item->type() == QGraphicsPathItem::Type || item->type() == QGraphicsRectItem::Type || item->type() == QGraphicsEllipseItem::Type)) {
    showFormat = true;
    m_pCurrentShape = static_cast<QAbstractGraphicsShapeItem*>(item);

    m_pLineWidthSpin->blockSignals(true);
    m_pLineWidthSpin->setValue(m_pCurrentShape->pen().width());
    m_pLineWidthSpin->blockSignals(false);

    updateButtonColor(m_pLineColorBtn, m_pCurrentShape->pen().color());

    // A freehand line has no fill.
    if (item->type() == QGraphicsPathItem::Type) {
      m_pFillContainer->hide();
    } else {
      m_pFillContainer->show();
      updateButtonColor(m_pFillColorBtn, m_pCurrentShape->brush().color());
    }
  }

  m_pSection->setVisible(showFormat);
  return showFormat;
}

void FormatPanel::onLineWidthChanged(int val) {
  if (!m_pCurrentShape) {
    return;
  }
  QPen pen = m_pCurrentShape->pen();
  pen.setWidth(val);
  m_pCurrentShape->setPen(pen);
}

void FormatPanel::onLineColorClicked() {
  if (!m_pCurrentShape) {
    return;
  }
  QColor newColor =
      QColorDialog::getColor(m_pCurrentShape->pen().color(), m_pLineColorBtn, "Line Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
  if (newColor.isValid()) {
    QPen pen = m_pCurrentShape->pen();
    pen.setColor(newColor);
    m_pCurrentShape->setPen(pen);
    updateButtonColor(m_pLineColorBtn, newColor);
  }
}

void FormatPanel::onFillColorClicked() {
  if (!m_pCurrentShape) {
    return;
  }
  QColor newColor =
      QColorDialog::getColor(m_pCurrentShape->brush().color(), m_pFillColorBtn, "Fill Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
  if (newColor.isValid()) {
    m_pCurrentShape->setBrush(QBrush(newColor));
    updateButtonColor(m_pFillColorBtn, newColor);
  }
}

void FormatPanel::updateButtonColor(QPushButton* btn, const QColor& color) {
  const QString colorName = (color.isValid() && color.alpha() > 0) ? color.name() : "transparent";
  btn->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid #777; border-radius: 3px;").arg(colorName));
}
