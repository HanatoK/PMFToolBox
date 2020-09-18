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
  ~HistoryPMFTab();

public slots:
  void loadReferencePMF();
  void saveFile();
  void addHistoryFile();
  void removeHistoryFile();
  void computeRMSD();

private:
  Ui::HistoryPMFTab *ui;
  ListModelFileList *mListModel;
  HistogramPMF mReferencePMF;
};

#endif // HISTORYPMFTAB_H
