#ifndef HISTORYPMFTAB_H
#define HISTORYPMFTAB_H

#include "base/historyfile.h"

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

private:
  Ui::HistoryPMFTab *ui;
  HistogramPMF mReferencePMF;
};

#endif // HISTORYPMFTAB_H
