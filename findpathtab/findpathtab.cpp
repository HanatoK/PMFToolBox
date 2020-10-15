#include "findpathtab.h"
#include "ui_findpathtab.h"

#include <QFileDialog>
#include <QMessageBox>

FindPathTab::FindPathTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::FindPathTab) {
  ui->setupUi(this);
  setupAvailableAlgorithms();
  connect(ui->pushButtonOpen, &QPushButton::clicked, this,
          &FindPathTab::loadPMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this,
          &FindPathTab::saveFile);
  connect(ui->pushButtonFind, &QPushButton::clicked, this,
          &FindPathTab::findPath);
  connect(&mPMFPathFinderThread, &PMFPathFinderThread::PathFinderDone, this,
          &FindPathTab::findPathDone);
}

FindPathTab::~FindPathTab() { delete ui; }

void FindPathTab::setupAvailableAlgorithms() {
  qDebug() << "Calling " << Q_FUNC_INFO;
  mAvailableAlgorithms["Dijkstra's algorithm"] =
      Graph::FindPathAlgorithm::Dijkstra;
  mAvailableAlgorithms["Shortest path faster algorithm (SPFA)"] =
      Graph::FindPathAlgorithm::SPFA;
  for (auto it = mAvailableAlgorithms.cbegin();
       it != mAvailableAlgorithms.cend(); ++it) {
    ui->comboBoxAlgorithm->addItem(it.key());
  }
}

Graph::FindPathAlgorithm FindPathTab::selectedAlgorithm() const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString selectedText = ui->comboBoxAlgorithm->currentText();
  return mAvailableAlgorithms[selectedText];
}

void FindPathTab::loadPMF() {
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open input PMF file"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty())
    return;
  if (mPMF.readFromFile(inputFileName)) {
    ui->lineEditInputPMF->setText(inputFileName);
    qDebug() << Q_FUNC_INFO << "Reading " << inputFileName << " successfully.";
  } else {
    QMessageBox errorBox;
    errorBox.critical(this, "Error", "Error on opening file " + inputFileName);
  }
}

void FindPathTab::saveFile() {
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save output files to"), "", tr("All Files (*)"));
  ui->lineEditOutput->setText(outputFileName);
}

void FindPathTab::findPath() {
  qDebug() << "Calling " << Q_FUNC_INFO;
  const std::vector<double> posStart =
      splitStringToNumbers<double>(ui->lineEditStart->text());
  const std::vector<double> posEnd =
      splitStringToNumbers<double>(ui->lineEditEnd->text());
  const Graph::FindPathAlgorithm algorithm = selectedAlgorithm();
  // TODO: allow to use a list of patches
  std::vector<GridDataPatch> patchList;
  const Graph::FindPathMode mode = Graph::FindPathMode::MFEPMode;
  // check
  if (mPMF.dimension() == 0) {
    const QString errorMsg{"Invalid PMF input!"};
    qDebug() << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  if (mPMF.dimension() == 1) {
    const QString errorMsg{"MFEP path finder is not available for 1D PMF."};
    qDebug() << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  if (posStart.size() != mPMF.dimension() ||
      posEnd.size() != mPMF.dimension()) {
    const QString errorMsg{"Dimensionality of starting or ending"};
    return;
  }
  mPMFPathFinder =
      PMFPathFinder(mPMF, patchList, posStart, posEnd, mode, algorithm);
  mPMFPathFinderThread.findPath(mPMFPathFinder);
  ui->pushButtonFind->setEnabled(false);
  ui->pushButtonFind->setText(tr("Running"));
}

void FindPathTab::findPathDone(const PMFPathFinder &result) {
  mPMFPathFinder = result;
  mPMFPathFinder.writePath(ui->lineEditOutput->text() + ".path");
  mPMFPathFinder.writeVisitedRegion(ui->lineEditOutput->text() + ".region");
  ui->pushButtonFind->setText(tr("Find"));
  ui->pushButtonFind->setEnabled(true);
}
