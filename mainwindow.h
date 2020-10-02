#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "reweightingtab/reweightingtab.h"
#include "projectpmftab/projectpmftab.h"
#include "historypmftab/historypmftab.h"
#include "namdlogtab/namdlogtab.h"

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
  void resizeEvent(QResizeEvent* event) override;
//  bool event(QEvent* event) override;

public slots:
  void updateSizes(int index);

private:
  Ui::MainWindow *ui;
  ReweightingTab *mReweightingTab;
  ProjectPMFTab *mProjectPMFTab;
  HistoryPMFTab *mHistoryPMFTab;
  NAMDLogTab *mNAMDLogTab;
};
#endif // MAINWINDOW_H
