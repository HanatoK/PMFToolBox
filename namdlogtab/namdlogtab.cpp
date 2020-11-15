/*
  PMFToolBox: A toolbox to analyze and post-process the output of
  potential of mean force calculations.
  Copyright (C) 2020  Haochuan Chen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "namdlogtab.h"
#include "ui_namdlogtab.h"

#include <QFileDialog>
#include <QDialog>

NAMDLogTab::NAMDLogTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::NAMDLogTab),
  mTableModel(new TableModelBinning)
{
  ui->setupUi(this);
  ui->tableViewAxis->setModel(mTableModel);
  ui->tableViewAxis->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  connect(ui->pushButtonOpenNAMDLog, &QPushButton::clicked, this, &NAMDLogTab::loadNAMDLog);
  connect(ui->pushButtonOpenColvarsTrajectory, &QPushButton::clicked, this, &NAMDLogTab::openTrajectory);
  connect(ui->pushButtonRun, &QPushButton::clicked, this, &NAMDLogTab::runBinning);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this, &NAMDLogTab::saveFile);
  connect(ui->pushButtonAddAxis, &QPushButton::clicked, this, &NAMDLogTab::addAxis);
  connect(ui->pushButtonRemoveAxis, &QPushButton::clicked, this, &NAMDLogTab::removeAxis);
  connect(&mLogReaderThread, &NAMDLogReaderThread::done, this, &NAMDLogTab::loadNAMDLogDone);
  connect(&mLogReaderThread, &NAMDLogReaderThread::progress, this, &NAMDLogTab::logReadingProgress);
  connect(&mBinningThread, &BinNAMDLogThread::doneHistogram, this, &NAMDLogTab::binningDone);
  connect(&mBinningThread, &BinNAMDLogThread::progress, this, &NAMDLogTab::binningProgress);
}

NAMDLogTab::~NAMDLogTab()
{
  delete ui;
}

void NAMDLogTab::loadNAMDLog()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open NAMD log file"), "",
      tr("All Files (*)"));
  if (inputFileName.isEmpty()) return;
  ui->lineEditNAMDLog->setText(inputFileName);
  ui->pushButtonOpenNAMDLog->setEnabled(false);
  ui->pushButtonRun->setEnabled(false);
  mLogReaderThread.invokeThread(inputFileName);
}

void NAMDLogTab::loadNAMDLogDone(NAMDLog log)
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  mLog = log;
  ui->pushButtonOpenNAMDLog->setEnabled(true);
  ui->pushButtonOpenNAMDLog->setText(tr("Open"));
  ui->pushButtonRun->setEnabled(true);
  // debug information
  qDebug() << Q_FUNC_INFO << ": available energy terms: " << mLog.getEnergyTitle();
  qDebug() << Q_FUNC_INFO << ": available force terms: " << mLog.getForceTitle();
  qDebug() << Q_FUNC_INFO << ": total frames: " << mLog.getStep().size();
}

void NAMDLogTab::logReadingProgress(int x)
{
  qDebug() << "Calling " << Q_FUNC_INFO << " progress: " << x;
  ui->pushButtonOpenNAMDLog->setText(QString::number(x) + " %");
}

void NAMDLogTab::openTrajectory()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open trajectory file"), "",
      tr("Colvars trajectory (*.traj);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  ui->lineEditColvarsTrajectory->setText(inputFileName);
}

void NAMDLogTab::saveFile()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save output to"), "",
      tr("All Files (*)"));
  ui->lineEditOutput->setText(outputFileName);
}

void NAMDLogTab::runBinning()
{
  qDebug() << Q_FUNC_INFO;
  const QStringList availableEnergyTitle = mLog.getEnergyTitle();
  const QStringList availableForceTitle = mLog.getForceTitle();
  if (availableEnergyTitle.size() <= 0 && availableForceTitle.size() <= 0) {
    qDebug() << Q_FUNC_INFO << ": no available energy or force terms in the log file.";
    return;
  }
  selectEnergyTermDialog dialog(availableEnergyTitle, availableForceTitle, this);
  if (dialog.exec()) {
    mSelectedEnergyTitle = dialog.selectedEnergyTitle();
    mSelectedForceTitle = dialog.selectedForceTitle();
    qDebug() << Q_FUNC_INFO << ": seleted energy terms:" << mSelectedEnergyTitle;
    qDebug() << Q_FUNC_INFO << ": seleted force terms:" << mSelectedForceTitle;
  }
  const std::vector<int> columns = mTableModel->fromColumns();
  const std::vector<Axis> axes = mTableModel->targetAxis();
  const QString trajectoryFileName = ui->lineEditColvarsTrajectory->text();
  const QString outputFilePrefix = ui->lineEditOutput->text();
  if (mSelectedEnergyTitle.isEmpty() && mSelectedForceTitle.isEmpty()) {
    qDebug() << Q_FUNC_INFO << ": no force or energy terms are selected.";
    return;
  }
  if (trajectoryFileName.isEmpty()) {
    qDebug() << Q_FUNC_INFO << ": invalid trajectory file.";
    return;
  }
  if (columns.empty() || axes.empty()) {
    qDebug() << Q_FUNC_INFO << ": invalid axes setting.";
    return;
  }
  if (outputFilePrefix.isEmpty()) {
    qDebug() << Q_FUNC_INFO << ": output file is unspecified.";
    return;
  }
  ui->pushButtonRun->setEnabled(false);
  mBinningThread.invokeThread(mLog, mSelectedEnergyTitle, mSelectedForceTitle, trajectoryFileName, axes, columns);
}

void NAMDLogTab::binningProgress(QString status, int x)
{
  ui->pushButtonRun->setText(status + " " + QString::number(x) + "%");
}

void NAMDLogTab::addAxis()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QModelIndex& index = ui->tableViewAxis->currentIndex();
  mTableModel->insertRows(index.row(), 1);
  mTableModel->layoutChanged();
}

void NAMDLogTab::removeAxis()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QModelIndex& index = ui->tableViewAxis->currentIndex();
  mTableModel->removeRows(index.row(), 1);
  mTableModel->layoutChanged();
}

void NAMDLogTab::binningDone(std::vector<HistogramScalar<double> > energyData, std::vector<HistogramVector<double> > forceData)
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  mEnergyHistogram = energyData;
  mForceHistogram = forceData;
  const QString outputFilePrefix = ui->lineEditOutput->text();
  if (outputFilePrefix.isEmpty()) return;
  for (int i = 0; i < mSelectedEnergyTitle.size(); ++i) {
    const QString outputFileName = outputFilePrefix + "_" + mSelectedEnergyTitle[i].toLower() + ".dat";
    mEnergyHistogram[i].writeToFile(outputFileName);
  }
  for (int i = 0; i < mSelectedForceTitle.size(); ++i) {
    const QString outputFileName = outputFilePrefix + "_" + mSelectedForceTitle[i].toLower() + ".dat";
    mForceHistogram[i].writeToFile(outputFileName);
  }
  ui->pushButtonRun->setText("Run binning");
  ui->pushButtonRun->setEnabled(true);
}



selectEnergyTermDialog::selectEnergyTermDialog(const QStringList &energyTitle, const QStringList &forceTitle, QWidget *parent):
  QDialog(parent), mAvailableEnergyTitle(energyTitle), mAvailableForceTitle(forceTitle)
{
  qDebug() << Q_FUNC_INFO;
  auto gLayout = new QGridLayout;
  auto okButton = new QPushButton(tr("OK"));
  auto cancelButton = new QPushButton(tr("Cancel"));
  auto hLayout = new QHBoxLayout;
  hLayout->addWidget(okButton);
  hLayout->addWidget(cancelButton);
  int j = 0;
  for (int i = 0; i < mAvailableEnergyTitle.size(); ++i, ++j) {
    QCheckBox *checkBox = new QCheckBox(mAvailableEnergyTitle[i]);
    mEnergyCheckList.append(checkBox);
    gLayout->addWidget(checkBox, j % 4, j - j % 4);
  }
  for (int i = 0; i < mAvailableForceTitle.size(); ++i, ++j) {
    QCheckBox *checkBox = new QCheckBox(mAvailableForceTitle[i]);
    mForceCheckList.append(checkBox);
    gLayout->addWidget(checkBox, j % 4, j - j % 4);
  }
  gLayout->addLayout(hLayout, gLayout->rowCount(), 0, 1, gLayout->columnCount());
  setLayout(gLayout);
  connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
  connect(cancelButton, &QAbstractButton::clicked, this, &QDialog::reject);
  setWindowTitle(tr("Select energy and force terms"));
}

QStringList selectEnergyTermDialog::selectedEnergyTitle() const
{
  qDebug() << Q_FUNC_INFO;
  QStringList selected;
  for (int i = 0; i < mAvailableEnergyTitle.size(); ++i) {
    if (mEnergyCheckList[i]->isChecked()) {
      selected.append(mAvailableEnergyTitle[i]);
    }
  }
  return selected;
}

QStringList selectEnergyTermDialog::selectedForceTitle() const
{
  qDebug() << Q_FUNC_INFO;
  QStringList selected;
  for (int i = 0; i < mAvailableForceTitle.size(); ++i) {
    if (mForceCheckList[i]->isChecked()) {
      selected.append(mAvailableForceTitle[i]);
    }
  }
  return selected;
}
