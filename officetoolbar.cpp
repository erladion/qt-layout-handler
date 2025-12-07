#include "officetoolbar.h"
#include <QApplication>
#include <QDebug>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QStyleOption>
#include <QVBoxLayout>
#include "constants.h"

// =============================================================================
// RibbonButton
// =============================================================================
RibbonButton::RibbonButton(QAction* action, Type type, QWidget* parent) : QToolButton(parent), m_type(type) {
  setDefaultAction(action);
  setAutoRaise(true);
  setCompactMode(false);

  // FIX: Convert QRgb to Hex String
  setStyleSheet(QString(Constants::Style::RibbonButton)
                    .arg(QColor(Constants::Color::RibbonText).name())
                    .arg(QColor(Constants::Color::RibbonHover).name())
                    .arg(QColor(Constants::Color::RibbonPressed).name()));
}

void RibbonButton::setCompactMode(bool compact) {
  if (compact) {
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  } else {
    if (m_type == Large) {
      setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
      setIconSize(QSize(32, 32));
      setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
      setMinimumWidth(50);
    } else {
      setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
      setIconSize(QSize(16, 16));
      setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
      setMinimumWidth(0);
    }
  }
}

// =============================================================================
// RibbonSection
// =============================================================================
RibbonSection::RibbonSection(const QString& title, const QIcon& icon, QWidget* parent)
    : QWidget(parent), m_title(title), m_sectionIcon(icon), m_mode(Normal), m_popupWidget(nullptr), m_gridColCount(0) {
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

  m_stackLayout = new QStackedLayout(this);
  m_stackLayout->setContentsMargins(0, 0, 0, 0);

  // --- Expanded View ---
  m_expandedWidget = new QWidget(this);
  m_expandedLayout = new QVBoxLayout(m_expandedWidget);
  m_expandedLayout->setContentsMargins(2, 2, 2, 2);
  m_expandedLayout->setSpacing(0);

  m_contentWidget = new QWidget(m_expandedWidget);
  m_contentGrid = new QGridLayout(m_contentWidget);
  m_contentGrid->setContentsMargins(0, 0, 0, 0);
  m_contentGrid->setSpacing(2);

  m_expandedLayout->addWidget(m_contentWidget);
  m_expandedLayout->addStretch();

  m_titleLbl = new QLabel(title, m_expandedWidget);
  m_titleLbl->setAlignment(Qt::AlignCenter);
  m_titleLbl->setStyleSheet("color: #888888; font-size: 10px; padding-top: 2px; background: transparent;");
  m_expandedLayout->addWidget(m_titleLbl);

  m_stackLayout->addWidget(m_expandedWidget);

  // --- Collapsed View ---
  m_collapsedWidget = new QWidget(this);
  QVBoxLayout* colLayout = new QVBoxLayout(m_collapsedWidget);
  colLayout->setContentsMargins(0, 0, 0, 0);
  colLayout->setSpacing(0);

  m_collapsedBtn = new QToolButton(m_collapsedWidget);
  m_collapsedBtn->setText(title);
  m_collapsedBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  m_collapsedBtn->setIcon(icon);
  m_collapsedBtn->setIconSize(QSize(32, 32));
  m_collapsedBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // Reuse style constant with hex strings
  m_collapsedBtn->setStyleSheet(QString(Constants::Style::RibbonButton)
                                    .arg(QColor(Constants::Color::RibbonText).name())
                                    .arg(QColor(Constants::Color::RibbonHover).name())
                                    .arg(QColor(Constants::Color::RibbonPressed).name()));

  connect(m_collapsedBtn, &QToolButton::clicked, this, &RibbonSection::onCollapseButtonClicked);

  colLayout->addStretch();
  colLayout->addWidget(m_collapsedBtn, 0, Qt::AlignCenter);
  colLayout->addStretch();

  m_stackLayout->addWidget(m_collapsedWidget);
}

RibbonSection::~RibbonSection() {
  if (m_popupWidget)
    delete m_popupWidget;
}

void RibbonSection::addLargeButton(RibbonButton* btn) {
  m_contentGrid->addWidget(btn, 0, m_gridColCount, 3, 1);
  m_gridColCount++;

  if (m_representativeIcon.isNull()) {
    m_representativeIcon = btn->icon();
  }
}

