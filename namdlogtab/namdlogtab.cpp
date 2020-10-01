#include "namdlogtab.h"
#include "ui_namdlogtab.h"

#include <QFileDialog>

NAMDLogTab::NAMDLogTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::NAMDLogTab)
{
  ui->setupUi(this);
  connect(ui->pushButtonOpenNAMDLog, &QPushButton::clicked, this, &NAMDLogTab::loadNAMDLog);
  connect(&mLogReaderThread, &NAMDLogReaderThread::done, this, &NAMDLogTab::loadNAMDLogDone);
  connect(&mLogReaderThread, &NAMDLogReaderThread::progress, this, &NAMDLogTab::logReadingProgress);
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
  mLogReaderThread.invokeThread(inputFileName);
}

void NAMDLogTab::loadNAMDLogDone(NAMDLog log)
{
  mLog = log;
  ui->pushButtonOpenNAMDLog->setEnabled(true);
  ui->pushButtonOpenNAMDLog->setText(tr("Open"));
}

void NAMDLogTab::logReadingProgress(int x)
{
  ui->pushButtonOpenNAMDLog->setText(QString::number(x) + " %");
}
