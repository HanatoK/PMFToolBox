#ifndef FINDPATHTAB_H
#define FINDPATHTAB_H

#include "base/pathfinderthread.h"

#include <QWidget>

namespace Ui {
class FindPathTab;
}

class FindPathTab : public QWidget
{
  Q_OBJECT

public:
  explicit FindPathTab(QWidget *parent = nullptr);
  ~FindPathTab();
  void setupAvailableAlgorithms();
  Graph::FindPathAlgorithm selectedAlgorithm() const;

public slots:
  void loadPMF();
  void saveFile();
  void findPath();
  void findPathDone(const PMFPathFinder &result);

private:
  Ui::FindPathTab *ui;
  HistogramPMF mPMF;
  PMFPathFinderThread mPMFPathFinderThread;
  PMFPathFinder mPMFPathFinder;
  QMap<QString, Graph::FindPathAlgorithm> mAvailableAlgorithms;
};

#endif // FINDPATHTAB_H
