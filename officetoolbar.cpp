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
#include <QTimer>
#include <QVBoxLayout>

#include "constants.h"

// =============================================================================
// RibbonButton
// =============================================================================
RibbonButton::RibbonButton(QAction* action, Type type, QWidget* parent) : QToolButton(parent), m_type(type) {
  setDefaultAction(action);
  setAutoRaise(true);
  setCompactMode(false);

  setStyleSheet(QString(Constants::Style::RibbonButton)
                    .arg(QColor(Constants::Color::RibbonText).name(), QColor(Constants::Color::RibbonHover).name(),
                         QColor(Constants::Color::RibbonPressed).name()));
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
    : QWidget(parent), m_title(title), m_sectionIcon(icon), m_mode(Normal), m_pPopupWidget(nullptr), m_gridColCount(0) {
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

  m_pStackLayout = new QStackedLayout(this);
  m_pStackLayout->setContentsMargins(0, 0, 0, 0);

  // --- Expanded View ---
  m_pExpandedWidget = new QWidget(this);
  m_pExpandedLayout = new QVBoxLayout(m_pExpandedWidget);
  m_pExpandedLayout->setContentsMargins(2, 2, 2, 2);
  m_pExpandedLayout->setSpacing(0);

  m_pContentWidget = new QWidget(m_pExpandedWidget);
  m_pContentWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
  m_pContentGrid = new QGridLayout(m_pContentWidget);
  m_pContentGrid->setContentsMargins(0, 0, 0, 0);
  m_pContentGrid->setSpacing(2);

  m_pExpandedLayout->addWidget(m_pContentWidget);
  // m_pExpandedLayout->addStretch();

  m_pTitleLabel = new QLabel(title, m_pExpandedWidget);
  m_pTitleLabel->setAlignment(Qt::AlignCenter);
  m_pTitleLabel->setStyleSheet("color: #888888; font-size: 10px; padding-top: 2px; background: transparent;");
  m_pExpandedLayout->addWidget(m_pTitleLabel);

  m_pStackLayout->addWidget(m_pExpandedWidget);

  // --- Collapsed View ---
  m_pCollapsedWidget = new QWidget(this);
  QVBoxLayout* colLayout = new QVBoxLayout(m_pCollapsedWidget);
  colLayout->setContentsMargins(0, 0, 0, 0);
  colLayout->setSpacing(0);

  m_pCollapsedButton = new QToolButton(m_pCollapsedWidget);
  m_pCollapsedButton->setText(title);
  m_pCollapsedButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  m_pCollapsedButton->setIcon(icon);
  m_pCollapsedButton->setIconSize(QSize(32, 32));
  m_pCollapsedButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // Reuse style constant with hex strings
  m_pCollapsedButton->setStyleSheet(QString(Constants::Style::RibbonButton)
                                        .arg(QColor(Constants::Color::RibbonText).name(), QColor(Constants::Color::RibbonHover).name(),
                                             QColor(Constants::Color::RibbonPressed).name()));

  connect(m_pCollapsedButton, &QToolButton::clicked, this, &RibbonSection::onCollapseButtonClicked);

  colLayout->addStretch();
  colLayout->addWidget(m_pCollapsedButton, 0, Qt::AlignCenter);
  colLayout->addStretch();

  m_pStackLayout->addWidget(m_pCollapsedWidget);
}

RibbonSection::~RibbonSection() {
  if (m_pPopupWidget) {
    delete m_pPopupWidget;
  }
}

void RibbonSection::addLargeButton(RibbonButton* btn) {
  m_pContentGrid->addWidget(btn, 0, m_gridColCount, 3, 1);
  m_gridColCount++;

  if (m_representativeIcon.isNull()) {
    m_representativeIcon = btn->icon();
  }
}

void RibbonSection::addWidget(QWidget* widget, int row, int col) {
  m_pContentGrid->addWidget(widget, row, col);
  if (col >= m_gridColCount) {
    m_gridColCount = col + 1;
  }
}

QSize RibbonSection::minimumSizeHint() const {
  return QSize(60, 90);
}

QSize RibbonSection::sizeHint() const {
  if (m_mode == Collapsed) {
    return QSize(60, 90);
  }
  return m_pExpandedWidget->sizeHint();
}

int RibbonSection::estimateWidth(Mode m) const {
  if (m == Collapsed) {
    return 60;
  }

  int totalWidth = 0;
  int spacing = m_pContentGrid->spacing();

  for (int col = 0; col < m_gridColCount; ++col) {
    int colWidth = 0;
    bool hasCustomWidget = false;
    bool hasButton = false;

    for (int i = 0; i < m_pContentGrid->count(); ++i) {
      QLayoutItem* item = m_pContentGrid->itemAt(i);
      int r, c, rs, cs;
      m_pContentGrid->getItemPosition(i, &r, &c, &rs, &cs);

      if (col >= c && col < c + cs) {
        QWidget* w = item->widget();
        if (!w) {
          continue;
        }

        if (qobject_cast<RibbonButton*>(w)) {
          hasButton = true;
        } else {
          hasCustomWidget = true;
          int wHint = w->sizeHint().width();
          if (w->maximumWidth() < 10000) {
            wHint = w->maximumWidth();
          }
          colWidth = qMax(colWidth, wHint);
        }
      }
    }

    if (hasCustomWidget) {
      if (colWidth == 0) {
        colWidth = 50;
      }
    } else if (hasButton) {
      if (m == Compact) {
        colWidth = 36;
      } else {
        colWidth = 70;
      }
    } else {
      colWidth = 0;
    }

    totalWidth += colWidth;
  }

  if (m_gridColCount > 0) {
    totalWidth += (m_gridColCount - 1) * spacing;
  }

  totalWidth += 10;

  return totalWidth;
}

void RibbonSection::setMode(Mode mode) {
  m_mode = mode;

  if (m_mode == Normal) {
    restoreContent();
    m_pStackLayout->setCurrentWidget(m_pExpandedWidget);
    m_pTitleLabel->show();

    QList<RibbonButton*> btns = m_pContentWidget->findChildren<RibbonButton*>();
    for (auto btn : std::as_const(btns)) {
      btn->setCompactMode(false);
    }
  } else if (m_mode == Compact) {
    restoreContent();
    m_pStackLayout->setCurrentWidget(m_pExpandedWidget);
    m_pTitleLabel->show();

    QList<RibbonButton*> btns = m_pContentWidget->findChildren<RibbonButton*>();
    for (auto btn : std::as_const(btns)) {
      btn->setCompactMode(true);
    }
  } else if (m_mode == Collapsed) {
    m_pStackLayout->setCurrentWidget(m_pCollapsedWidget);
  }

  updateGeometry();
  update();
}

void RibbonSection::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter p(this);

  if (m_mode != Collapsed) {
    p.setPen(QColor(Constants::Color::RibbonSeparator));
    p.drawLine(width() - 1, 4, width() - 1, height() - 4);
  }
}

