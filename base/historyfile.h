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

#ifndef HISOTRYFILE_H
#define HISOTRYFILE_H

#include "base/histogram.h"

#include <QObject>
#include <QThread>
#include <QMutex>

class HistoryReaderThread : public QThread {
  Q_OBJECT
public:
  HistoryReaderThread(QObject *parent = nullptr);
  ~HistoryReaderThread();
  void readFromFile(const QStringList& fileNameList);
signals:
  void done(HistogramPMFHistory PMFHistory);
  void error(QString err);
  void progress(int fileRead, int percent);
protected:
  void run() override;
private:
  bool readFromStream(QTextStream& ifs,
                      HistogramPMFHistory& PMFHistory,
                      int fileIndex, int fileSize);
  QMutex mutex;
  QStringList mHistoryFilename;
  static const int refreshPeriod = 5;
};

#endif // HISOTRYFILE_H
