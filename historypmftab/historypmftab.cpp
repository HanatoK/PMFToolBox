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

#include "historypmftab.h"
#include "ui_historypmftab.h"

#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>

HistoryPMFTab::HistoryPMFTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::HistoryPMFTab),
      mListModel(new ListModelFileList(this)) {
  ui->setupUi(this);
  ui->listViewHistoryFile->setModel(mListModel);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this,
          &HistoryPMFTab::loadReferencePMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this,
          &HistoryPMFTab::saveFile);
  connect(ui->pushButtonAdd, &QPushButton::clicked, this,
          &HistoryPMFTab::addHistoryFile);
  connect(ui->pushButtonRemove, &QPushButton::clicked, this,
          &HistoryPMFTab::removeHistoryFile);
  connect(ui->pushButtonSplit, &QPushButton::clicked, this,
          &HistoryPMFTab::split);
  connect(ui->pushButtonComputeRMSD, &QPushButton::clicked, this,
          &HistoryPMFTab::computeRMSD);
}

void HistoryPMFTab::writeRMSDToFile(const std::vector<double> &rmsd,
                                    const QString &filename) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": trying to open file " << filename;
  QFile RMSDFile(filename);
  if (RMSDFile.open(QIODevice::WriteOnly)) {
    QTextStream ofs(&RMSDFile);
    ofs.setRealNumberNotation(QTextStream::FixedNotation);
    for (size_t i = 0; i < rmsd.size(); ++i) {
      ofs << qSetFieldWidth(OUTPUT_WIDTH);
      ofs << i;
      ofs << qSetFieldWidth(0) << ' ';
      ofs << qSetFieldWidth(OUTPUT_WIDTH);
      ofs.setRealNumberPrecision(OUTPUT_PRECISION);
      ofs << rmsd[i];
      ofs << qSetFieldWidth(0) << '\n';
    }
  } else {
    const QString errorMsg{"Cannot open output file for writing RMSD."};
    qDebug() << Q_FUNC_INFO << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
}

HistoryPMFTab::~HistoryPMFTab() { delete ui; }

void HistoryPMFTab::loadReferencePMF() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open reference PMF file"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty())
    return;
  if (mReferencePMF.readFromFile(inputFileName)) {
    ui->lineEditReferencePMF->setText(inputFileName);
    qDebug() << Q_FUNC_INFO << "Reading " << inputFileName << " successfully.";
  } else {
    QMessageBox errorBox;
    errorBox.critical(this, "Error", "Error on opening file " + inputFileName);
  }
}

void HistoryPMFTab::saveFile() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputFileName =
      QFileDialog::getSaveFileName(this, tr("Save output to"), "");
  ui->lineEditOutputPrefix->setText(outputFileName);
}

void HistoryPMFTab::addHistoryFile() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open history PMF file"), "",
      tr("History PMF file (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty())
    return;
  const QModelIndex &index = ui->listViewHistoryFile->currentIndex();
  mListModel->addItem(inputFileName, index);
}

void HistoryPMFTab::removeHistoryFile() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QModelIndex &index = ui->listViewHistoryFile->currentIndex();
  mListModel->removeItem(index);
}

void HistoryPMFTab::computeRMSD() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  connect(&mReaderThread, &HistoryReaderThread::done, this,
          &HistoryPMFTab::computeRMSDDone);
  connect(&mReaderThread, &HistoryReaderThread::progress, this,
          &HistoryPMFTab::computeRMSDProgress);
  const QStringList &inputFile = mListModel->trajectoryFileNameList();
  if (inputFile.isEmpty()) {
    const QString errorMsg{"No input file."};
    qDebug() << Q_FUNC_INFO << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  ui->pushButtonSplit->setEnabled(false);
  ui->pushButtonComputeRMSD->setEnabled(false);
  ui->pushButtonComputeRMSD->setText(tr("Running"));
  mReaderThread.readFromFile(inputFile);
}

void HistoryPMFTab::computeRMSDProgress(int fileRead, int percent) {
  const int numFiles = mListModel->trajectoryFileNameList().size();
  const QString newText =
      "Reading " +
      QString(" (%1/%2) %3").arg(fileRead).arg(numFiles).arg(percent) + "%";
  ui->pushButtonComputeRMSD->setText(newText);
}

void HistoryPMFTab::computeRMSDDone(const HistogramPMFHistory &hist) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  mPMFHistory = hist;
  disconnect(&mReaderThread, &HistoryReaderThread::progress, this,
             &HistoryPMFTab::computeRMSDProgress);
  disconnect(&mReaderThread, &HistoryReaderThread::done, this,
             &HistoryPMFTab::computeRMSDDone);
  std::vector<double> rmsd;
  if (mReferencePMF.dimension() > 0) {
    qDebug() << Q_FUNC_INFO
             << ": compute rmsd with respect to the reference PMF.";
    rmsd = mPMFHistory.computeRMSD(mReferencePMF.data());
  } else {
    qDebug()
        << Q_FUNC_INFO
        << ": compute rmsd with respect to the last frame of the history file.";
    rmsd = mPMFHistory.computeRMSD();
  }
  const QString &outputPrefix = ui->lineEditOutputPrefix->text();
  // write RMSD if output prefix is available
  if (outputPrefix.size() > 0) {
    writeRMSDToFile(rmsd, outputPrefix + "_rmsd.dat");
  }
  // plot RMSD
  ui->widgetRMSDPlot->PlotRMSD(rmsd);
  ui->pushButtonSplit->setEnabled(true);
  ui->pushButtonComputeRMSD->setEnabled(true);
  ui->pushButtonComputeRMSD->setText(tr("Compute RMSD"));
}

