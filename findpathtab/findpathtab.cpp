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
  connect(ui->pushButtonPathOnPMF, &QPushButton::clicked, this,
          &FindPathTab::plotPathOnPMF);
  connect(ui->pushButtonEnergyAlongPath, &QPushButton::clicked, this, &FindPathTab::plotEnergy);
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

void FindPathTab::plotEnergy()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const std::vector<double> energy = mPMFPathFinder.pathEnergy();
  if (energy.empty()) {
    qDebug() << Q_FUNC_INFO << ": energies are empty.";
    return;
  }
  ui->widgetPlot->plotEnergyAlongPath(energy, true);
}
