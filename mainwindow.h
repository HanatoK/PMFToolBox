#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "reweightingtab/reweightingtab.h"
#include "projectpmftab/projectpmftab.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

public slots:
  void updateSizes(int index);

private:
  Ui::MainWindow *ui;
  ReweightingTab *mReweightingTab;
  ProjectPMFTab *mProjectPMFTab;
};
#endif // MAINWINDOW_H
