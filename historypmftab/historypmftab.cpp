#include "historypmftab.h"
#include "ui_historypmftab.h"

#include <QFileDialog>
#include <QMessageBox>

HistoryPMFTab::HistoryPMFTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::HistoryPMFTab)
{
  ui->setupUi(this);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this, &HistoryPMFTab::loadReferencePMF);
  connect(ui->pushButtonSaveTo, &QPushButton::clicked, this, &HistoryPMFTab::saveFile);
}

HistoryPMFTab::~HistoryPMFTab()
{
  delete ui;
}

void HistoryPMFTab::loadReferencePMF()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open reference PMF file"), "",
      tr("Potential of Mean force (*.pmf);;All Files (*)"));
  if (inputFileName.isEmpty()) return;
  if (mReferencePMF.readFromFile(inputFileName)) {
    ui->lineEditReferencePMF->setText(inputFileName);
    qDebug() << Q_FUNC_INFO << "Reading " << inputFileName << " successfully.";
  } else {
    QMessageBox errorBox;
    errorBox.critical(this, "Error",
                      "Error on opening file " + inputFileName);
  }
}

void HistoryPMFTab::saveFile()
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save output to"), "");
  ui->lineEditOutputPrefix->setText(outputFileName);
}
