#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  mReweightingTab = new ReweightingTab(this);
  ui->tabWidget->addTab(mReweightingTab, "Reweighting");
}

MainWindow::~MainWindow()
{
  delete ui;
}

