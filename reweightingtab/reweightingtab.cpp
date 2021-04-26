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

#include "reweightingtab.h"
#include "ui_reweightingtab.h"
#include "base/helper.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ReweightingTab::ReweightingTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::ReweightingTab),
      mTableModel(new TableModelReweightingAxis(this)),
      mListModel(new ListModelFileList(this)) {
  ui->setupUi(this);
  ui->tableViewReweightingAxis->setModel(mTableModel);
  ui->tableViewReweightingAxis->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  ui->listViewTrajectory->setModel(mListModel);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this, &ReweightingTab::loadPMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this, &ReweightingTab::saveFile);
  connect(ui->pushButtonAddTrajectory, &QPushButton::clicked, this, &ReweightingTab::addTrajectory);
  connect(ui->pushButtonRemoveTrajectory, &QPushButton::clicked, this, &ReweightingTab::removeTrajectory);
  connect(ui->pushButtonClearAll, &QPushButton::clicked, this, &ReweightingTab::clearTrajectory);
  connect(ui->pushButtonReadAxes, &QPushButton::clicked, this, &ReweightingTab::readAxisData);
  connect(ui->pushButtonRun, &QPushButton::clicked, this, &ReweightingTab::reweighting);
  connect(&mWorkerThread, &ReweightingThread::error, this, &ReweightingTab::reweightingError);
  connect(&mWorkerThread, &ReweightingThread::progress, this, &ReweightingTab::reweightingProgress);
  connect(&mWorkerThread, &ReweightingThread::done, this, &ReweightingTab::reweightingDone);
}

ReweightingTab::~ReweightingTab() { delete ui; }

double ReweightingTab::getKbT() const
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const double temperature = ui->lineEditTemperature->text().toDouble();
  const QString unit = ui->comboBoxUnit->currentText();
  return kbT(temperature, unit);
}

void ReweightingTab::loadPMF()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
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

void ReweightingTab::saveFile()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save reweighted PMF file to"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  ui->lineEditOutput->setText(outputFileName);
}

void ReweightingTab::addTrajectory()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QStringList inputFileName = QFileDialog::getOpenFileNames(
      this, tr("Open trajectory file"), "",
      tr("Colvars trajectory (*.traj);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  const QModelIndex& index = ui->listViewTrajectory->currentIndex();
  mListModel->addItems(inputFileName, index);
}

void ReweightingTab::removeTrajectory()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QModelIndex& index = ui->listViewTrajectory->currentIndex();
  mListModel->removeItem(index);
}

void ReweightingTab::clearTrajectory()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  mListModel->clearAll();
}

