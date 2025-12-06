#ifndef OFFICETOOLBAR_H
#define OFFICETOOLBAR_H

#include <QAction>
#include <QList>
#include <QToolButton>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;
class QGridLayout;
class QLabel;
class RibbonSection;
class QStackedLayout;  // NEW

// --- 1. Custom Ribbon Button ---
class RibbonButton : public QToolButton {
  Q_OBJECT
public:
  enum Type { Large, Small };
  explicit RibbonButton(QAction* action, Type type, QWidget* parent = nullptr);

  // Helper to switch visual style dynamically
  void setCompactMode(bool compact);

private:
  Type m_type;
};

// --- 2. Ribbon Section ---
class RibbonSection : public QWidget {
  Q_OBJECT
public:
  enum Mode { Normal, Compact, Collapsed };

  explicit RibbonSection(const QString& title, const QIcon& icon, QWidget* parent = nullptr);
  ~RibbonSection();

  void addLargeButton(RibbonButton* btn);
  void addSmallButton(RibbonButton* btn, int col = 0);
  void addWidget(QWidget* widget, int row, int col);

  void setMode(Mode mode);
  Mode mode() const { return m_mode; }

  QSize minimumSizeHint() const override;
  QSize sizeHint() const override;

  int estimateWidth(Mode m) const;

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  // Manual painting to avoid stylesheet inheritance issues
  void paintEvent(QPaintEvent* event) override;

private slots:
  void onCollapseButtonClicked();

private:
  void setupPopup();
  void restoreContent();

  QString m_title;
  QIcon m_sectionIcon;
  Mode m_mode;

  // REFACTORED: Stacked Layout for switching between content and collapsed icon
  QStackedLayout* m_stackLayout;

  // Expanded View (Content + Title)
  QWidget* m_expandedWidget;
  QVBoxLayout* m_expandedLayout;

  // Collapsed View (Centered Icon)
  QWidget* m_collapsedWidget;

  QWidget* m_contentWidget;
  QGridLayout* m_contentGrid;

  QLabel* m_titleLbl;
  QToolButton* m_collapsedBtn;

  QWidget* m_popupWidget;
  int m_gridColCount;

  QIcon m_representativeIcon;
};

// --- 3. Main Office Toolbar ---
class OfficeToolbar : public QWidget {
  Q_OBJECT
public:
  explicit OfficeToolbar(QWidget* parent = nullptr);

  RibbonSection* addSection(const QString& title, const QIcon& icon);
  void addSpacer();

  QSize minimumSizeHint() const override;

protected:
  void resizeEvent(QResizeEvent* event) override;
  // Manual painting for background and bottom border
  void paintEvent(QPaintEvent* event) override;

private:
  void layoutSections();

  QHBoxLayout* m_layout;
  QList<RibbonSection*> m_sections;
};

#endif  // OFFICETOOLBAR_H
