#include "metadynamicstab.h"
#include "ui_metadynamicstab.h"

#include <QFileDialog>

MetadynamicsTab::MetadynamicsTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::MetadynamicsTab),
      mTableModel(new TableModelAxes) {
  ui->setupUi(this);
  ui->tableViewAxis->setModel(mTableModel);
  ui->tableViewAxis->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this,
          &MetadynamicsTab::loadTrajectory);
  connect(ui->pushButtonSave, &QPushButton::clicked, this,
          &MetadynamicsTab::saveFile);
  connect(ui->checkBoxWellTempered, &QCheckBox::toggled, this,
          &MetadynamicsTab::toggleWellTempered);
  connect(ui->pushButtonRun, &QPushButton::clicked, this,
          &MetadynamicsTab::runSumHills);
  connect(ui->pushButtonAddAxis, &QPushButton::clicked, this,
          &MetadynamicsTab::addAxis);
  connect(ui->pushButtonRemoveAxis, &QPushButton::clicked, this,
          &MetadynamicsTab::removeAxis);
  connect(&mWorkerThread, &SumHillsThread::stridedResult, this,
          &MetadynamicsTab::intermediate);
  connect(&mWorkerThread, &SumHillsThread::done, this, &MetadynamicsTab::done);
  connect(&mWorkerThread, &SumHillsThread::progress, this,
          &MetadynamicsTab::progress);
  ui->lineEditDeltaT->setEnabled(ui->checkBoxWellTempered->isChecked());
  ui->lineEditTemperature->setEnabled(ui->checkBoxWellTempered->isChecked());
}

MetadynamicsTab::~MetadynamicsTab() { delete ui; }

void MetadynamicsTab::loadTrajectory() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open NAMD log file"), "", tr("All Files (*)"));
  if (inputFileName.isEmpty())
    return;
  ui->lineEditInputTrajectory->setText(inputFileName);
}

void MetadynamicsTab::intermediate(qint64 step, HistogramScalar<double> PMF,
                                   HistogramVector<double> gradients) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputPMFFilename =
      ui->lineEditOutput->text() + "_" + QString::number(step) + ".pmf";
  const QString outputGradFilename =
      ui->lineEditOutput->text() + "_" + QString::number(step) + ".grad";
  if (ui->checkBoxWellTempered->isChecked()) {
    const double temperature = ui->lineEditTemperature->text().toDouble();
    const double deltaT = ui->lineEditDeltaT->text().toDouble();
    Metadynamics::writePMF(PMF, outputPMFFilename, true, deltaT, temperature);
    Metadynamics::writeGradients(gradients, outputGradFilename, true, deltaT,
                                 temperature);
  } else {
    Metadynamics::writePMF(PMF, outputPMFFilename, false, 0.0, 1.0);
    Metadynamics::writeGradients(gradients, outputGradFilename, false, 0.0,
                                 1.0);
  }
}

void MetadynamicsTab::done(HistogramScalar<double> PMF,
                           HistogramVector<double> gradients) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputPMFFilename = ui->lineEditOutput->text() + ".pmf";
  const QString outputGradFilename = ui->lineEditOutput->text() + ".grad";
  if (ui->checkBoxWellTempered->isChecked()) {
    const double temperature = ui->lineEditTemperature->text().toDouble();
    const double deltaT = ui->lineEditDeltaT->text().toDouble();
    Metadynamics::writePMF(PMF, outputPMFFilename, true, deltaT, temperature);
    Metadynamics::writeGradients(gradients, outputGradFilename, true, deltaT,
                                 temperature);
  } else {
    Metadynamics::writePMF(PMF, outputPMFFilename, false, 0.0, 1.0);
    Metadynamics::writeGradients(gradients, outputGradFilename, false, 0.0,
                                 1.0);
  }
  ui->pushButtonRun->setEnabled(true);
  ui->pushButtonRun->setText(tr("Run"));
}

void MetadynamicsTab::saveFile() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputFilename = QFileDialog::getSaveFileName(
      this, tr("Save output to"), "", tr("All Files (*)"));
  ui->lineEditOutput->setText(outputFilename);
}

void MetadynamicsTab::runSumHills() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const std::vector<Axis> ax = mTableModel->targetAxis();
  const qint64 strides = std::nearbyint(ui->doubleSpinBoxStrides->value());
  const QString &inputFilename = ui->lineEditInputTrajectory->text();
  if (ax.empty() || inputFilename.size() == 0) {
    // TODO: handle error
    return;
  }
  ui->pushButtonRun->setEnabled(false);
  mWorkerThread.sumHills(ax, strides, inputFilename);
}

void MetadynamicsTab::toggleWellTempered(bool enableWellTempered) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << enableWellTempered;
  ui->lineEditDeltaT->setEnabled(enableWellTempered);
  ui->lineEditTemperature->setEnabled(enableWellTempered);
}

