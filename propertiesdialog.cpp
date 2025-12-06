#include "propertiesdialog.h"
#include <QFormLayout>
#include <QGraphicsItem>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSpinBox>
#include <QStyleOption>
#include <QVBoxLayout>
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "zoneitem.h"

PropertiesDialog::PropertiesDialog(QWidget* parent) : QWidget(parent), m_currentItem(nullptr), m_blockSignals(false), m_isDragging(false) {
  setFixedSize(200, 220);

  QPalette pal = palette();
  pal.setColor(QPalette::WindowText, QColor("#e0e0e0"));
  pal.setColor(QPalette::Text, QColor("#ffffff"));
  pal.setColor(QPalette::ButtonText, QColor("#e0e0e0"));
  pal.setColor(QPalette::Base, QColor("#2b2b2b"));
  setPalette(pal);

  // Enhanced stylesheet for visibility
  setStyleSheet(
      "QLabel { color: #e0e0e0; }"
      "QSpinBox { background-color: #2b2b2b; color: #ffffff; border: 1px solid #505050; padding: 2px; selection-background-color: #505050; }"

      // Button styling
      "QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 16px; border-left: 1px solid #505050; border-bottom: "
      "1px solid #505050; background: #353535; }"
      "QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 16px; border-left: 1px solid #505050; "
      "border-top: 0px solid #505050; background: #353535; }"

      // Hover/Pressed states
      "QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: #454545; }"
      "QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background: #252525; }"

      // Arrow Icons
      "QSpinBox::up-arrow { width: 10px; height: 10px; image: url(:/resources/spin-up.svg); }"
      "QSpinBox::down-arrow { width: 10px; height: 10px; image: url(:/resources/spin-down.svg); }"

      // Disabled state
      "QSpinBox:disabled { color: #808080; background-color: #252525; }"
      "QSpinBox::up-arrow:disabled, QSpinBox::down-arrow:disabled { opacity: 0.3; }");

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(10, 10, 10, 10);
  mainLayout->setSpacing(10);

  // Header
  typeLabel = new QLabel("Properties");
  typeLabel->setAlignment(Qt::AlignCenter);
  QFont f = typeLabel->font();
  f.setBold(true);
  f.setPointSize(10);
  typeLabel->setFont(f);
  typeLabel->setStyleSheet("background-color: #252525; border-radius: 2px; padding: 4px; color: #e0e0e0; border: 1px solid #404040;");
  mainLayout->addWidget(typeLabel);

  QFormLayout* form = new QFormLayout();
  form->setLabelAlignment(Qt::AlignRight);

  // Setup SpinBoxes
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
    xSpin->setEnabled(false);
    ySpin->setEnabled(false);
    wSpin->setEnabled(false);
    hSpin->setEnabled(false);
    return;
  }

  if (auto app = dynamic_cast<ResizableAppItem*>(item)) {
    typeLabel->setText(app->name());
  } else if (dynamic_cast<ZoneItem*>(item)) {
    typeLabel->setText("Zone");
  } else if (dynamic_cast<SnappingItemGroup*>(item)) {
    typeLabel->setText("Group");
  } else {
    typeLabel->setText("Item");
  }

  refreshValues();
}

void PropertiesDialog::refreshValues() {
  if (!m_currentItem)
    return;

  m_blockSignals = true;

  if (auto group = dynamic_cast<SnappingItemGroup*>(m_currentItem)) {
    // Only update if not focused
    if (!xSpin->hasFocus())
      xSpin->setValue(group->pos().x());
    if (!ySpin->hasFocus())
      ySpin->setValue(group->pos().y());

    wSpin->setValue(group->boundingRect().width());
    hSpin->setValue(group->boundingRect().height());

    xSpin->setEnabled(true);
    ySpin->setEnabled(true);
    wSpin->setEnabled(false);
    hSpin->setEnabled(false);
  } else {
    // Only update if not focused
    if (!xSpin->hasFocus())
      xSpin->setValue(m_currentItem->pos().x());
    if (!ySpin->hasFocus())
      ySpin->setValue(m_currentItem->pos().y());

    QRectF r;
    if (auto app = dynamic_cast<ResizableAppItem*>(m_currentItem))
      r = app->rect();
    else if (auto zone = dynamic_cast<ZoneItem*>(m_currentItem))
      r = zone->rect();

    if (!wSpin->hasFocus())
      wSpin->setValue(r.width());
    if (!hSpin->hasFocus())
      hSpin->setValue(r.height());

    xSpin->setEnabled(true);
    ySpin->setEnabled(true);
    wSpin->setEnabled(true);
    hSpin->setEnabled(true);
  }

  m_blockSignals = false;
}

void PropertiesDialog::onValueChanged() {
  if (m_blockSignals || !m_currentItem)
    return;

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

// --- Dragging Logic ---

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

  // Manual painting of background and border
  p.setBrush(QColor("#353535"));
  p.setPen(QPen(QColor("#505050"), 1));
  p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);
}
