#ifndef NEWLAYOUTDIALOG_H
#define NEWLAYOUTDIALOG_H

#include <QDialog>

class QComboBox;
class QSpinBox;

class NewLayoutDialog : public QDialog {
  Q_OBJECT

public:
  explicit NewLayoutDialog(QWidget* parent = nullptr);

  int selectedWidth() const;
  int selectedHeight() const;

private slots:
  void onPresetChanged(int index);

private:
  QComboBox* m_pPresetCombo;
  QSpinBox* m_pWidthSpin;
  QSpinBox* m_pHeightSpin;
};

#endif  // NEWLAYOUTDIALOG_H
