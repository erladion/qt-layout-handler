#include "propertiesdialog.h"

#include <QFormLayout>
#include <QGraphicsItem>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSpinBox>
#include <QStyleOption>
#include <QVBoxLayout>

#include "guidelineitem.h"  // NEW
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "zoneitem.h"

PropertiesDialog::PropertiesDialog(QWidget* parent)
    : QWidget(parent),
      m_currentItem(nullptr),
      m_blockSignals(false),
      m_isDragging(false) {
  setFixedSize(200, 220);

  QPalette pal = palette();
  pal.setColor(QPalette::WindowText, QColor("#e0e0e0"));
  pal.setColor(QPalette::Text, QColor("#ffffff"));
  pal.setColor(QPalette::ButtonText, QColor("#e0e0e0"));
  pal.setColor(QPalette::Base, QColor("#2b2b2b"));
  setPalette(pal);

  setStyleSheet(
      "QLabel { color: #e0e0e0; }"
      "QSpinBox { background-color: #2b2b2b; color: #ffffff; border: 1px solid "
      "#505050; padding: 2px; selection-background-color: #505050; }"

      "QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: "
      "top right; width: 16px; border-left: 1px solid #505050; border-bottom: "
      "1px solid #505050; background: #353535; }"
      "QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: "
      "bottom right; width: 16px; border-left: 1px solid #505050; border-top: "
      "0px solid #505050; background: #353535; }"

      "QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: "
      "#454545; }"
      "QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { "
      "background: #252525; }"

      "QSpinBox::up-arrow { width: 10px; height: 10px; image: "
      "url(:/resources/spin-up.svg); }"
      "QSpinBox::down-arrow { width: 10px; height: 10px; image: "
      "url(:/resources/spin-down.svg); }"

      "QSpinBox:disabled { color: #808080; background-color: #252525; }"
      "QSpinBox::up-arrow:disabled, QSpinBox::down-arrow:disabled { opacity: "
      "0.3; }");

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(10, 10, 10, 10);
  mainLayout->setSpacing(10);

  typeLabel = new QLabel("Properties");
  typeLabel->setAlignment(Qt::AlignCenter);
  QFont f = typeLabel->font();
  f.setBold(true);
  f.setPointSize(10);
  typeLabel->setFont(f);
  typeLabel->setStyleSheet(
      "background-color: #252525; border-radius: 2px; padding: 4px; color: "
      "#e0e0e0; border: 1px solid #404040;");
  mainLayout->addWidget(typeLabel);

  QFormLayout* form = new QFormLayout();
  form->setLabelAlignment(Qt::AlignRight);

  xSpin = new QSpinBox();
  xSpin->setRange(-10000, 10000);
  xSpin->setKeyboardTracking(false);
  form->addRow("X:", xSpin);

  ySpin = new QSpinBox();
  ySpin->setRange(-10000, 10000);
  ySpin->setKeyboardTracking(false);
  form->addRow("Y:", ySpin);

  wSpin = new QSpinBox();
  wSpin->setRange(10, 10000);
  wSpin->setKeyboardTracking(false);
  form->addRow("Width:", wSpin);

  hSpin = new QSpinBox();
  hSpin->setRange(10, 10000);
  hSpin->setKeyboardTracking(false);
  form->addRow("Height:", hSpin);

  mainLayout->addLayout(form);
  mainLayout->addStretch();

  connect(xSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PropertiesDialog::onValueChanged);
  connect(ySpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PropertiesDialog::onValueChanged);
  connect(wSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PropertiesDialog::onValueChanged);
  connect(hSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PropertiesDialog::onValueChanged);
}

void PropertiesDialog::setItem(QGraphicsItem* item) {
  m_currentItem = item;

  if (!item) {
    typeLabel->setText("No Selection");

    // Hide values if no selection
    m_blockSignals = true;
    auto clearSpin = [](QSpinBox* spin) {
      spin->setEnabled(false);
      spin->setSpecialValueText(" ");
      spin->setValue(spin->minimum());
    };
    clearSpin(xSpin);
    clearSpin(ySpin);
    clearSpin(wSpin);
    clearSpin(hSpin);
    m_blockSignals = false;

    return;
  }

  if (auto app = dynamic_cast<ResizableAppItem*>(item)) {
    typeLabel->setText(app->name());
  } else if (dynamic_cast<ZoneItem*>(item)) {
    typeLabel->setText("Zone");
  } else if (dynamic_cast<SnappingItemGroup*>(item)) {
    typeLabel->setText("Group");
  } else if (auto guide = dynamic_cast<GuideLineItem*>(item)) {
    // Handle Guides
    if (guide->orientation() == GuideLineItem::Horizontal)
      typeLabel->setText("Horizontal Guide");
    else
      typeLabel->setText("Vertical Guide");
  } else {
    typeLabel->setText("Item");
  }

  refreshValues();
}

