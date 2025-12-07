#include "propertiesdialog.h"
#include <QFormLayout>
#include <QGraphicsItem>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSpinBox>
#include <QStyleOption>
#include <QVBoxLayout>
#include "constants.h"
#include "guidelineitem.h"
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "zoneitem.h"

PropertiesDialog::PropertiesDialog(QWidget* parent) : QWidget(parent), m_currentItem(nullptr), m_blockSignals(false), m_isDragging(false) {
  setFixedSize(Constants::PropertiesDialogWidth, Constants::PropertiesDialogHeight);

  QPalette pal = palette();
  // FIX: Convert QRgb to QColor for Palette
  pal.setColor(QPalette::WindowText, QColor(Constants::Color::PropText));
  pal.setColor(QPalette::Text, QColor(Constants::Color::PropInputText));
  pal.setColor(QPalette::ButtonText, QColor(Constants::Color::PropText));
  pal.setColor(QPalette::Base, QColor(Constants::Color::PropInputBase));
  setPalette(pal);

  // FIX: Convert QRgb to Hex String (.name()) for Stylesheet args
  QString spinStyle = QString(Constants::Style::SpinBox)
                          .arg(QColor(Constants::Color::PropInputBase).name(), QColor(Constants::Color::PropInputText).name(),
                               QColor(Constants::Color::PropBorder).name(), QColor(Constants::Color::SpinBoxDarkSelection).name(),
                               QColor(Constants::Color::PropBackground).name(), QColor(Constants::Color::SpinBoxDarkHover).name(),
                               QColor(Constants::Color::PropHeaderBg).name(), Constants::Color::IconSpinUpLight, Constants::Color::IconSpinDownLight,
                               QColor(Constants::Color::SpinBoxDarkDisabledText).name(), QColor(Constants::Color::PropHeaderBg).name());

  QString compositeStyle = QString(Constants::Style::PropLabel).arg(QColor(Constants::Color::PropText).name()) + spinStyle;
  setStyleSheet(compositeStyle);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(10, 10, 10, 10);
  mainLayout->setSpacing(10);

  typeLabel = new QLabel("Properties");
  typeLabel->setAlignment(Qt::AlignCenter);
  QFont f = typeLabel->font();
  f.setBold(true);
  f.setPointSize(10);
  typeLabel->setFont(f);

  // FIX: Convert QRgb to Hex String
  typeLabel->setStyleSheet(QString("background-color: %1; border-radius: 2px; padding: 4px; color: %2; border: 1px solid %3;")
                               .arg(QColor(Constants::Color::PropHeaderBg).name())
                               .arg(QColor(Constants::Color::PropText).name())
                               .arg("transparent"));
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

  connect(xSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
  connect(ySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
  connect(wSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
  connect(hSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
}

void PropertiesDialog::setItem(QGraphicsItem* item) {
  m_currentItem = item;

  if (!item) {
    typeLabel->setText("No Selection");

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
  if (!m_currentItem)
    return;

  m_blockSignals = true;

  auto updateField = [&](QSpinBox* spin, bool enabled, int value) {
    spin->setEnabled(enabled);
    if (enabled) {
      spin->setSpecialValueText("");
      if (!spin->hasFocus())
        spin->setValue(value);
    } else {
      spin->setSpecialValueText(" ");
      spin->setValue(spin->minimum());
    }
  };

  if (auto guide = dynamic_cast<GuideLineItem*>(m_currentItem)) {
    if (guide->orientation() == GuideLineItem::Horizontal) {
      updateField(xSpin, false, 0);
      updateField(ySpin, true, guide->pos().y());
      updateField(wSpin, false, 0);
      updateField(hSpin, false, 0);
    } else {
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
  if (m_blockSignals || !m_currentItem)
    return;

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
      if (newPos.x() < 0)
        newPos.setX(0);
      if (newPos.x() + width() > parentRect.width())
        newPos.setX(parentRect.width() - width());
      if (newPos.y() < 0)
        newPos.setY(0);
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

  // FIX: Convert QRgb to QColor for Painter
  p.setBrush(QColor(Constants::Color::PropBackground));
  p.setPen(QPen(QColor(Constants::Color::PropBorder), 1));
  p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);
}
