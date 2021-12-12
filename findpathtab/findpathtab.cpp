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

#include "findpathtab.h"
#include "ui_findpathtab.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

FindPathTab::FindPathTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::FindPathTab),
      mPatchTable(new PatchTableModel(this)) {
  ui->setupUi(this);
  setupAvailableAlgorithms();
  ui->tableViewPatch->setModel(mPatchTable);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this,
          &FindPathTab::loadPMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this,
          &FindPathTab::saveFile);
  connect(ui->pushButtonFind, &QPushButton::clicked, this,
          &FindPathTab::findPath);
  connect(&mPMFPathFinderThread, &PMFPathFinderThread::PathFinderDone, this,
          &FindPathTab::findPathDone);
  connect(ui->pushButtonPathOnPMF, &QPushButton::clicked, this,
          &FindPathTab::plotPathOnPMF);
  connect(ui->pushButtonEnergyAlongPath, &QPushButton::clicked, this,
          &FindPathTab::plotEnergy);
  connect(ui->pushButtonAddPatch, &QPushButton::clicked, this,
          &FindPathTab::showAddPatchDialog);
  connect(ui->pushButtonRemovePatch, &QPushButton::clicked, this,
          &FindPathTab::removePatch);
}

FindPathTab::~FindPathTab() { delete ui; }

void FindPathTab::setupAvailableAlgorithms() {
  qDebug() << "Calling" << Q_FUNC_INFO;
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
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString selectedText = ui->comboBoxAlgorithm->currentText();
  return mAvailableAlgorithms[selectedText];
}

void FindPathTab::addPatch(const QString &center, const QVector<double> &length,
                           const double value) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << "add patch:"
           << "center =" << center;
  for (int i = 0; i < length.size(); ++i)
    qDebug() << Q_FUNC_INFO << "length" << i << "=" << length[i];
  qDebug() << Q_FUNC_INFO << "value =" << value;

  mPatchTable->setDimension(length.size());
  mPatchTable->insertRows(0, 1, QModelIndex());
  QModelIndex index = mPatchTable->index(0, 0, QModelIndex());
  mPatchTable->setData(index, center, Qt::EditRole);
  for (size_t i = 0; i < mPatchTable->dimension(); ++i) {
    index = mPatchTable->index(0, i + 1, QModelIndex());
    mPatchTable->setData(index, length[i], Qt::EditRole);
  }
  index = mPatchTable->index(0, mPatchTable->dimension() + 1, QModelIndex());
  mPatchTable->setData(index, value, Qt::EditRole);
  ui->tableViewPatch->setCurrentIndex(ui->tableViewPatch->currentIndex());
}

void FindPathTab::loadPMF() {
  qDebug() << "Calling" << Q_FUNC_INFO;
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
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save output files to"), "", tr("All Files (*)"));
  ui->lineEditOutput->setText(outputFileName);
}

void FindPathTab::findPath() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const std::vector<double> posStart =
      splitStringToNumbers<double>(ui->lineEditStart->text());
  const std::vector<double> posEnd =
      splitStringToNumbers<double>(ui->lineEditEnd->text());
  const Graph::FindPathAlgorithm algorithm = selectedAlgorithm();
  // TODO: allow to use a list of patches
  const auto tmp_patchList = mPatchTable->patchList();
  std::vector<GridDataPatch> patchList(tmp_patchList.begin(),
                                       tmp_patchList.end());
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
  if (!mPMFPathFinder.patchList().empty()) {
    mPMFPathFinder.writePatchedPMF(ui->lineEditOutput->text() + ".patched");
  }
  ui->pushButtonFind->setText(tr("Find"));
  ui->pushButtonFind->setEnabled(true);
}

void FindPathTab::plotPathOnPMF() {
  const size_t dimension = mPMFPathFinder.histogram().dimension();
  if (dimension == 0) {
    const QString errorMsg(tr("PMF and path are not loaded or initialized."));
    qDebug() << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  if (dimension != 2) {
    const QString errorMsg(tr("You are trying to load a ") +
                           QString::number(dimension) +
                           tr("D PMF, but only 2D PMF is supported."));
    qDebug() << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  ui->widgetPlot->plotPMF2D(mPMFPathFinder.histogram());
  ui->widgetPlot->plotPath2D(mPMFPathFinder.pathPosition());
}

void FindPathTab::plotEnergy() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const std::vector<double> energy = mPMFPathFinder.pathEnergy();
  if (energy.empty()) {
    qDebug() << Q_FUNC_INFO << ": energies are empty.";
    return;
  }
  ui->widgetPlot->plotEnergyAlongPath(energy, true);
}

void FindPathTab::showAddPatchDialog() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (mPMF.histogramSize() == 0 || mPMF.dimension() == 0) {
    QMessageBox errorBox;
    const QString errorMsg =
        QString("You need to open a valid PMF file first.");
    errorBox.critical(this, "Error", errorMsg);
    qDebug() << errorMsg;
    return;
  }

  AddPatchDialog aDialog(mPMF.dimension(), this);
  if (aDialog.exec()) {
    QVector<double> length(mPMF.dimension());
    for (int i = 0; i < length.size(); ++i) {
      length[i] = aDialog.length(i);
    }
    addPatch(aDialog.center(), length, aDialog.value());
  }
}

