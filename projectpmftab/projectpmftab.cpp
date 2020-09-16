#include "projectpmftab.h"
#include "ui_projectpmftab.h"
#include "base/helper.h"

#include <QFileDialog>
#include <QMessageBox>

void ProjectPMFTab::loadPMF()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open input PMF file"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  if (mOriginPMF.readFromFile(inputFileName)) {
    ui->lineEditInputPMF->setText(inputFileName);
    qDebug() << Q_FUNC_INFO << "Reading " << inputFileName << " successfully.";
  } else {
    QMessageBox errorBox;
    errorBox.critical(this, "Error",
                      "Error on opening file " + inputFileName);
  }
}

void ProjectPMFTab::saveFile()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save reweighted PMF file to"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  ui->lineEditOutput->setText(outputFileName);
}

void ProjectPMFTab::projectPMF()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QVector<size_t> toAxis = splitStringToNumbers<size_t>(ui->lineEditProjectTo->text());
  const QString saveFile = ui->lineEditOutput->text();
  const double kbt = kbT(ui->lineEditTemperature->text().toDouble(), ui->comboBoxUnit->currentText());
  // check origin PMF
  if (mOriginPMF.dimension() == 0 || mOriginPMF.histogramSize() == 0) {
    const QString errorMsg("PMF file is not loaded or invalid!");
    qDebug() << Q_FUNC_INFO << ": " << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  // check output
  if (saveFile.isEmpty()) {
    const QString errorMsg("No output file is selected.");
    qDebug() << Q_FUNC_INFO << ": " << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    return;
  }
  // check axis input
  for (int i = 0; i < toAxis.size(); ++i) {
    if (toAxis[i] >= mOriginPMF.dimension()) {
      const QString errorMsg = QString("Axis %1 is larger than the dimensionality of the PMF.").arg(toAxis[i]);
      qDebug() << Q_FUNC_INFO << ": " << errorMsg;
      QMessageBox errorBox;
      errorBox.critical(this, "Error", errorMsg);
    }
  }
  // transform the PMF to probability histogram
  HistogramProbability p;
  mOriginPMF.toProbability(p, kbt);
  // marginalize the probability
  p = p.reduceDimension(toAxis);
  // convert the marginal probability back to the target PMF
  mProjectedPMF.fromProbability(p, kbt);
  // output
  mProjectedPMF.writeToFile(saveFile);
}

void ProjectPMFTab::plotOriginPMF()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  switch (mOriginPMF.dimension()) {
  case 0: {
    const QString errorMsg("PMF file is not loaded or invalid!");
    qDebug() << Q_FUNC_INFO << ": " << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    break;
  }
  case 1: {
    qDebug() << Q_FUNC_INFO << ": plot 1D PMF";
    ui->widgetPlotPMF->plotPMF1D(mOriginPMF);
    break;
  }
  case 2: {
    qDebug() << Q_FUNC_INFO << ": plot 2D PMF";
    ui->widgetPlotPMF->plotPMF2D(mOriginPMF);
    break;
  }
  default: {
    const QString errorMsg = QString("%1D plotting is not implemented.").arg(mOriginPMF.dimension());
    qDebug() << Q_FUNC_INFO << ": " << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    break;
  }
  }
}

void ProjectPMFTab::plotProjectedPMF()
{
  // TODO: debug
  qDebug() << "Calling " << Q_FUNC_INFO;
  switch (mProjectedPMF.dimension()) {
  case 0: {
    const QString errorMsg("Projected PMF is not calculated.");
    qDebug() << Q_FUNC_INFO << ": " << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    break;
  }
  case 1: {
    qDebug() << Q_FUNC_INFO << ": plot 1D PMF";
    ui->widgetPlotPMF->plotPMF1D(mProjectedPMF);
    break;
  }
  case 2: {
    qDebug() << Q_FUNC_INFO << ": plot 2D PMF";
    ui->widgetPlotPMF->plotPMF2D(mProjectedPMF);
    break;
  }
  default: {
    const QString errorMsg = QString("%1D plotting is not implemented.").arg(mProjectedPMF.dimension());
    qDebug() << Q_FUNC_INFO << ": " << errorMsg;
    QMessageBox errorBox;
    errorBox.critical(this, "Error", errorMsg);
    break;
  }
  }
}

ProjectPMFTab::ProjectPMFTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ProjectPMFTab)
{
  ui->setupUi(this);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this, &ProjectPMFTab::loadPMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this, &ProjectPMFTab::saveFile);
  connect(ui->pushButtonProjectAndSave, &QPushButton::clicked, this, &ProjectPMFTab::projectPMF);
  connect(ui->pushButtonPlotOriginPMF, &QPushButton::clicked, this, &ProjectPMFTab::plotOriginPMF);
  connect(ui->pushButtonPlotProjectedPMF, &QPushButton::clicked, this, &ProjectPMFTab::plotProjectedPMF);
}

ProjectPMFTab::~ProjectPMFTab()
{
  delete ui;
}
