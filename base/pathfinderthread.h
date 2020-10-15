#ifndef PATHFINDERTHREAD_H
#define PATHFINDERTHREAD_H

#include "base/histogram.h"

#include <QThread>
#include <QMutex>

class PMFPathFinderThread : public QThread
{
  Q_OBJECT
public:
  PMFPathFinderThread(QObject *parent = nullptr);
  ~PMFPathFinderThread();
  void findPath(const PMFPathFinder& x);
signals:
  void PathFinderDone(const PMFPathFinder& result);
protected:
  virtual void run() override;
private:
  QMutex mutex;
  PMFPathFinder mPMFPathFinder;
};

#endif // PATHFINDERTHREAD_H
