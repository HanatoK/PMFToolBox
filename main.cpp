#include "mainwindow.h"
#include "base/histogram.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  qRegisterMetaType<HistogramPMF>("HistogramPMF");
  qRegisterMetaType<HistogramProbability>("HistogramProbability");
  qRegisterMetaType<HistogramPMFHistory>("HistogramPMFHistory");
  MainWindow w;
  w.show();
  return a.exec();
}
