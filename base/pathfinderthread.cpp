#include "pathfinderthread.h"

PMFPathFinderThread::PMFPathFinderThread(QObject *parent): QThread(parent)
{

}

PMFPathFinderThread::~PMFPathFinderThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void PMFPathFinderThread::findPath(const PMFPathFinder &x)
{
  QMutexLocker locker(&mutex);
  mPMFPathFinder = x;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void PMFPathFinderThread::run() {
  mutex.lock();
  mPMFPathFinder.findPath();
  mutex.unlock();
  emit PathFinderDone(mPMFPathFinder);
}