void RibbonSection::addSmallButton(RibbonButton* btn, int col) {
  Q_UNUSED(col);
}

void RibbonSection::addWidget(QWidget* widget, int row, int col) {
  m_contentGrid->addWidget(widget, row, col);
  if (col >= m_gridColCount)
    m_gridColCount = col + 1;
}

QSize RibbonSection::minimumSizeHint() const {
  return QSize(60, 90);
}

QSize RibbonSection::sizeHint() const {
  if (m_mode == Collapsed)
    return QSize(60, 90);
  return m_expandedWidget->sizeHint();
}

int RibbonSection::estimateWidth(Mode m) const {
  if (m == Collapsed)
    return 60;

  int totalWidth = 0;
  int spacing = m_contentGrid->spacing();

  for (int col = 0; col < m_gridColCount; ++col) {
    int colWidth = 0;
    bool hasCustomWidget = false;
    bool hasButton = false;

    for (int i = 0; i < m_contentGrid->count(); ++i) {
      QLayoutItem* item = m_contentGrid->itemAt(i);
      int r, c, rs, cs;
      m_contentGrid->getItemPosition(i, &r, &c, &rs, &cs);

      if (col >= c && col < c + cs) {
        QWidget* w = item->widget();
        if (!w)
          continue;

        if (qobject_cast<RibbonButton*>(w)) {
          hasButton = true;
        } else {
          hasCustomWidget = true;
          int wHint = w->sizeHint().width();
          if (w->maximumWidth() < 10000)
            wHint = w->maximumWidth();
          colWidth = qMax(colWidth, wHint);
        }
      }
    }

    if (hasCustomWidget) {
      if (colWidth == 0)
        colWidth = 50;
    } else if (hasButton) {
      if (m == Compact)
        colWidth = 36;
      else
        colWidth = 70;
    } else {
      colWidth = 0;
    }

    totalWidth += colWidth;
  }

  if (m_gridColCount > 0)
    totalWidth += (m_gridColCount - 1) * spacing;

  totalWidth += 10;

  return totalWidth;
}

void RibbonSection::setMode(Mode mode) {
  m_mode = mode;

  if (m_mode == Normal) {
    restoreContent();
    m_stackLayout->setCurrentWidget(m_expandedWidget);
    m_titleLbl->show();

    QList<RibbonButton*> btns = m_contentWidget->findChildren<RibbonButton*>();
    for (auto btn : btns)
      btn->setCompactMode(false);
  } else if (m_mode == Compact) {
    restoreContent();
    m_stackLayout->setCurrentWidget(m_expandedWidget);
    m_titleLbl->show();

    QList<RibbonButton*> btns = m_contentWidget->findChildren<RibbonButton*>();
    for (auto btn : btns)
      btn->setCompactMode(true);
  } else if (m_mode == Collapsed) {
    m_stackLayout->setCurrentWidget(m_collapsedWidget);
  }

  updateGeometry();
  update();
}

void RibbonSection::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter p(this);

  if (m_mode != Collapsed) {
    // FIX: Wrap QRgb in QColor
    p.setPen(QColor(Constants::Color::RibbonSeparator));
    p.drawLine(width() - 1, 4, width() - 1, height() - 4);
  }
}

void RibbonSection::onCollapseButtonClicked() {
  if (!m_popupWidget)
    setupPopup();

  QList<RibbonButton*> btns = m_contentWidget->findChildren<RibbonButton*>();
  for (auto btn : btns)
    btn->setCompactMode(false);

  m_contentWidget->setParent(m_popupWidget);
  m_contentWidget->show();
  m_popupWidget->layout()->addWidget(m_contentWidget);

  m_popupWidget->adjustSize();

  QPoint globalPos = m_collapsedBtn->mapToGlobal(QPoint(0, m_collapsedBtn->height()));
  m_popupWidget->move(globalPos);
  m_popupWidget->show();
}

