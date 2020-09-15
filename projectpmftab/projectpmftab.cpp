#include "projectpmftab.h"
#include "ui_projectpmftab.h"

ProjectPMFTab::ProjectPMFTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ProjectPMFTab)
{
  ui->setupUi(this);
}

ProjectPMFTab::~ProjectPMFTab()
{
  delete ui;
}
