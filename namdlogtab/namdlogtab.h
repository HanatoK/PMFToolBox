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

#ifndef NAMDLOGTAB_H
#define NAMDLOGTAB_H

#include "base/namdlogparser.h"
#include "namdlogtab/tablemodelbinning.h"

#include <QWidget>
#include <QDialog>
#include <QCheckBox>
#include <QList>

namespace Ui {
class NAMDLogTab;
}

class selectEnergyTermDialog : public QDialog {
  Q_OBJECT
public:
  selectEnergyTermDialog(const QStringList& energyTitle, const QStringList& forceTitle, QWidget *parent = nullptr);
  QStringList selectedEnergyTitle() const;
  QStringList selectedForceTitle() const;

private:
  QStringList mAvailableEnergyTitle;
  QStringList mAvailableForceTitle;
  QList<QCheckBox*> mEnergyCheckList;
  QList<QCheckBox*> mForceCheckList;
};

class NAMDLogTab : public QWidget
{
  Q_OBJECT

public:
  explicit NAMDLogTab(QWidget *parent = nullptr);
  ~NAMDLogTab();

public slots:
  void loadNAMDLog();
  void loadNAMDLogDone(NAMDLog log);
  void logReadingProgress(int x);
  void openTrajectory();
  void saveFile();
  void runBinning();
  void binningProgress(QString status, int x);
  void addAxis();
  void removeAxis();
  void binningDone(std::vector<HistogramScalar<double> > data);

private:
  Ui::NAMDLogTab *ui;
  TableModelBinning *mTableModel;
  NAMDLogReaderThread mLogReaderThread;
  NAMDLog mLog;
  BinNAMDLogEnergyThread mEnergyBinningThread;
  QStringList mSeletedTitle;
  std::vector<HistogramScalar<double>> mEnergyHistogram;
  std::vector<HistogramVector<double>> mForceHistogram;
};

#endif // NAMDLOGTAB_H
