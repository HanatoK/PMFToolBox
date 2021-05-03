#include "metadynamicstab.h"
#include "ui_metadynamicstab.h"

#include <QFileDialog>

MetadynamicsTab::MetadynamicsTab(QWidget *parent) :
  QWidget(parent), ui(new Ui::MetadynamicsTab),
  mTableModel(new TableModelAxes)
{
  ui->setupUi(this);
  ui->tableViewAxis->setModel(mTableModel);
  ui->tableViewAxis->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  connect(ui->pushButtonOpen, &QPushButton::clicked, this, &MetadynamicsTab::loadTrajectory);
  connect(ui->pushButtonSave, &QPushButton::clicked, this, &MetadynamicsTab::saveFile);
}

MetadynamicsTab::~MetadynamicsTab()
{
  delete ui;
}

void MetadynamicsTab::loadTrajectory()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString inputFileName = QFileDialog::getOpenFileName(
      this, tr("Open NAMD log file"), "", tr("All Files (*)"));
  if (inputFileName.isEmpty())
    return;
  ui->lineEditInputTrajectory->setText(inputFileName);
}

void MetadynamicsTab::saveFile()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const QString outputFileName = QFileDialog::getSaveFileName(
      this, tr("Save output to"), "", tr("All Files (*)"));
  ui->lineEditOutput->setText(outputFileName);
}

void MetadynamicsTab::runSumHills()
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  // TODO
}
