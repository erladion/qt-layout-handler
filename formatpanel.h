#ifndef FORMATPANEL_H
#define FORMATPANEL_H

#include <QColor>
#include <QObject>
#include <QString>

class OfficeToolbar;
class RibbonSection;
class QSpinBox;
class QPushButton;
class QWidget;
class QGraphicsItem;
class QAbstractGraphicsShapeItem;

// Owns the contextual "Format" ribbon section that edits the line width and the
// line/fill colours of the currently selected drawn shape.
class FormatPanel : public QObject {
  Q_OBJECT
public:
  // Builds the (initially hidden) Format section into the given ribbon.
  FormatPanel(OfficeToolbar* ribbon, const QString& spinStyle, QObject* parent = nullptr);

  // Shows and populates the section for the selection's lead item. Returns true
  // if it is a drawn shape this panel handles, so the caller can suppress the
  // properties dialog for it.
  bool updateForSelection(QGraphicsItem* item);

private:
  void onLineWidthChanged(int val);
  void onLineColorClicked();
  void onFillColorClicked();
  void updateButtonColor(QPushButton* btn, const QColor& color);

  RibbonSection* m_pSection = nullptr;
  QSpinBox* m_pLineWidthSpin = nullptr;
  QPushButton* m_pLineColorBtn = nullptr;
  QPushButton* m_pFillColorBtn = nullptr;
  QWidget* m_pFillContainer = nullptr;

  QAbstractGraphicsShapeItem* m_pCurrentShape = nullptr;
};

#endif  // FORMATPANEL_H
