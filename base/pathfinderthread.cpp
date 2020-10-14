#include "pathfinderthread.h"

PathFinderThread::PathFinderThread(QObject *parent): QThread(parent)
{

}

PathFinderThread::~PathFinderThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void PathFinderThread::findPath(const PMFPathFinder &x)
{
  QMutexLocker locker(&mutex);
  mPMFPathFinder = x;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void PathFinderThread::run() {
  mutex.lock();
  mPMFPathFinder.findPath();
  mutex.unlock();
  emit PathFinderDone(mPMFPathFinder);
}
