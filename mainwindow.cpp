#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  mReweightingTab = new ReweightingTab(this);
  mProjectPMFTab = new ProjectPMFTab(this);
  mHistoryPMFTab = new HistoryPMFTab(this);
  mNAMDLogTab = new NAMDLogTab(this);
  mFindPathTab = new FindPathTab(this);
  ui->tabWidget->addTab(mReweightingTab, "Reweighting");
  ui->tabWidget->addTab(mProjectPMFTab, "Project PMF");
  ui->tabWidget->addTab(mHistoryPMFTab, "History PMF");
  ui->tabWidget->addTab(mNAMDLogTab, "NAMD log");
  ui->tabWidget->addTab(mFindPathTab, "Find MFEP");
  // I'm still wondering if this is a good idea to auto resize the main window
  // https://forum.qt.io/topic/119614/auto-resize-the-mainwindow-to-fit-the-content-in-the-tab-widget/4
//  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateSizes);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
  qDebug() << "old size: " << event->oldSize() << " ; new size:" << event->size();
  QMainWindow::resizeEvent(event);
  qDebug() << "After resize: " << this->size();
}

void MainWindow::updateSizes(int index)
{
  // FIXME: I don't know how to resize the window according to the tab widgest correctly
  // https://stackoverflow.com/questions/29128936/qtabwidget-size-depending-on-current-tab
  qDebug() << "Calling " << Q_FUNC_INFO;
  for (int i = 0; i < ui->tabWidget->count(); ++i) {
    if (i != index) {
      ui->tabWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
  }
  ui->tabWidget->widget(index)->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  qDebug() << "tab widget " << index << " size hint: " << ui->tabWidget->widget(index)->minimumSizeHint();
  ui->tabWidget->widget(index)->resize(ui->tabWidget->widget(index)->minimumSizeHint());
  ui->tabWidget->widget(index)->adjustSize();
  qDebug() << "tab widget " << index << " size after resizing: " << ui->tabWidget->widget(index)->size();
  qDebug() << "tab size hint: " << ui->tabWidget->minimumSizeHint();
  ui->tabWidget->resize(ui->tabWidget->minimumSizeHint());
//  ui->tabWidget->adjustSize();
  qDebug() << "tab size after resizing: " << ui->tabWidget->size();
  qDebug() << "central layout size hint: " << ui->centralwidget->minimumSizeHint();
  ui->centralwidget->resize(ui->centralwidget->minimumSizeHint());
//  ui->centralwidget->adjustSize();
  qDebug() << "central layout size after resizing: " << ui->centralwidget->size();
  qDebug() << "mainwindow size hint: " << this->minimumSizeHint();
  this->resize(this->minimumSizeHint());
//  this->adjustSize();
  qDebug() << "mainwindow size after resizing: " << this->size();
}