void MetadynamicsTab::addAxis() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QModelIndex &index = ui->tableViewAxis->currentIndex();
  mTableModel->insertRows(index.row(), 1);
  emit mTableModel->layoutChanged();
}

void MetadynamicsTab::removeAxis() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QModelIndex &index = ui->tableViewAxis->currentIndex();
  mTableModel->removeRows(index.row(), 1);
  emit mTableModel->layoutChanged();
}

void MetadynamicsTab::progress(qint64 percent) {
  ui->pushButtonRun->setText(QString("%1 %").arg(percent));
}

MetadynamicsCLI::MetadynamicsCLI(QObject *parent): QObject(parent)
{
  connect(&mWorkerThread, &SumHillsThread::progress, this, &MetadynamicsCLI::progress);
  connect(&mWorkerThread, &SumHillsThread::done, this, &MetadynamicsCLI::done);
  connect(&mWorkerThread, &SumHillsThread::stridedResult, this, &MetadynamicsCLI::intermediate);
  connect(&mWorkerThread, &SumHillsThread::error, this, &MetadynamicsCLI::error);
}

bool MetadynamicsCLI::readJSON(const QString &jsonFilename)
{
  qDebug() << "Reading" << jsonFilename;
  QFile loadFile(jsonFilename);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning() << QString("Could not open json file") + jsonFilename;
    return false;
  }
  QByteArray jsonData = loadFile.readAll();
  QJsonParseError jsonParseError;
  const QJsonDocument loadDoc(
      QJsonDocument::fromJson(jsonData, &jsonParseError));
  if (loadDoc.isNull()) {
    qWarning() << QString("Invalid json file:") + jsonFilename;
    qWarning() << "Json parse error:" << jsonParseError.errorString();
    return false;
  }
  //TODO
  mTrajectoryFilename = loadDoc["Trajectory"].toString();
  mOutputPrefix = loadDoc["Output"].toString();
  const QJsonArray jsonAxes = loadDoc["Axes"].toArray();
  for (int i = 0; i < jsonAxes.size(); ++i) {
    const auto jsonAxis = jsonAxes[i].toObject();
    const double lowerBound = jsonAxis["Lower bound"].toDouble();
    const double upperBound = jsonAxis["Upper bound"].toDouble();
    const double width = jsonAxis["Width"].toDouble();
    const size_t bins = std::nearbyint((upperBound - lowerBound) / width);
    const bool periodic = jsonAxis["Periodic"].toBool();
    mAxes.push_back(Axis(lowerBound, upperBound, bins, periodic));
  }
  mIsWellTempered = loadDoc["Well tempered"].toBool(false);
  if (mIsWellTempered) {
    mDeltaT = loadDoc["Bias temperature"].toDouble();
    mTemperature = loadDoc["Temperature"].toDouble();
  }
  mStride = loadDoc["Stride"].toInt();
  return true;
}

void MetadynamicsCLI::start()
{
  mWorkerThread.sumHills(mAxes, mStride, mTrajectoryFilename);
}

MetadynamicsCLI::~MetadynamicsCLI()
{

}

void MetadynamicsCLI::progress(int percent)
{
  qInfo() << "Metadynamics sum hills: " << percent << "%";
}

void MetadynamicsCLI::error(QString msg)
{
  qWarning() << msg;
  emit allDone();
}

void MetadynamicsCLI::intermediate(qint64 step, HistogramScalar<double> PMF, HistogramVector<double> gradients)
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputPMFFilename =
      mOutputPrefix + "_" + QString::number(step) + ".pmf";
  const QString outputGradFilename =
      mOutputPrefix + "_" + QString::number(step) + ".grad";
  if (mIsWellTempered) {
    Metadynamics::writePMF(PMF, outputPMFFilename, true, mDeltaT, mTemperature);
    Metadynamics::writeGradients(gradients, outputGradFilename, true, mDeltaT,
                                 mTemperature);
  } else {
    Metadynamics::writePMF(PMF, outputPMFFilename, false, 0.0, 1.0);
    Metadynamics::writeGradients(gradients, outputGradFilename, false, 0.0,
                                 1.0);
  }
}

void MetadynamicsCLI::done(HistogramScalar<double> PMF, HistogramVector<double> gradients)
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputPMFFilename = mOutputPrefix + ".pmf";
  const QString outputGradFilename = mOutputPrefix + ".grad";
  if (mIsWellTempered) {
    Metadynamics::writePMF(PMF, outputPMFFilename, true, mDeltaT, mTemperature);
    Metadynamics::writeGradients(gradients, outputGradFilename, true, mDeltaT,
                                 mTemperature);
  } else {
    Metadynamics::writePMF(PMF, outputPMFFilename, false, 0.0, 1.0);
    Metadynamics::writeGradients(gradients, outputGradFilename, false, 0.0,
                                 1.0);
  }
  emit allDone();
}
