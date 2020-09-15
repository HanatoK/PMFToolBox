#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  mReweightingTab = new ReweightingTab(this);
  mProjectPMFTab = new ProjectPMFTab(this);
  ui->tabWidget->addTab(mReweightingTab, "Reweighting");
  ui->tabWidget->addTab(mProjectPMFTab, "Project PMF");
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateSizes);
}

MainWindow::~MainWindow()
{
  delete ui;
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
  ui->tabWidget->widget(index)->resize(ui->tabWidget->widget(index)->minimumSizeHint());
  ui->tabWidget->widget(index)->adjustSize();
  ui->tabWidget->resize(ui->tabWidget->widget(index)->minimumSizeHint());
  ui->tabWidget->adjustSize();
  resize(ui->tabWidget->widget(index)->minimumSizeHint());
  const auto size = ui->tabWidget->widget(index)->minimumSizeHint();
  qDebug() << size;
  adjustSize();
}

