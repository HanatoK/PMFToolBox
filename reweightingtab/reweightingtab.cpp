#include "reweightingtab.h"
#include "ui_reweightingtab.h"
#include "lib/helper.h"

#include <QFileDialog>
#include <QMessageBox>

ReweightingTab::ReweightingTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::ReweightingTab),
      mTableModel(new TableModelReweightingAxis(this)),
      mListModel(new ListModelTrajectory(this)) {
  ui->setupUi(this);
  ui->tableViewReweightingAxis->setModel(mTableModel);
  ui->listViewTrajectory->setModel(mListModel);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this, &ReweightingTab::loadPMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this, &ReweightingTab::loadSaveFile);
  connect(ui->pushButtonAddTrajectory, &QPushButton::clicked, this, &ReweightingTab::addTrajectory);
  connect(ui->pushButtonRemoveTrajectory, &QPushButton::clicked, this, &ReweightingTab::removeTrajectory);
  connect(ui->pushButtonReadAxes, &QPushButton::clicked, this, &ReweightingTab::readAxisData);
  connect(&mWorkerThread, &ReweightingThread::done, this, &ReweightingTab::reweightingDone);
}

ReweightingTab::~ReweightingTab() { delete ui; }

void ReweightingTab::loadPMF()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open input PMF file"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  if (mPMF.readFromFile(inputFileName)) {
    ui->lineEditInputPMF->setText(inputFileName);
    qDebug() << Q_FUNC_INFO << "Reading " << inputFileName << " successfully.";
  } else {
    QMessageBox errorBox;
    errorBox.critical(this, "Error",
                      "Error on opening file " + inputFileName);
  }
}

void ReweightingTab::loadSaveFile()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(this, tr("Save to"), "");
  ui->lineEditOutput->setText(outputFileName);
}

void ReweightingTab::addTrajectory()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open trajectory file"), "",
      tr("Colvars trajectory (*.traj);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  const QModelIndex& index = ui->listViewTrajectory->currentIndex();
  mListModel->addItem(inputFileName, index);
}

void ReweightingTab::removeTrajectory()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QModelIndex& index = ui->listViewTrajectory->currentIndex();
  mListModel->removeItem(index);
}

void ReweightingTab::readAxisData()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  mTableModel->clearAll();
  const QVector<int> fromAxis = splitStringToNumbers<int>(ui->lineEditFromColumns->text());
  const QVector<int> toAxis = splitStringToNumbers<int>(ui->lineEditToColumns->text());
  if (fromAxis.isEmpty() || toAxis.isEmpty()) {
    qDebug() << Q_FUNC_INFO << ": axes are empty.";
    return;
  }
  if (fromAxis.size() != static_cast<int>(mPMF.dimension())) {
    QMessageBox errorBox;
    errorBox.critical(this, "Error",
                      "The dimensionality of PMF input doesn't match the "
                      "columns reweighting from.");
    return;
  }
  const QVector<Axis> &ax = mPMF.axes();
  for (int i = 0; i < ax.size(); ++i) {
    auto find_in_reweightTo =
        std::find(toAxis.begin(), toAxis.end(), fromAxis[i]);
    if (find_in_reweightTo != toAxis.end()) {
      mTableModel->addItem(ax[i], fromAxis[i], true, true);
    } else {
      mTableModel->addItem(ax[i], fromAxis[i], true, false);
    }
  }
  for (int i = 0; i < toAxis.size(); ++i) {
    auto find_in_reweightFrom =
        std::find(fromAxis.begin(), fromAxis.end(), toAxis[i]);
    if (find_in_reweightFrom == fromAxis.end()) {
      Axis axis(0, 1, 1, false);
      mTableModel->addItem(axis, toAxis[i], false, true);
    }
  }
}

void ReweightingTab::reweighting()
{
//  const QVector<int> fromAxis = splitStringToNumbers<int>(ui->lineEditFromColumns->text());
//  const QVector<int> toAxis = splitStringToNumbers<int>(ui->lineEditToColumns->text());
  // TODO
}

void ReweightingTab::reweightingDone()
{
  ui->pushButtonRun->setEnabled(true);
  ui->pushButtonRun->setText(tr("Run"));
}
