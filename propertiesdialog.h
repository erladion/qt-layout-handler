#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include <QWidget>

class QSpinBox;
class QGraphicsItem;
class QLabel;

class PropertiesDialog : public QWidget {
  Q_OBJECT

public:
  explicit PropertiesDialog(QWidget* parent = nullptr);

  void setItem(QGraphicsItem* item);
  void refreshValues();

signals:
  void propertyChanged();

protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

private slots:
  void onValueChanged();

private:
  QGraphicsItem* m_pCurrentItem;
  bool m_blockSignals;

  QSpinBox* m_pXSpin;
  QSpinBox* m_pYSpin;
  QSpinBox* m_pWSpin;
  QSpinBox* m_pHSpin;
  QLabel* m_pTypeLabel;

  bool m_isDragging;
  QPoint m_dragOffset;
};

#endif  // PROPERTIESDIALOG_H
