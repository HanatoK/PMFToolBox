#include "findpathtab.h"
#include "ui_findpathtab.h"

FindPathTab::FindPathTab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FindPathTab)
{
  ui->setupUi(this);
}

FindPathTab::~FindPathTab()
{
  delete ui;
}
