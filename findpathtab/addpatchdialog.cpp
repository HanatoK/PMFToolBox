#include "addpatchdialog.h"

#include <QtWidgets>

AddPatchDialog::AddPatchDialog(int dimension, QWidget *parent)
    : QDialog(parent), mDimension(dimension), mLengthText(dimension, nullptr) {
  mCenterText = new QLineEdit;
  for (size_t i = 0; i < mDimension; ++i) {
    mLengthText[i] = new QLineEdit;
  }
  mValueText = new QLineEdit;

  auto centerLabel = new QLabel(tr("Center"));
  QVector<QLabel *> lengthLabel(mDimension, nullptr);
  for (size_t i = 0; i < mDimension; ++i) {
    QString label_i = tr("Length ") + QString::number(i + 1);
    lengthLabel[i] = new QLabel(label_i);
  }
  auto valueLabel = new QLabel(tr("Value"));

  auto gLayout = new QGridLayout;
  gLayout->setColumnStretch(1, 2);
  gLayout->addWidget(centerLabel, 0, 0);
  for (size_t i = 0; i < mDimension; ++i) {
    gLayout->addWidget(lengthLabel[i], i + 1, 0);
  }
  gLayout->addWidget(valueLabel, mDimension + 1, 0);

  gLayout->addWidget(mCenterText, 0, 1);
  for (size_t i = 0; i < mDimension; ++i) {
    gLayout->addWidget(mLengthText[i], i + 1, 1);
  }
  gLayout->addWidget(mValueText, mDimension + 1, 1);

  auto okButton = new QPushButton(tr("OK"));
  auto cancelButton = new QPushButton(tr("Cancel"));
  auto buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(okButton);
  buttonLayout->addWidget(cancelButton);
  gLayout->addLayout(buttonLayout, mDimension + 2, 1, Qt::AlignRight);

  auto mainLayout = new QVBoxLayout;
  mainLayout->addLayout(gLayout);
  setLayout(mainLayout);

  connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
  connect(cancelButton, &QAbstractButton::clicked, this, &QDialog::reject);

  setWindowTitle(tr("Add a Patch"));
}

QString AddPatchDialog::center() const { return mCenterText->text(); }

double AddPatchDialog::length(int i) const {
  return mLengthText[i]->text().toDouble();
}

double AddPatchDialog::value() const { return mValueText->text().toDouble(); }

size_t AddPatchDialog::dimension() const { return mDimension; }

void AddPatchDialog::setDimension(int dimension) { mDimension = dimension; }
