#include "mainwindow.h"
#include "lib/histogram.h"

#include <QApplication>

void testHistogram() {
  HistogramPMF h;
  h.readFromFile("/home/quinolone/HDD/Docs/playground/PMFToolBox_test/result.pmf");
  h.writeToFile("/home/quinolone/HDD/Docs/playground/PMFToolBox_test/new.pmf");
  Axis a1(-180, 180, 5);
  Axis a2(-180, 180, 5);
  QVector v{a1, a2};
  HistogramPMF h2(v);
  h2.writeToFile("/home/quinolone/HDD/Docs/playground/PMFToolBox_test/new2.pmf");
}

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  testHistogram();
  MainWindow w;
  w.show();
  return a.exec();
}