void PropertiesDialog::refreshValues() {
  if (!m_currentItem) return;

  m_blockSignals = true;

  // Helper to update state and value
  auto updateField = [&](QSpinBox* spin, bool enabled, int value) {
    spin->setEnabled(enabled);
    if (enabled) {
      spin->setSpecialValueText("");
      if (!spin->hasFocus()) spin->setValue(value);
    } else {
      // Show empty if disabled/not applicable
      spin->setSpecialValueText(" ");
      spin->setValue(spin->minimum());
    }
  };

  if (auto guide = dynamic_cast<GuideLineItem*>(m_currentItem)) {
    if (guide->orientation() == GuideLineItem::Horizontal) {
      // Horizontal guide (y-axis position)
      updateField(xSpin, false, 0);
      updateField(ySpin, true, guide->pos().y());
      updateField(wSpin, false, 0);
      updateField(hSpin, false, 0);
    } else {
      // Vertical guide (x-axis position)
      updateField(xSpin, true, guide->pos().x());
      updateField(ySpin, false, 0);
      updateField(wSpin, false, 0);
      updateField(hSpin, false, 0);
    }
  } else if (auto group = dynamic_cast<SnappingItemGroup*>(m_currentItem)) {
    updateField(xSpin, true, group->pos().x());
    updateField(ySpin, true, group->pos().y());
    updateField(wSpin, false, 0);
    updateField(hSpin, false, 0);
  } else {
    updateField(xSpin, true, m_currentItem->pos().x());
    updateField(ySpin, true, m_currentItem->pos().y());

    QRectF r;
    if (auto app = dynamic_cast<ResizableAppItem*>(m_currentItem))
      r = app->rect();
    else if (auto zone = dynamic_cast<ZoneItem*>(m_currentItem))
      r = zone->rect();

    updateField(wSpin, true, r.width());
    updateField(hSpin, true, r.height());
  }

  m_blockSignals = false;
}

void PropertiesDialog::onValueChanged() {
  if (m_blockSignals || !m_currentItem) return;

  // Special handling for GuideLines to enforce axis lock
  if (auto guide = dynamic_cast<GuideLineItem*>(m_currentItem)) {
    if (guide->orientation() == GuideLineItem::Horizontal) {
      guide->setPos(0, ySpin->value());
    } else {
      guide->setPos(xSpin->value(), 0);
    }
    m_currentItem->update();
    emit propertyChanged();
    return;
  }

  // Normal handling for other items
  m_currentItem->setPos(xSpin->value(), ySpin->value());

  if (auto app = dynamic_cast<ResizableAppItem*>(m_currentItem)) {
    QRectF r = app->rect();
    r.setWidth(wSpin->value());
    r.setHeight(hSpin->value());
    app->setRect(r);
    app->updateStatusText();
  } else if (auto zone = dynamic_cast<ZoneItem*>(m_currentItem)) {
    QRectF r = zone->rect();
    r.setWidth(wSpin->value());
    r.setHeight(hSpin->value());
    zone->setRect(r);
  }

  m_currentItem->update();
  emit propertyChanged();
}

void PropertiesDialog::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = true;
    m_dragOffset = event->pos();
  }
  QWidget::mousePressEvent(event);
}

void PropertiesDialog::mouseMoveEvent(QMouseEvent* event) {
  if (m_isDragging && event->buttons() & Qt::LeftButton) {
    QPoint newPos = mapToParent(event->pos()) - m_dragOffset;

    if (parentWidget()) {
      QRect parentRect = parentWidget()->rect();
      if (newPos.x() < 0) newPos.setX(0);
      if (newPos.x() + width() > parentRect.width())
        newPos.setX(parentRect.width() - width());
      if (newPos.y() < 0) newPos.setY(0);
      if (newPos.y() + height() > parentRect.height())
        newPos.setY(parentRect.height() - height());
    }

    move(newPos);
  }
  QWidget::mouseMoveEvent(event);
}

void PropertiesDialog::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = false;
  }
  QWidget::mouseReleaseEvent(event);
}

void PropertiesDialog::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  p.setBrush(QColor("#353535"));
  p.setPen(QPen(QColor("#505050"), 1));
  p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);
}
