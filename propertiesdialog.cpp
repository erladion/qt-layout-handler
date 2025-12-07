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

PropertiesDialog::PropertiesDialog(QWidget* parent) : QWidget(parent), m_pCurrentItem(nullptr), m_blockSignals(false), m_isDragging(false) {
  setFixedSize(Constants::PropertiesDialogWidth, Constants::PropertiesDialogHeight);

  QPalette pal = palette();

  pal.setColor(QPalette::WindowText, QColor::fromRgba(Constants::Color::PropText));
  pal.setColor(QPalette::Text, QColor::fromRgba(Constants::Color::PropInputText));
  pal.setColor(QPalette::ButtonText, QColor::fromRgba(Constants::Color::PropText));
  pal.setColor(QPalette::Base, QColor::fromRgba(Constants::Color::PropInputBase));
  setPalette(pal);

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

  m_pTypeLabel = new QLabel("Properties");
  m_pTypeLabel->setAlignment(Qt::AlignCenter);
  QFont f = m_pTypeLabel->font();
  f.setBold(true);
  f.setPointSize(10);
  m_pTypeLabel->setFont(f);

  m_pTypeLabel->setStyleSheet(QString("background-color: %1; border-radius: 2px; padding: 4px; color: %2; border: 1px solid %3;")
                                  .arg(QColor(Constants::Color::PropHeaderBg).name(), QColor(Constants::Color::PropText).name(), "transparent"));
  mainLayout->addWidget(m_pTypeLabel);

  QFormLayout* form = new QFormLayout();
  form->setLabelAlignment(Qt::AlignRight);

  m_pXSpin = new QSpinBox();
  m_pXSpin->setRange(-10000, 10000);
  m_pXSpin->setKeyboardTracking(false);
  form->addRow("X:", m_pXSpin);

  m_pYSpin = new QSpinBox();
  m_pYSpin->setRange(-10000, 10000);
  m_pYSpin->setKeyboardTracking(false);
  form->addRow("Y:", m_pYSpin);

  m_pWSpin = new QSpinBox();
  m_pWSpin->setRange(10, 10000);
  m_pWSpin->setKeyboardTracking(false);
  form->addRow("Width:", m_pWSpin);

  m_pHSpin = new QSpinBox();
  m_pHSpin->setRange(10, 10000);
  m_pHSpin->setKeyboardTracking(false);
  form->addRow("Height:", m_pHSpin);

  mainLayout->addLayout(form);
  mainLayout->addStretch();

  connect(m_pXSpin, qOverload<int>(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
  connect(m_pYSpin, qOverload<int>(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
  connect(m_pWSpin, qOverload<int>(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
  connect(m_pHSpin, qOverload<int>(&QSpinBox::valueChanged), this, &PropertiesDialog::onValueChanged);
}

void PropertiesDialog::setItem(QGraphicsItem* item) {
  m_pCurrentItem = item;

  if (!item) {
    m_pTypeLabel->setText("No Selection");

    m_blockSignals = true;
    auto clearSpin = [](QSpinBox* spin) {
      spin->setEnabled(false);
      spin->setSpecialValueText(" ");
      spin->setValue(spin->minimum());
    };
    clearSpin(m_pXSpin);
    clearSpin(m_pYSpin);
    clearSpin(m_pWSpin);
    clearSpin(m_pHSpin);
    m_blockSignals = false;

    return;
  }

  switch (item->type()) {
    case Constants::Item::AppItem: {
      auto app = static_cast<ResizableAppItem*>(item);
      m_pTypeLabel->setText(app->name());
      break;
    }
    case Constants::Item::ZoneItem: {
      m_pTypeLabel->setText("Zone");
      break;
    }
    case Constants::Item::GroupItem: {
      m_pTypeLabel->setText("Group");
      break;
    }
    case Constants::Item::GuideItem: {
      auto guide = static_cast<GuideLineItem*>(item);
      if (guide->orientation() == GuideLineItem::Horizontal) {
        m_pTypeLabel->setText("Horizontal Guide");
      } else {
        m_pTypeLabel->setText("Vertical Guide");
      }
      break;
    }
    default:
      m_pTypeLabel->setText("Item");
      break;
  }

  refreshValues();
}

void PropertiesDialog::refreshValues() {
  if (!m_pCurrentItem) {
    return;
  }

  m_blockSignals = true;

  auto updateField = [&](QSpinBox* spin, bool enabled, int value) {
    spin->setEnabled(enabled);
    if (enabled) {
      spin->setSpecialValueText("");
      if (!spin->hasFocus()) {
        spin->setValue(value);
      }
    } else {
      spin->setSpecialValueText(" ");
      spin->setValue(spin->minimum());
    }
  };

  if (m_pCurrentItem->type() == Constants::Item::GuideItem) {
    auto guide = static_cast<GuideLineItem*>(m_pCurrentItem);
    if (guide->orientation() == GuideLineItem::Horizontal) {
      updateField(m_pXSpin, false, 0);
      updateField(m_pYSpin, true, guide->pos().y());
      updateField(m_pWSpin, false, 0);
      updateField(m_pHSpin, false, 0);
    } else {
      updateField(m_pXSpin, true, guide->pos().x());
      updateField(m_pYSpin, false, 0);
      updateField(m_pWSpin, false, 0);
      updateField(m_pHSpin, false, 0);
    }
  } else if (m_pCurrentItem->type() == Constants::Item::GroupItem) {
    auto group = static_cast<SnappingItemGroup*>(m_pCurrentItem);
    updateField(m_pXSpin, true, group->pos().x());
    updateField(m_pYSpin, true, group->pos().y());
    updateField(m_pWSpin, false, 0);
    updateField(m_pHSpin, false, 0);
  } else {
    updateField(m_pXSpin, true, m_pCurrentItem->pos().x());
    updateField(m_pYSpin, true, m_pCurrentItem->pos().y());

    QRectF r;
    if (m_pCurrentItem->type() == Constants::Item::AppItem) {
      auto app = static_cast<ResizableAppItem*>(m_pCurrentItem);
      r = app->rect();
    } else if (m_pCurrentItem->type() == Constants::Item::ZoneItem) {
      auto zone = static_cast<ZoneItem*>(m_pCurrentItem);
      r = zone->rect();
    }

    updateField(m_pWSpin, true, r.width());
    updateField(m_pHSpin, true, r.height());
  }

  m_blockSignals = false;
}

void PropertiesDialog::onValueChanged() {
  if (m_blockSignals || !m_pCurrentItem) {
    return;
  }

  if (m_pCurrentItem->type() == Constants::Item::GuideItem) {
    auto guide = static_cast<GuideLineItem*>(m_pCurrentItem);
    if (guide->orientation() == GuideLineItem::Horizontal) {
      guide->setPos(0, m_pYSpin->value());
    } else {
      guide->setPos(m_pXSpin->value(), 0);
    }
    m_pCurrentItem->update();
    emit propertyChanged();
    return;
  }

  m_pCurrentItem->setPos(m_pXSpin->value(), m_pYSpin->value());

  if (m_pCurrentItem->type() == Constants::Item::AppItem) {
    auto app = static_cast<ResizableAppItem*>(m_pCurrentItem);
    QRectF r = app->rect();
    r.setWidth(m_pWSpin->value());
    r.setHeight(m_pHSpin->value());
    app->setRect(r);
    app->updateStatusText();
  } else if (m_pCurrentItem->type() == Constants::Item::ZoneItem) {
    auto zone = static_cast<ZoneItem*>(m_pCurrentItem);
    QRectF r = zone->rect();
    r.setWidth(m_pWSpin->value());
    r.setHeight(m_pHSpin->value());
    zone->setRect(r);
  }

  m_pCurrentItem->update();
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
      if (newPos.x() < 0) {
        newPos.setX(0);
      }
      if (newPos.x() + width() > parentRect.width()) {
        newPos.setX(parentRect.width() - width());
      }
      if (newPos.y() < 0) {
        newPos.setY(0);
      }
      if (newPos.y() + height() > parentRect.height()) {
        newPos.setY(parentRect.height() - height());
      }
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

  p.setBrush(QColor::fromRgba(Constants::Color::PropBackground));
  p.setPen(QPen(QColor::fromRgba(Constants::Color::PropBorder), 1));
  p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);
}