void ReweightingTab::readAxisData()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  mTableModel->clearAll();
  const std::vector<int> fromAxis = splitStringToNumbers<int>(ui->lineEditFromColumns->text());
  const std::vector<int> toAxis = splitStringToNumbers<int>(ui->lineEditToColumns->text());
  if (fromAxis.empty() || toAxis.empty()) {
    qDebug() << Q_FUNC_INFO << ": axes are empty.";
    return;
  }
  if (fromAxis.size() != mPMF.dimension()) {
    QMessageBox errorBox;
    errorBox.critical(this, "Error",
                      "The dimensionality of PMF input doesn't match the "
                      "columns reweighting from.");
    return;
  }
  const std::vector<Axis> &ax = mPMF.axes();
  for (size_t i = 0; i < ax.size(); ++i) {
    auto find_in_reweightTo =
        std::find(toAxis.begin(), toAxis.end(), fromAxis[i]);
    if (find_in_reweightTo != toAxis.end()) {
      mTableModel->addItem(ax[i], fromAxis[i], true, true);
    } else {
      mTableModel->addItem(ax[i], fromAxis[i], true, false);
    }
  }
  for (size_t i = 0; i < toAxis.size(); ++i) {
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
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QStringList fileList = mListModel->trajectoryFileNameList();
  const std::vector<int> fromColumns = mTableModel->fromColumns();
  const std::vector<int> toColumns = mTableModel->toColumns();
  const std::vector<Axis> targetAxis = mTableModel->targetAxis();
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
  if (fromColumns.empty() || toColumns.empty() || targetAxis.empty()) {
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
  const QString newText = "Running " + QString(" (%1/%2) %3").arg(fileRead+1).arg(numFiles).arg(percent) + "%";
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

ReweightingCLI::ReweightingCLI(QObject *parent): QObject(parent) {
  connect(&mWorkerThread, &ReweightingThread::error, this, &ReweightingCLI::reweightingError);
  connect(&mWorkerThread, &ReweightingThread::progress, this, &ReweightingCLI::reweightingProgress);
  connect(&mWorkerThread, &ReweightingThread::done, this, &ReweightingCLI::reweightingDone);
}

void ReweightingCLI::reweightingProgress(int fileRead, int percent) {
  qDebug() << "Reading file " << mFileList[fileRead] << " (" << percent << "%)";
}

void ReweightingCLI::reweightingError(QString msg) {
  qDebug() << msg;
  emit allDone();
}

void ReweightingCLI::reweightingDone()
{
  qDebug() << "Calling slot" << Q_FUNC_INFO;
  qDebug() << "Operation succeeded.";
  emit allDone();
}

bool ReweightingCLI::readJSON(const QString &jsonFilename)
{
  qDebug() << "Reading" << jsonFilename;
  QFile loadFile(jsonFilename);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning() << QString("Could not open json file") + jsonFilename;
    return false;
  }
  QByteArray jsonData = loadFile.readAll();
  QJsonParseError jsonParseError;
  const QJsonDocument loadDoc(QJsonDocument::fromJson(jsonData, &jsonParseError));
  if (loadDoc.isNull()) {
    qWarning() << QString("Invalid json file:") + jsonFilename;
    qWarning() << "Json parse error:" << jsonParseError.errorString();
    return false;
  }
  const QString inputFilename = loadDoc["Input"].toString();
  mOutputFilename = loadDoc["Output"].toString();
  const QJsonArray jsonFromColumns = loadDoc["From columns"].toArray();
  const QJsonArray jsonTocolumns = loadDoc["To columns"].toArray();
  const QString unit = loadDoc["Unit"].toString();
  const double temperature = loadDoc["Temperature"].toDouble();
  mConvertToPMF = loadDoc["Convert to PMF"].toBool();
  const QJsonArray jsonTrajectories = loadDoc["Trajectories"].toArray();
  const QJsonArray jsonReweightingAxes = loadDoc["Reweighting Axes"].toArray();
  for (const auto& i : jsonTrajectories) {
    mFileList.push_back(i.toString());
  }
  for (const auto& i: jsonFromColumns) {
    mFromColumns.push_back(i.toInt());
  }
  for (const auto& i: jsonTocolumns) {
    mToColumns.push_back(i.toInt());
  }
  for (const auto& a: jsonReweightingAxes) {
    const auto nested_json = a.toObject();
    const int target_column = nested_json["Target Column"].toInt();
    qDebug() << "Read target column: " << target_column;
    const auto find_result = std::find(mToColumns.begin(), mToColumns.end(), target_column);
    if (find_result != mToColumns.end()) {
      const double lower_bound = nested_json["Lower bound"].toDouble();
      const double upper_bound = nested_json["Upper bound"].toDouble();
      const size_t nbins = std::nearbyint((upper_bound - lower_bound) / nested_json["Width"].toDouble());
      mTargetAxis.push_back(Axis(lower_bound, upper_bound, nbins));
    } else {
      qDebug() << "Target column not found!";
      qDebug() << "Problem json data: " << nested_json;
      return false;
    }
  }
  mInputPMF.readFromFile(inputFilename);
  mKbT = kbT(temperature, unit);
  return true;
}

void ReweightingCLI::start()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  mWorkerThread.reweighting(mFileList, mOutputFilename, mInputPMF,
                            mFromColumns, mToColumns, mTargetAxis,
                            mKbT, mConvertToPMF);
}

ReweightingCLI::~ReweightingCLI()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
}
