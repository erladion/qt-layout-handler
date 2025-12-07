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
class QStackedLayout;

class RibbonButton : public QToolButton {
  Q_OBJECT
public:
  enum Type { Large, Small };
  explicit RibbonButton(QAction* action, Type type, QWidget* parent = nullptr);

  void setCompactMode(bool compact);

private:
  Type m_type;
};

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
  void paintEvent(QPaintEvent* event) override;

private slots:
  void onCollapseButtonClicked();

private:
  void setupPopup();
  void restoreContent();

  QString m_title;
  QIcon m_sectionIcon;
  Mode m_mode;

  QStackedLayout* m_pStackLayout;

  // Expanded View (Content + Title)
  QWidget* m_pExpandedWidget;
  QVBoxLayout* m_pExpandedLayout;

  // Collapsed View (Centered Icon)
  QWidget* m_pCollapsedWidget;

  QWidget* m_pContentWidget;
  QGridLayout* m_pContentGrid;

  QLabel* m_pTitleLabel;
  QToolButton* m_pCollapsedButton;

  QWidget* m_pPopupWidget;
  int m_gridColCount;

  QIcon m_representativeIcon;
};

class OfficeToolbar : public QWidget {
  Q_OBJECT
public:
  explicit OfficeToolbar(QWidget* parent = nullptr);

  RibbonSection* addSection(const QString& title, const QIcon& icon);
  void addSpacer();

  QSize minimumSizeHint() const override;

protected:
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

private:
  void layoutSections();

  QHBoxLayout* m_pLayout;
  QList<RibbonSection*> m_sections;
};

#endif  // OFFICETOOLBAR_H
