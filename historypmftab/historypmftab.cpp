#include "historypmftab.h"
#include "ui_historypmftab.h"

#include <QFileDialog>
#include <QMessageBox>

HistoryPMFTab::HistoryPMFTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::HistoryPMFTab),
  mListModel(new ListModelFileList(this))
{
  ui->setupUi(this);
  ui->listViewHistoryFile->setModel(mListModel);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this, &HistoryPMFTab::loadReferencePMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this, &HistoryPMFTab::saveFile);
  connect(ui->pushButtonAdd, &QPushButton::clicked, this, &HistoryPMFTab::addHistoryFile);
  connect(ui->pushButtonRemove, &QPushButton::clicked, this, &HistoryPMFTab::removeHistoryFile);
  connect(ui->pushButtonSplit, &QPushButton::clicked, this, &HistoryPMFTab::split);
  connect(ui->pushButtonComputeRMSD, &QPushButton::clicked, this, &HistoryPMFTab::computeRMSD);
}

void HistoryPMFTab::writeRMSDToFile(const QVector<double>& rmsd, const QString &filename)
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": trying to open file " << filename;
  QFile RMSDFile(filename);
  if (RMSDFile.open(QIODevice::WriteOnly)) {
    QTextStream ofs(&RMSDFile);
    ofs.setRealNumberNotation(QTextStream::FixedNotation);
    for (int i = 0; i < rmsd.size(); ++i) {
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

HistoryPMFTab::~HistoryPMFTab()
{
  delete ui;
}

void HistoryPMFTab::loadReferencePMF()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open reference PMF file"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  if (mReferencePMF.readFromFile(inputFileName)) {
    ui->lineEditReferencePMF->setText(inputFileName);
    qDebug() << Q_FUNC_INFO << "Reading " << inputFileName << " successfully.";
  } else {
    QMessageBox errorBox;
    errorBox.critical(this, "Error",
                      "Error on opening file " + inputFileName);
  }
}

void HistoryPMFTab::saveFile()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save output to"), "");
  ui->lineEditOutputPrefix->setText(outputFileName);
}

void HistoryPMFTab::addHistoryFile()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open history PMF file"), "",
      tr("History PMF file (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  const QModelIndex& index = ui->listViewHistoryFile->currentIndex();
  mListModel->addItem(inputFileName, index);
}

void HistoryPMFTab::removeHistoryFile()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QModelIndex& index = ui->listViewHistoryFile->currentIndex();
  mListModel->removeItem(index);
}

void HistoryPMFTab::computeRMSD()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  connect(&mReaderThread, &HistoryReaderThread::done, this, &HistoryPMFTab::computeRMSDDone);
  connect(&mReaderThread, &HistoryReaderThread::progress, this, &HistoryPMFTab::computeRMSDProgress);
  const QStringList& inputFile = mListModel->trajectoryFileNameList();
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

void HistoryPMFTab::computeRMSDProgress(int fileRead, int percent)
{
  const int numFiles = mListModel->trajectoryFileNameList().size();
  const QString newText = "Reading " + QString(" (%1/%2) %3").arg(fileRead).arg(numFiles).arg(percent) + "%";
  ui->pushButtonComputeRMSD->setText(newText);
}

void HistoryPMFTab::computeRMSDDone(const HistogramPMFHistory& hist) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  mPMFHistory = hist;
  disconnect(&mReaderThread, &HistoryReaderThread::progress, this, &HistoryPMFTab::computeRMSDProgress);
  disconnect(&mReaderThread, &HistoryReaderThread::done, this, &HistoryPMFTab::computeRMSDDone);
  QVector<double> rmsd;
  if (mReferencePMF.dimension() > 0) {
    qDebug() << Q_FUNC_INFO << ": compute rmsd with respect to the reference PMF.";
    rmsd = mPMFHistory.computeRMSD(mReferencePMF.data());
  } else {
    qDebug() << Q_FUNC_INFO << ": compute rmsd with respect to the last frame of the history file.";
    rmsd = mPMFHistory.computeRMSD();
  }
  const QString& outputPrefix = ui->lineEditOutputPrefix->text();
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

void HistoryPMFTab::split()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  connect(&mReaderThread, &HistoryReaderThread::done, this, &HistoryPMFTab::splitDone);
  connect(&mReaderThread, &HistoryReaderThread::progress, this, &HistoryPMFTab::splitProgress);
  const QStringList& inputFile = mListModel->trajectoryFileNameList();
  const QString& outputPrefix = ui->lineEditOutputPrefix->text();
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

void HistoryPMFTab::splitProgress(int fileRead, int percent)
{
  const int numFiles = mListModel->trajectoryFileNameList().size();
  const QString newText = "Reading " + QString(" (%1/%2) %3").arg(fileRead).arg(numFiles).arg(percent) + "%";
  ui->pushButtonSplit->setText(newText);
}

void HistoryPMFTab::splitDone(const HistogramPMFHistory &hist)
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  mPMFHistory = hist;
  disconnect(&mReaderThread, &HistoryReaderThread::progress, this, &HistoryPMFTab::splitProgress);
  disconnect(&mReaderThread, &HistoryReaderThread::done, this, &HistoryPMFTab::splitDone);
  const QString& outputPrefix = ui->lineEditOutputPrefix->text();
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