void RibbonSection::onCollapseButtonClicked() {
  if (!m_pPopupWidget) {
    setupPopup();
  }

  QList<RibbonButton*> btns = m_pContentWidget->findChildren<RibbonButton*>();
  for (auto btn : std::as_const(btns)) {
    btn->setCompactMode(false);
  }

  m_pContentWidget->setParent(m_pPopupWidget);
  m_pContentWidget->show();
  m_pPopupWidget->layout()->addWidget(m_pContentWidget);

  m_pPopupWidget->adjustSize();

  QPoint globalPos = m_pCollapsedButton->mapToGlobal(QPoint(0, m_pCollapsedButton->height()));
  m_pPopupWidget->move(globalPos);
  m_pPopupWidget->show();
}

void RibbonSection::setupPopup() {
  m_pPopupWidget = new QWidget(nullptr, Qt::Popup);
  m_pPopupWidget->setAttribute(Qt::WA_DeleteOnClose, false);
  QVBoxLayout* layout = new QVBoxLayout(m_pPopupWidget);
  layout->setContentsMargins(5, 5, 5, 5);
  m_pPopupWidget->installEventFilter(this);

  QPalette pal = m_pPopupWidget->palette();
  pal.setColor(QPalette::Window, Qt::white);
  m_pPopupWidget->setPalette(pal);

  m_pPopupWidget->setStyleSheet(
      QString(Constants::Style::RibbonPopup).arg(QColor(Constants::Color::RibbonBorder).name(), QColor(Constants::Color::RibbonBg).name()));
}

void RibbonSection::restoreContent() {
  if (m_pContentWidget->parent() != m_pExpandedWidget) {
    if (m_pPopupWidget) {
      m_pPopupWidget->hide();
      m_pPopupWidget->layout()->removeWidget(m_pContentWidget);
    }
    m_pContentWidget->setParent(m_pExpandedWidget);
    m_pExpandedLayout->insertWidget(0, m_pContentWidget);
  }
}

bool RibbonSection::eventFilter(QObject* watched, QEvent* event) {
  if (watched == m_pPopupWidget && event->type() == QEvent::Hide) {
    restoreContent();
    setMode(m_mode);
  }
  return QWidget::eventFilter(watched, event);
}

