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
  // TODO
}

void HistoryPMFTab::split()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  connect(&mReaderThread, &HistoryReaderThread::done, this, &HistoryPMFTab::splitDone);
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

void HistoryPMFTab::splitDone(const HistogramPMFHistory &hist)
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  mPMFHistory = hist;
  disconnect(&mReaderThread, &HistoryReaderThread::done, this, &HistoryPMFTab::splitDone);
  const QString& outputPrefix = ui->lineEditOutputPrefix->text();
  if (outputPrefix.isEmpty()) {
    const QString errorMsg{"Output prefix is empty."};
    qDebug() << Q_FUNC_INFO << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  mPMFHistory.splitToFile(outputPrefix);
  ui->pushButtonSplit->setEnabled(true);
  ui->pushButtonComputeRMSD->setEnabled(true);
  ui->pushButtonSplit->setText(tr("Split"));
}
