/*
  PMFToolBox: A toolbox to analyze and post-process the output of
  potential of mean force calculations.
  Copyright (C) 2020  Haochuan Chen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "base/graph.h"
#include "base/histogram.h"
#include "base/namdlogparser.h"
#include "mainwindow.h"
#include "test/test.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QObject>
#include <QString>

void runTests() {
  qDebug() << "==============Dijkstra==============";
  testDijkstra();
  qDebug() << "==============SPFA==============";
  testSPFA();
  qDebug() << "==============SPFA2==============";
  testSPFA2();
}

void initTypes() {
  qRegisterMetaType<HistogramPMF>("HistogramPMF");
  qRegisterMetaType<HistogramProbability>("HistogramProbability");
  qRegisterMetaType<HistogramPMFHistory>("HistogramPMFHistory");
  qRegisterMetaType<NAMDLog>("NAMDLog");
  qRegisterMetaType<std::vector<HistogramScalar<double>>>(
      "std::vector<HistogramScalar<double>>");
  qRegisterMetaType<std::vector<HistogramVector<double>>>(
      "std::vector<HistogramVector<double>>");
  qRegisterMetaType<PMFPathFinder>("PMFPathFinder");
}

int runConsole(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);
  // TODO
  QCommandLineParser parser;
  parser.setApplicationDescription(QCoreApplication::translate(
      "main", "A toolbox to analyze and manipulate the output of potential of "
              "mean force (PMF)."));
  parser.addHelpOption();
  const QCommandLineOption projectOption(
      "project",
      QCoreApplication::translate(
          "main", "project a multidimensional PMF to selected axes."));
  const QCommandLineOption reweightOption(
      "reweight",
      QCoreApplication::translate(
          "main", "reweight the colvars trajectories according to a PMF."));
  const QCommandLineOption historyOption(
      "history",
      QCoreApplication::translate("main", "parse the history PMF files."));
  const QCommandLineOption namdlogOption(
      "namdlog",
      QCoreApplication::translate(
          "main",
          "parse and bin a NAMD log file (support pair interactions)."));
  const QCommandLineOption mfepOption(
      "mfep", QCoreApplication::translate(
                  "main", "find the MFEP in a multidimensional PMF."));
  parser.addOption(projectOption);
  parser.addOption(reweightOption);
  parser.addOption(historyOption);
  parser.addOption(namdlogOption);
  parser.addOption(mfepOption);
  parser.addPositionalArgument("jsonfile", "the json configuration file");

  QStringList args;
  for (int i = 0; i < argc; ++i) {
    args << QString::fromLocal8Bit(argv[i]);
  }
  // TODO
  parser.process(a);
  qDebug() << args;
  const QStringList jsonFilenameList = parser.positionalArguments();
  if (jsonFilenameList.empty()) {
    qDebug() << "No json file specified!";
    a.exit(1);
  }
  const QString jsonFile = jsonFilenameList.first();
  if (parser.isSet(projectOption)) {
    if (readProjectPMFJson(jsonFile)) {
      qDebug() << "Operation succeeded.";
      a.quit();
      return 0;
    } else {
      qDebug() << "Error occured!";
      a.exit(1);
      return 1;
    }
  } else if (parser.isSet(reweightOption)) {
    if (readReweightJSON(jsonFile)) {
      qDebug() << "Operation succeeded.";
      a.quit();
      return 0;
    } else {
      qDebug() << "Error occured!";
      a.exit(1);
      return 1;
    }
  }
  return a.exec();
}

int runGui(int argc, char *argv[]) {
  QApplication a(argc, argv);

  MainWindow w;
  w.show();
  return a.exec();
}

int main(int argc, char *argv[]) {

  initTypes();
  //  runTests();

  if (argc > 1) {
    return runConsole(argc, argv);
  } else {
    return runGui(argc, argv);
  }
}