// =============================================================================
// OfficeToolbar
// =============================================================================
OfficeToolbar::OfficeToolbar(QWidget* parent) : QWidget(parent) {
  // QPalette pal = palette();
  // pal.setColor(QPalette::Window, Qt::white);
  // setAutoFillBackground(true);
  // setPalette(pal);

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  m_pLayout->setAlignment(Qt::AlignLeft);
  setFixedHeight(Constants::RibbonHeight);
  // Fill the toolbar's full width so layoutSections() measures the real
  // available space; otherwise it sizes to its content and collapses sections
  // as soon as the contextual Format section appears.
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSize OfficeToolbar::minimumSizeHint() const {
  return QSize(200, Constants::RibbonHeight);
}

RibbonSection* OfficeToolbar::addSection(const QString& title, const QIcon& icon) {
  RibbonSection* section = new RibbonSection(title, icon, this);
  m_pLayout->addWidget(section);
  m_sections.append(section);
  return section;
}

void OfficeToolbar::addSpacer() {
  QWidget* spacer = new QWidget(this);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  m_pLayout->addWidget(spacer);
}

void OfficeToolbar::resizeEvent(QResizeEvent* event) {
  layoutSections();
  QWidget::resizeEvent(event);
}

void OfficeToolbar::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter p(this);
  p.fillRect(rect(), Qt::white);

  p.setPen(QColor(Constants::Color::RibbonBorder));
  p.drawLine(0, height() - 1, width(), height() - 1);
}

bool OfficeToolbar::event(QEvent* e) {
  // A section being shown/hidden sends a LayoutRequest, so re-run the collapse
  // logic — but only when the visible set actually changed. Reacting to every
  // LayoutRequest would loop, since layoutSections() -> setMode() ->
  // updateGeometry() posts another LayoutRequest.
  if (e->type() == QEvent::LayoutRequest) {
    int visibleCount = 0;
    for (auto sec : std::as_const(m_sections)) {
      if (!sec->isHidden()) {
        visibleCount++;
      }
    }
    if (visibleCount != m_lastVisibleCount) {
      m_lastVisibleCount = visibleCount;
      QTimer::singleShot(0, this, [this]() { layoutSections(); });
    }
  }
  return QWidget::event(e);
}

void OfficeToolbar::layoutSections() {
  // Budget against the toolbar's full width, not our own. The ribbon can be
  // sized to its content (QToolBar doesn't always honor an expanding policy),
  // so width() understates the real space — especially right when the
  // contextual Format section appears and we haven't been re-stretched yet.
  int availableWidth = width();
  if (QWidget* toolbar = parentWidget()) {
    availableWidth = qMax(availableWidth, toolbar->width());
  }
  int wAllNormal = 0;
  int wAllCompact = 0;
  int wAllCollapsed = 0;

  int visibleCount = 0;

  // 1. Calculate required space ONLY for visible sections
  for (auto sec : std::as_const(m_sections)) {
    if (sec->isHidden()) {
      continue;
    }

    wAllNormal += sec->estimateWidth(RibbonSection::Normal);
    wAllCompact += sec->estimateWidth(RibbonSection::Compact);
    wAllCollapsed += sec->estimateWidth(RibbonSection::Collapsed);
    visibleCount++;
  }

  // Calculate separators between visible sections
  const int sepWidth = visibleCount > 0 ? ((visibleCount - 1) * 5) : 0;
  wAllNormal += sepWidth;
  wAllCompact += sepWidth;
  wAllCollapsed += sepWidth;

  // 2. Apply modes based on true available width
  if (availableWidth >= wAllNormal) {
    for (auto sec : std::as_const(m_sections)) {
      if (!sec->isHidden()) {
        sec->setMode(RibbonSection::Normal);
      }
    }
  } else if (availableWidth >= wAllCompact) {
    for (auto sec : std::as_const(m_sections)) {
      if (!sec->isHidden()) {
        sec->setMode(RibbonSection::Compact);
      }
    }
  } else {
    // 3. Fallback: collapse from right to left if still out of space
    int currentW = wAllCompact;
    QVector<RibbonSection::Mode> modes(m_sections.size(), RibbonSection::Compact);

    for (int i = m_sections.size() - 1; i >= 0; --i) {
      if (m_sections[i]->isHidden()) {
        continue;
      }

      if (currentW <= availableWidth) {
        break;
      }

      const int compactW = m_sections[i]->estimateWidth(RibbonSection::Compact);
      const int collapsedW = m_sections[i]->estimateWidth(RibbonSection::Collapsed);

      currentW = currentW - compactW + collapsedW;
      modes[i] = RibbonSection::Collapsed;
    }

    for (int i = 0; i < m_sections.size(); ++i) {
      if (!m_sections[i]->isHidden()) {
        m_sections[i]->setMode(modes[i]);
      }
    }
  }
}
