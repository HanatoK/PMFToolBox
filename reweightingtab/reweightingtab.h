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

#ifndef REWEIGHTINGTAB_H
#define REWEIGHTINGTAB_H

#include "reweightingtab/tablemodelreweightingaxis.h"
#include "reweightingtab/listmodelfilelist.h"
#include "base/reweighting.h"

#include <QWidget>

namespace Ui {
class ReweightingTab;
}

class ReweightingTab : public QWidget
{
  Q_OBJECT

public:
  explicit ReweightingTab(QWidget *parent = nullptr);
  ~ReweightingTab();
  double getKbT() const;

public slots:
  void loadPMF();
  void saveFile();
  void addTrajectory();
  void removeTrajectory();
  void clearTrajectory();
  void readAxisData();
  void reweighting();
  void reweightingProgress(int fileRead, int percent);
  void reweightingError(QString msg);
  void reweightingDone();
  void help();

private:
  Ui::ReweightingTab *ui;
  TableModelReweightingAxis *mTableModel;
  ListModelFileList *mListModel;
  ReweightingThread mWorkerThread;
  HistogramPMF mPMF;
};

class ReweightingCLI: public QObject
{
  Q_OBJECT
public:
  explicit ReweightingCLI(QObject *parent = nullptr);
  bool readReweightJSON(const QString& jsonFilename);
  void startReweighting();
  ~ReweightingCLI();
public slots:
  void reweightingProgress(int fileRead, int percent);
  void reweightingError(QString msg);
  void reweightingDone();
signals:
  void allDone();
private:
  QStringList mFileList;
  QString mOutputFilename;
  HistogramPMF mInputPMF;
  std::vector<int> mFromColumns;
  std::vector<int> mToColumns;
  std::vector<Axis> mTargetAxis;
  double mKbT;
  bool mConvertToPMF;
  ReweightingThread mWorkerThread;
};

//bool readReweightJSON(const QString& jsonFilename);

#endif // REWEIGHTINGTAB_H
