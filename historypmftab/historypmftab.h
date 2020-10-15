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
