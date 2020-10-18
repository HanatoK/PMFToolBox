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

#ifndef HISTORYPMFTAB_H
#define HISTORYPMFTAB_H

#include "base/historyfile.h"
// Need to change the directory of this header
#include "reweightingtab/listmodelfilelist.h"

#include <QWidget>

namespace Ui {
class HistoryPMFTab;
}

class HistoryPMFTab : public QWidget
{
  Q_OBJECT

public:
  explicit HistoryPMFTab(QWidget *parent = nullptr);
  void writeRMSDToFile(const std::vector<double> &rmsd, const QString& filename);
  ~HistoryPMFTab();

public slots:
  void loadReferencePMF();
  void saveFile();
  void addHistoryFile();
  void removeHistoryFile();
  void computeRMSD();
  void computeRMSDProgress(int fileRead, int percent);
  void computeRMSDDone(const HistogramPMFHistory& hist);
  void split();
  void splitProgress(int fileRead, int percent);
  void splitDone(const HistogramPMFHistory& hist);

private:
  Ui::HistoryPMFTab *ui;
  ListModelFileList *mListModel;
  HistogramPMF mReferencePMF;
  HistoryReaderThread mReaderThread;
  HistogramPMFHistory mPMFHistory;
  static const int OUTPUT_PRECISION = 7;
  static const int OUTPUT_WIDTH = 14;
};

#endif // HISTORYPMFTAB_H
