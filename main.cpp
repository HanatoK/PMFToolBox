#include "mainwindow.h"
#include "base/histogram.h"
#include "base/namdlogparser.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  qRegisterMetaType<HistogramPMF>("HistogramPMF");
  qRegisterMetaType<HistogramProbability>("HistogramProbability");
  qRegisterMetaType<HistogramPMFHistory>("HistogramPMFHistory");
  qRegisterMetaType<NAMDLog>("NAMDLog");
  MainWindow w;
  w.show();
  return a.exec();
}