void HistoryPMFTab::split() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  connect(&mReaderThread, &HistoryReaderThread::done, this,
          &HistoryPMFTab::splitDone);
  connect(&mReaderThread, &HistoryReaderThread::progress, this,
          &HistoryPMFTab::splitProgress);
  const QStringList &inputFile = mListModel->trajectoryFileNameList();
  const QString &outputPrefix = ui->lineEditOutputPrefix->text();
  if (inputFile.isEmpty()) {
    const QString errorMsg{"No input file."};
    qDebug() << Q_FUNC_INFO << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  if (outputPrefix.isEmpty()) {
    const QString errorMsg{"Output prefix is empty."};
    qDebug() << Q_FUNC_INFO << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  ui->pushButtonSplit->setEnabled(false);
  ui->pushButtonComputeRMSD->setEnabled(false);
  ui->pushButtonSplit->setText("Running");
  mReaderThread.readFromFile(inputFile);
}

void HistoryPMFTab::splitProgress(int fileRead, int percent) {
  const int numFiles = mListModel->trajectoryFileNameList().size();
  const QString newText =
      "Reading " +
      QString(" (%1/%2) %3").arg(fileRead).arg(numFiles).arg(percent) + "%";
  ui->pushButtonSplit->setText(newText);
}

void HistoryPMFTab::splitDone(const HistogramPMFHistory &hist) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  mPMFHistory = hist;
  disconnect(&mReaderThread, &HistoryReaderThread::progress, this,
             &HistoryPMFTab::splitProgress);
  disconnect(&mReaderThread, &HistoryReaderThread::done, this,
             &HistoryPMFTab::splitDone);
  const QString &outputPrefix = ui->lineEditOutputPrefix->text();
  if (outputPrefix.isEmpty()) {
    const QString errorMsg{"Output prefix is empty."};
    qDebug() << Q_FUNC_INFO << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  ui->pushButtonSplit->setText(tr("Writing files"));
  mPMFHistory.splitToFile(outputPrefix);
  ui->pushButtonSplit->setEnabled(true);
  ui->pushButtonComputeRMSD->setEnabled(true);
  ui->pushButtonSplit->setText(tr("Split"));
}

HistoryCLI::HistoryCLI(QObject *parent) : CLIObject(parent) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  connect(&mReaderThread, &HistoryReaderThread::progress, this,
          &HistoryCLI::progress);
  connect(&mReaderThread, &HistoryReaderThread::done, this, &HistoryCLI::done);
  connect(&mReaderThread, &HistoryReaderThread::error, this,
          &HistoryCLI::error);
}

bool HistoryCLI::readJSON(const QString &jsonFilename) {
  if (!CLIObject::readJSON(jsonFilename)) {
    return false;
  }
  const QString referenceFilename = mLoadDoc["Reference"].toString();
  mOutputPrefix = mLoadDoc["Output"].toString();
  const QJsonArray jsonHistoryFiles = mLoadDoc["History PMF Files"].toArray();
  mDoSplitting = mLoadDoc["Split"].toBool();
  mDoComputingRMSD = mLoadDoc["RMSD"].toBool();
  for (const auto &i : jsonHistoryFiles) {
    mHistoryFilename.append(i.toString());
  }
  mReferencePMF.readFromFile(referenceFilename);
  return true;
}

void HistoryCLI::start() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  mReaderThread.readFromFile(mHistoryFilename);
}

void HistoryCLI::writeRMSDToFile(const std::vector<double> &rmsd,
                                 const QString &filename) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": trying to open file " << filename;
  QFile RMSDFile(filename);
  if (RMSDFile.open(QIODevice::WriteOnly)) {
    QTextStream ofs(&RMSDFile);
    ofs.setRealNumberNotation(QTextStream::FixedNotation);
    for (size_t i = 0; i < rmsd.size(); ++i) {
      ofs << qSetFieldWidth(OUTPUT_WIDTH);
      ofs << i;
      ofs << qSetFieldWidth(0) << ' ';
      ofs << qSetFieldWidth(OUTPUT_WIDTH);
      ofs.setRealNumberPrecision(OUTPUT_PRECISION);
      ofs << rmsd[i];
      ofs << qSetFieldWidth(0) << '\n';
    }
  } else {
    const QString errorMsg{"Cannot open output file for writing RMSD."};
    qDebug() << Q_FUNC_INFO << errorMsg;
    return;
  }
}

HistoryCLI::~HistoryCLI() { qDebug() << "Calling" << Q_FUNC_INFO; }

void HistoryCLI::progress(int fileRead, int percent) {
  qDebug() << "Reading file " << mHistoryFilename[fileRead] << " (" << percent
           << "%)";
}

void HistoryCLI::done(const HistogramPMFHistory &hist) {
  qDebug() << "Calling slot" << Q_FUNC_INFO;
  mPMFHistory = hist;
  if (mDoComputingRMSD) {
    std::vector<double> rmsd;
    if (mReferencePMF.dimension() > 0) {
      qDebug() << Q_FUNC_INFO
               << ": compute rmsd with respect to the reference PMF.";
      rmsd = mPMFHistory.computeRMSD(mReferencePMF.data());
    } else {
      qDebug() << Q_FUNC_INFO
               << ": compute rmsd with respect to the last frame of the "
                  "history file.";
      rmsd = mPMFHistory.computeRMSD();
    }
    if (mOutputPrefix.size() > 0) {
      writeRMSDToFile(rmsd, mOutputPrefix + "_rmsd.dat");
    }
  }
  if (mDoSplitting) {
    mPMFHistory.splitToFile(mOutputPrefix);
  }
  qDebug() << "Operation succeeded.";
  emit allDone();
}

void HistoryCLI::error(QString msg) {
  qDebug() << msg;
  // TODO: should emit a separate error signal.
  emit allDone();
}
