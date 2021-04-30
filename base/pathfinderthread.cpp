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

#include "pathfinderthread.h"

PMFPathFinderThread::PMFPathFinderThread(QObject *parent) : QThread(parent) {}

PMFPathFinderThread::~PMFPathFinderThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void PMFPathFinderThread::findPath(const PMFPathFinder &x) {
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
