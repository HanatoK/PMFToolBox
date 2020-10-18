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
