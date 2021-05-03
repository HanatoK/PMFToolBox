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

#include "base/historyfile.h"

HistoryReaderThread::HistoryReaderThread(QObject *parent) : QThread(parent) {}

HistoryReaderThread::~HistoryReaderThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void HistoryReaderThread::readFromFile(const QStringList &fileNameList) {
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mHistoryFilename = fileNameList;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void HistoryReaderThread::run() {
  // TODO
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  HistogramPMFHistory result;
  if (mHistoryFilename.isEmpty()) {
    emit error("No PMF history files");
    return;
  }
  // use the first history file to initialize the axis
  QFile histFile(mHistoryFilename[0]);
  bool ok = true;
  QTextStream stream;
  if (histFile.open(QFile::ReadOnly)) {
    stream.setDevice(&histFile);
    ok = result.HistogramBase::readFromStream(stream);
    histFile.close();
  } else {
    emit error("Error on opening file " + histFile.fileName());
    return;
  }
  for (int i = 0; i < mHistoryFilename.size(); ++i) {
    if (ok) {
      QFile histFile(mHistoryFilename[i]);
      if (histFile.open(QFile::ReadOnly)) {
        stream.setDevice(&histFile);
        ok = readFromStream(stream, result, i, histFile.size());
      }
    }
  }
  emit done(result);
  mutex.unlock();
}

bool HistoryReaderThread::readFromStream(QTextStream &ifs,
                                         HistogramPMFHistory &PMFHistory,
                                         int fileIndex, int fileSize) {
  qDebug() << Q_FUNC_INFO;
  QString line;
  std::vector<double> pos(PMFHistory.dimension(), 0);
  std::vector<double> pmfData(PMFHistory.histogramSize(), 0);
  QVector<QStringRef> tmpFields;
  bool firsttime = true;
  double readSize = 0;
  int previousProgress = 0;
  const QRegularExpression split_regex("\\s+");
  const int maxFileIndex = mHistoryFilename.size() - 1;
  while (!ifs.atEnd()) {
    ifs.readLineInto(&line);
    readSize += line.size() + 1;
    const int readingProgress = std::nearbyint(readSize / fileSize * 100);
    if (readingProgress % refreshPeriod == 0 || readingProgress == 100) {
      if (previousProgress != readingProgress) {
        previousProgress = readingProgress;
        //        qDebug() << Q_FUNC_INFO << "reading " << readingProgress <<
        //        "%";
        if (readingProgress == 100)
          fileIndex += 1;
        emit progress(qMin(maxFileIndex, fileIndex), readingProgress);
      }
    }
    tmpFields = line.splitRef(split_regex, Qt::SkipEmptyParts);
    if (tmpFields.size() == 0)
      continue;
    // header lines
    if (tmpFields[0].startsWith("#")) {
      if (tmpFields.size() == 2 && !firsttime) {
        PMFHistory.appendHistogram(pmfData);
        continue;
      } else {
        continue;
      }
    }
    // data lines
    if (tmpFields.size() == int(PMFHistory.dimension() + 1)) {
      firsttime = false;
      for (size_t i = 0; i < PMFHistory.dimension(); ++i) {
        pos[i] = tmpFields[i].toDouble();
      }
      const size_t addr = PMFHistory.address(pos);
      pmfData[addr] = tmpFields[PMFHistory.dimension()].toDouble();
    }
  }
  PMFHistory.appendHistogram(pmfData);
  return true;
}