void FindPathTab::removePatch() {
  qDebug() << Q_FUNC_INFO;
  const QModelIndexList indexes =
      ui->tableViewPatch->selectionModel()->selectedIndexes();
  for (QModelIndex index : indexes) {
    int row = index.row();
    mPatchTable->removeRows(row, 1, QModelIndex());
  }
  ui->tableViewPatch->setCurrentIndex(ui->tableViewPatch->currentIndex());
}

FindPathCLI::FindPathCLI(QObject *parent): CLIObject(parent)
{
  connect(&mPMFPathFinderThread, &PMFPathFinderThread::PathFinderDone, this,
          &FindPathCLI::findPathDone);
}

bool FindPathCLI::readJSON(const QString &jsonFilename)
{
  if (!CLIObject::readJSON(jsonFilename)) {
    return false;
  }
  mInputPMF = mLoadDoc["Input PMF"].toString();
  mOutputPrefix = mLoadDoc["Output"].toString();
  mStart = mLoadDoc["Start"].toString();
  mEnd = mLoadDoc["End"].toString();
  mAlgorithm = mLoadDoc["Algorithm"].toInt();
  const QJsonArray jsonPatches = mLoadDoc["Patches"].toArray();
  for (const auto &a: jsonPatches) {
    const auto jsonPatch = a.toObject();
    const QString patchCenter = jsonPatch["Center"].toString();
    const QString patchLength = jsonPatch["Lengths"].toString();
    const double patchValue = jsonPatch["Value"].toDouble();
    mPatchList.push_back(GridDataPatch{splitStringToNumbers<double>(patchCenter),
                                       splitStringToNumbers<double>(patchLength),
                                       patchValue});
  }
  HistogramScalar<double> inputPMFHistogram;
  const Graph::FindPathMode mode = Graph::FindPathMode::MFEPMode;
  if (inputPMFHistogram.readFromFile(mInputPMF)) {
    // TODO: check if the input is valid!
    mPMFPathFinder.setup(inputPMFHistogram, mPatchList,
                         splitStringToNumbers<double>(mStart),
                         splitStringToNumbers<double>(mEnd),
                         mode, static_cast<Graph::FindPathAlgorithm>(mAlgorithm));
    return true;
  } else {
    qWarning() << "Failed to read from" << mInputPMF;
    return false;
  }
}

void FindPathCLI::start()
{
  mPMFPathFinderThread.findPath(mPMFPathFinder);
}

FindPathCLI::~FindPathCLI()
{

}

void FindPathCLI::findPathDone(const PMFPathFinder &result)
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  mPMFPathFinder = result;
  mPMFPathFinder.writePath(mOutputPrefix + ".path");
  mPMFPathFinder.writeVisitedRegion(mOutputPrefix + ".region");
  if (!mPMFPathFinder.patchList().empty()) {
    mPMFPathFinder.writePatchedPMF(mOutputPrefix + ".patched");
  }
  emit allDone();
}

PathPMFInPMFCLI::PathPMFInPMFCLI(QObject* parent): CLIObject(parent)
{

}

bool PathPMFInPMFCLI::readJSON(const QString& jsonFilename)
{
  if (!CLIObject::readJSON(jsonFilename)) {
    return false;
  }
  mInputPMF = mLoadDoc["Input PMF"].toString();
  mInputPathFile = mLoadDoc["Input Path File"].toString();
  mOutput = mLoadDoc["Output"].toString();
}

void PathPMFInPMFCLI::start()
{
  HistogramScalar<double> inputPMFHistogram;
  if (inputPMFHistogram.readFromFile(mInputPMF)) {
    QFile pathFile(mInputPathFile);
    QFile outputFile(mOutput);
    if (pathFile.open(QFile::ReadOnly) &&
        outputFile.open(QFile::WriteOnly)) {
      QTextStream ifs(&pathFile);
      QTextStream ofs(&outputFile);
      QString line;
      std::vector<double> pos(inputPMFHistogram.dimension(), 0);
      const QRegularExpression split_regex("\\s+");
      QVector<QStringRef> tmpFields;
      while (!ifs.atEnd()) {
        ifs.readLineInto(&line);
        tmpFields = line.splitRef(split_regex, Qt::SkipEmptyParts);
        if (tmpFields[0].startsWith("#")) continue;
        else {
          if (tmpFields.size() == static_cast<int>(pos.size())) {
            for (size_t i = 0; i < pos.size(); ++i) {
              pos[i] = tmpFields[i].toDouble();
              ofs << qSetFieldWidth(OUTPUT_WIDTH);
              ofs.setRealNumberPrecision(OUTPUT_POSITION_PRECISION);
              ofs << pos[i];
              ofs << qSetFieldWidth(0) << ' ';
            }
            if (inputPMFHistogram.isInGrid(pos)) {
              // find the position in PMF
              const double pmf_val = inputPMFHistogram(pos);
              ofs.setRealNumberPrecision(OUTPUT_PRECISION);
              ofs << qSetFieldWidth(OUTPUT_WIDTH) << pmf_val << qSetFieldWidth(0);
            }
            ofs << "\n";
          }
        }
      }
    }
  } else {
    qWarning() << "Failed to read from" << mInputPMF;
  }
}

PathPMFInPMFCLI::~PathPMFInPMFCLI()
{

}
