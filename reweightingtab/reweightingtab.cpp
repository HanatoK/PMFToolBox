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
  connect(ui->pushButtonRun, &QPushButton::clicked, this, &ReweightingTab::reweighting);
  connect(ui->pushButtonHelp, &QPushButton::clicked, this, &ReweightingTab::help);
  connect(&mWorkerThread, &ReweightingThread::error, this, &ReweightingTab::reweightingError);
  connect(&mWorkerThread, &ReweightingThread::progress, this, &ReweightingTab::reweightingProgress);
  connect(&mWorkerThread, &ReweightingThread::done, this, &ReweightingTab::reweightingDone);
}

ReweightingTab::~ReweightingTab() { delete ui; }

double ReweightingTab::getKbT() const
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const double temperature = ui->lineEditTemperature->text().toDouble();
  const QString unit = ui->comboBoxUnit->currentText();
  double factor = 1.0;
  if (unit.compare("kcal/mol", Qt::CaseInsensitive) == 0) {
#ifdef SI2019
    factor = 0.001985875;
#else
    factor = 0.0019872041;
#endif
  } else if (unit.compare("kj/mol", Qt::CaseInsensitive) == 0) {
#ifdef SI2019
    factor = 0.008314463;
#else
    factor = 0.0083144621;
#endif
  } else {
    qDebug() << Q_FUNC_INFO << ": undefined unit factor, use " << factor;
  }
  const double kbt = temperature * factor;
  return kbt;
}

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
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save reweighted PMF file to"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
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
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QStringList fileList = mListModel->trajectoryFileNameList();
  const QVector<int> fromColumns = mTableModel->fromColumns();
  const QVector<int> toColumns = mTableModel->toColumns();
  const QVector<Axis> targetAxis = mTableModel->targetAxis();
  const QString outputFileName = ui->lineEditOutput->text();
  const bool usePMF = ui->checkBoxConvertToPMF->isChecked();
  if (fileList.isEmpty()) {
    const QString errorMsg("No trajectory file selected.");
    qDebug() << errorMsg;
    qDebug() << "Trajectory file list: " << fileList;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  if (fromColumns.isEmpty() || toColumns.isEmpty() || targetAxis.isEmpty()) {
    const QString errorMsg("Incorrect axis settings.");
    qDebug() << errorMsg;
    qDebug() << "From columns: " << fromColumns;
    qDebug() << "To columns: " << toColumns;
    qDebug() << "Target histogram axis: " << targetAxis;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  if (outputFileName.isEmpty()) {
    const QString errorMsg("No output file specified.");
    qDebug() << errorMsg;
    qDebug() << "Selected output file: " << outputFileName;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  ui->pushButtonRun->setText(tr("Running"));
  ui->pushButtonRun->setEnabled(false);
  mWorkerThread.reweighting(fileList, outputFileName, mPMF, fromColumns, toColumns, targetAxis, getKbT(), usePMF);
}

void ReweightingTab::reweightingProgress(int fileRead, int percent)
{
  const int numFiles = mListModel->trajectoryFileNameList().size();
  const QString newText = "Running " + QString(" (%1/%2) %3").arg(fileRead).arg(numFiles).arg(percent) + "%";
  ui->pushButtonRun->setText(newText);
}

void ReweightingTab::reweightingError(QString msg)
{
  QMessageBox errorBox;
  errorBox.critical(this, "Error", msg);
  ui->pushButtonRun->setEnabled(true);
  ui->pushButtonRun->setText(tr("Run"));
}

void ReweightingTab::reweightingDone()
{
  ui->pushButtonRun->setEnabled(true);
  ui->pushButtonRun->setText(tr("Run"));
}

void ReweightingTab::help()
{
  // TODO
}
