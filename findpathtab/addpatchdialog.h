#ifndef ADDPATCHDIALOG_H
#define ADDPATCHDIALOG_H

#include <QObject>
#include <QDialog>
#include <QLineEdit>

class AddPatchDialog : public QDialog
{
  Q_OBJECT
public:
  AddPatchDialog(QWidget *parent = nullptr, int dimension = 0);
  QString center() const;
  double length(int i) const;
  double value() const;
  size_t dimension() const;
  void setDimension(int dimension);

private:
  size_t mDimension;
  QLineEdit *mCenterText;
  QVector<QLineEdit*> mLengthText;
  QLineEdit *mValueText;
};

#endif // ADDPATCHDIALOG_H
