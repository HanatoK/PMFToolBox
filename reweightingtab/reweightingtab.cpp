#include "reweightingtab.h"
#include "ui_reweightingtab.h"

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
