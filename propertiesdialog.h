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
  // Dragging Logic overrides
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

private slots:
  void onValueChanged();

private:
  QGraphicsItem* m_currentItem;
  bool m_blockSignals;

  QSpinBox* xSpin;
  QSpinBox* ySpin;
  QSpinBox* wSpin;
  QSpinBox* hSpin;
  QLabel* typeLabel;

  // Dragging state
  bool m_isDragging;
  QPoint m_dragOffset;
};

#endif  // PROPERTIESDIALOG_H
