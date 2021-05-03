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

#ifndef FINDPATHTAB_H
#define FINDPATHTAB_H

#include "base/pathfinderthread.h"
#include "findpathtab/addpatchdialog.h"
#include "findpathtab/patchtablemodel.h"

#include <QWidget>

namespace Ui {
class FindPathTab;
}

class FindPathTab : public QWidget {
  Q_OBJECT

public:
  explicit FindPathTab(QWidget *parent = nullptr);
  ~FindPathTab();
  void setupAvailableAlgorithms();
  Graph::FindPathAlgorithm selectedAlgorithm() const;
  void addPatch(const QString &center, const QVector<double> &length,
                const double value);

public slots:
  void loadPMF();
  void saveFile();
  void findPath();
  void findPathDone(const PMFPathFinder &result);
  void plotPathOnPMF();
  void plotEnergy();
  void showAddPatchDialog();
  void removePatch();

private:
  Ui::FindPathTab *ui;
  PatchTableModel *mPatchTable;
  HistogramPMF mPMF;
  PMFPathFinderThread mPMFPathFinderThread;
  PMFPathFinder mPMFPathFinder;
  QMap<QString, Graph::FindPathAlgorithm> mAvailableAlgorithms;
};

class FindPathCLI: public QObject {
  Q_OBJECT
public:
  explicit FindPathCLI(QObject *parent = nullptr);
  bool readJSON(const QString &jsonFilename);
  void start();
  void saveFile();
  ~FindPathCLI();
public slots:
  void findPathDone(const PMFPathFinder &result);
signals:
  void allDone();
private:
  QString mInputPMF;
  QString mOutputPrefix;
  QString mStart;
  QString mEnd;
  int mAlgorithm;
  std::vector<GridDataPatch> mPatchList;
  HistogramPMF mPMF;
  PMFPathFinderThread mPMFPathFinderThread;
  PMFPathFinder mPMFPathFinder;
};

#endif // FINDPATHTAB_H
