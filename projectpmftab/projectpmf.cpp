#include "projectpmf.h"
#include "ui_projectpmf.h"

ProjectPMF::ProjectPMF(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ProjectPMF)
{
  ui->setupUi(this);
}

ProjectPMF::~ProjectPMF()
{
  delete ui;
}
