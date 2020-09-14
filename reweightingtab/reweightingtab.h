#ifndef REWEIGHTINGTAB_H
#define REWEIGHTINGTAB_H

#include "reweightingtab/tablemodelreweightingaxis.h"
#include "reweightingtab/listmodeltrajectory.h"
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
  void loadSaveFile();
  void addTrajectory();
  void removeTrajectory();
  void readAxisData();
  void reweighting();
  void reweightingProgress(int fileRead, int percent);
  void reweightingError(QString msg);
  void reweightingDone();
  void help();

private:
  Ui::ReweightingTab *ui;
  TableModelReweightingAxis *mTableModel;
  ListModelTrajectory *mListModel;
  ReweightingThread mWorkerThread;
  HistogramPMF mPMF;
};

#endif // REWEIGHTINGTAB_H