void RibbonSection::setupPopup() {
  m_popupWidget = new QWidget(nullptr, Qt::Popup);
  m_popupWidget->setAttribute(Qt::WA_DeleteOnClose, false);
  QVBoxLayout* layout = new QVBoxLayout(m_popupWidget);
  layout->setContentsMargins(5, 5, 5, 5);
  m_popupWidget->installEventFilter(this);

  QPalette pal = m_popupWidget->palette();
  pal.setColor(QPalette::Window, Qt::white);
  m_popupWidget->setPalette(pal);

  // FIX: Multi-arg arg() with QColor().name()
  m_popupWidget->setStyleSheet(
      QString(Constants::Style::RibbonPopup).arg(QColor(Constants::Color::RibbonBorder).name(), QColor(Constants::Color::RibbonBg).name()));
}

void RibbonSection::restoreContent() {
  if (m_contentWidget->parent() != m_expandedWidget) {
    if (m_popupWidget) {
      m_popupWidget->hide();
      m_popupWidget->layout()->removeWidget(m_contentWidget);
    }
    m_contentWidget->setParent(m_expandedWidget);
    m_expandedLayout->insertWidget(0, m_contentWidget);
  }
}

bool RibbonSection::eventFilter(QObject* watched, QEvent* event) {
  if (watched == m_popupWidget && event->type() == QEvent::Hide) {
    restoreContent();
    setMode(m_mode);
  }
  return QWidget::eventFilter(watched, event);
}

// =============================================================================
// OfficeToolbar
// =============================================================================
OfficeToolbar::OfficeToolbar(QWidget* parent) : QWidget(parent) {
  QPalette pal = palette();
  pal.setColor(QPalette::Window, Qt::white);
  setAutoFillBackground(true);
  setPalette(pal);

  m_layout = new QHBoxLayout(this);
  m_layout->setContentsMargins(0, 0, 0, 0);
  m_layout->setSpacing(0);
  m_layout->setAlignment(Qt::AlignLeft);
  setFixedHeight(Constants::RibbonHeight);
}

QSize OfficeToolbar::minimumSizeHint() const {
  return QSize(200, Constants::RibbonHeight);
}

RibbonSection* OfficeToolbar::addSection(const QString& title, const QIcon& icon) {
  RibbonSection* section = new RibbonSection(title, icon, this);
  m_layout->addWidget(section);
  m_sections.append(section);
  return section;
}

void OfficeToolbar::addSpacer() {
  QWidget* spacer = new QWidget(this);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  m_layout->addWidget(spacer);
}

void OfficeToolbar::resizeEvent(QResizeEvent* event) {
  layoutSections();
  QWidget::resizeEvent(event);
}

void OfficeToolbar::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter p(this);
  p.fillRect(rect(), Qt::white);

  // FIX: Wrap QRgb in QColor
  p.setPen(QColor(Constants::Color::RibbonBorder));
  p.drawLine(0, height() - 1, width(), height() - 1);
}

void OfficeToolbar::layoutSections() {
  int availableWidth = width();
  int wAllNormal = 0;
  int wAllCompact = 0;
  int wAllCollapsed = 0;

  for (auto sec : m_sections) {
    wAllNormal += sec->estimateWidth(RibbonSection::Normal);
    wAllCompact += sec->estimateWidth(RibbonSection::Compact);
    wAllCollapsed += sec->estimateWidth(RibbonSection::Collapsed);
  }
  int sepWidth = m_sections.size() * 5;
  wAllNormal += sepWidth;
  wAllCompact += sepWidth;
  wAllCollapsed += sepWidth;

  if (availableWidth >= wAllNormal) {
    for (auto sec : m_sections)
      sec->setMode(RibbonSection::Normal);
  } else if (availableWidth >= wAllCompact) {
    for (auto sec : m_sections)
      sec->setMode(RibbonSection::Compact);
  } else {
    int currentW = wAllCompact;
    QVector<RibbonSection::Mode> modes(m_sections.size(), RibbonSection::Compact);

    for (int i = m_sections.size() - 1; i >= 0; --i) {
      if (currentW <= availableWidth)
        break;

      int compactW = m_sections[i]->estimateWidth(RibbonSection::Compact);
      int collapsedW = m_sections[i]->estimateWidth(RibbonSection::Collapsed);

      currentW = currentW - compactW + collapsedW;
      modes[i] = RibbonSection::Collapsed;
    }

    for (int i = 0; i < m_sections.size(); ++i)
      m_sections[i]->setMode(modes[i]);
  }
}
