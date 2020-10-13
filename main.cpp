#include "mainwindow.h"
#include "base/histogram.h"
#include "base/namdlogparser.h"
#include "base/graph.h"
#include "test/test.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  qDebug() << "==============Dijkstra==============";
  testDijkstra();
  qDebug() << "==============SPFA==============";
  testSPFA();
  qDebug() << "==============SPFA2==============";
  testSPFA2();
  QApplication a(argc, argv);
  qRegisterMetaType<HistogramPMF>("HistogramPMF");
  qRegisterMetaType<HistogramProbability>("HistogramProbability");
  qRegisterMetaType<HistogramPMFHistory>("HistogramPMFHistory");
  qRegisterMetaType<NAMDLog>("NAMDLog");
  qRegisterMetaType<QVector<HistogramScalar<double>>>("QVector<HistogramScalar<double>>");
  MainWindow w;
  w.show();
  return a.exec();
}
