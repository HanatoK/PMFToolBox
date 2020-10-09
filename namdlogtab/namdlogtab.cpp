#include "namdlogtab.h"
#include "ui_namdlogtab.h"

#include <QFileDialog>
#include <QDialog>

NAMDLogTab::NAMDLogTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::NAMDLogTab),
  mTableModel(new TableModelBinning)
{
  ui->setupUi(this);
  ui->tableViewAxis->setModel(mTableModel);
  ui->tableViewAxis->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  connect(ui->pushButtonOpenNAMDLog, &QPushButton::clicked, this, &NAMDLogTab::loadNAMDLog);
  connect(ui->pushButtonOpenColvarsTrajectory, &QPushButton::clicked, this, &NAMDLogTab::openTrajectory);
  connect(ui->pushButtonRun, &QPushButton::clicked, this, &NAMDLogTab::runBinning);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this, &NAMDLogTab::saveFile);
  connect(ui->pushButtonAddAxis, &QPushButton::clicked, this, &NAMDLogTab::addAxis);
  connect(ui->pushButtonRemoveAxis, &QPushButton::clicked, this, &NAMDLogTab::removeAxis);
  connect(&mLogReaderThread, &NAMDLogReaderThread::done, this, &NAMDLogTab::loadNAMDLogDone);
  connect(&mLogReaderThread, &NAMDLogReaderThread::progress, this, &NAMDLogTab::logReadingProgress);
  connect(&mBinningThread, &BinNAMDLogThread::doneHistogram, this, &NAMDLogTab::binningDone);
  connect(&mBinningThread, &BinNAMDLogThread::progress, this, &NAMDLogTab::binningProgress);
}

NAMDLogTab::~NAMDLogTab()
{
  delete ui;
}

void NAMDLogTab::loadNAMDLog()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open NAMD log file"), "",
      tr("All Files (*)"));
  if (inputFileName.isEmpty()) return;
  ui->lineEditNAMDLog->setText(inputFileName);
  ui->pushButtonOpenNAMDLog->setEnabled(false);
  ui->pushButtonRun->setEnabled(false);
  mLogReaderThread.invokeThread(inputFileName);
}

void NAMDLogTab::loadNAMDLogDone(NAMDLog log)
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  mLog = log;
  ui->pushButtonOpenNAMDLog->setEnabled(true);
  ui->pushButtonOpenNAMDLog->setText(tr("Open"));
  ui->pushButtonRun->setEnabled(true);
  // debug information
  qDebug() << Q_FUNC_INFO << ": available energy terms: " << mLog.getEnergyTitle();
  qDebug() << Q_FUNC_INFO << ": total frames: " << mLog.getStep().size();
}

void NAMDLogTab::logReadingProgress(int x)
{
  qDebug() << "Calling " << Q_FUNC_INFO << " progress: " << x;
  ui->pushButtonOpenNAMDLog->setText(QString::number(x) + " %");
}

void NAMDLogTab::openTrajectory()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open trajectory file"), "",
      tr("Colvars trajectory (*.traj);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  ui->lineEditColvarsTrajectory->setText(inputFileName);
}

void NAMDLogTab::saveFile()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save output to"), "",
      tr("All Files (*)"));
  ui->lineEditOutput->setText(outputFileName);
}

void NAMDLogTab::runBinning()
{
  qDebug() << Q_FUNC_INFO;
  const QStringList availableTitle = mLog.getEnergyTitle();
  if (availableTitle.size() <= 0) return;
  selectEnergyTermDialog dialog(availableTitle, this);
  if (dialog.exec()) {
    mSeletedTitle = dialog.selectedTitle();
    qDebug() << Q_FUNC_INFO << ": seleted titles" << mSeletedTitle;
  }
  const QVector<int> columns = mTableModel->fromColumns();
  const QVector<Axis> axes = mTableModel->targetAxis();
  const QString trajectoryFileName = ui->lineEditColvarsTrajectory->text();
//  const QString outputFilePrefix = ui->lineEditOutput->text();
  if (mSeletedTitle.empty() || columns.empty() || axes.empty() /*|| outputFilePrefix.isEmpty()*/ || trajectoryFileName.isEmpty()) {
    qDebug() << Q_FUNC_INFO << ": invalid input";
    return;
  }
  ui->pushButtonRun->setEnabled(false);
  mBinningThread.invokeThread(mLog, mSeletedTitle, trajectoryFileName, axes, columns);
}

void NAMDLogTab::binningProgress(QString status, int x)
{
  ui->pushButtonRun->setText(status + " " + QString::number(x) + "%");
}

void NAMDLogTab::addAxis()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QModelIndex& index = ui->tableViewAxis->currentIndex();
  mTableModel->insertRows(index.row(), 1);
  mTableModel->layoutChanged();
}

void NAMDLogTab::removeAxis()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QModelIndex& index = ui->tableViewAxis->currentIndex();
  mTableModel->removeRows(index.row(), 1);
  mTableModel->layoutChanged();
}

void NAMDLogTab::binningDone(QVector<HistogramScalar<double> > data)
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  mHistogram = data;
  const QString outputFilePrefix = ui->lineEditOutput->text();
  if (outputFilePrefix.isEmpty()) return;
  for (int i = 0; i < mSeletedTitle.size(); ++i) {
    const QString outputFileName = outputFilePrefix + "_" + mSeletedTitle[i].toLower() + ".dat";
    mHistogram[i].writeToFile(outputFileName);
  }
  ui->pushButtonRun->setText("Run binning");
  ui->pushButtonRun->setEnabled(true);
}



selectEnergyTermDialog::selectEnergyTermDialog(const QStringList &title, QWidget *parent): QDialog(parent), mAvailableTitle(title)
{
  qDebug() << Q_FUNC_INFO;
  auto gLayout = new QGridLayout;
  auto okButton = new QPushButton(tr("OK"));
  auto cancelButton = new QPushButton(tr("Cancel"));
  auto hLayout = new QHBoxLayout;
  hLayout->addWidget(okButton);
  hLayout->addWidget(cancelButton);
  for (int i = 0; i < mAvailableTitle.size(); ++i) {
    QCheckBox *checkBox = new QCheckBox(mAvailableTitle[i]);
    mCheckList.append(checkBox);
    gLayout->addWidget(checkBox, i % 4, i - i % 4);
  }
  gLayout->addLayout(hLayout, gLayout->rowCount(), 0, 1, gLayout->columnCount());
  setLayout(gLayout);
  connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
  connect(cancelButton, &QAbstractButton::clicked, this, &QDialog::reject);
  setWindowTitle(tr("Select energy terms"));
}

QStringList selectEnergyTermDialog::selectedTitle() const
{
  qDebug() << Q_FUNC_INFO;
  QStringList selected;
  for (int i = 0; i < mAvailableTitle.size(); ++i) {
    if (mCheckList[i]->isChecked()) {
      selected.append(mAvailableTitle[i]);
    }
  }
  return selected;
}
