#include "base/historyfile.h"

HistoryReaderThread::HistoryReaderThread(QObject *parent): QThread(parent)
{
}

HistoryReaderThread::~HistoryReaderThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void HistoryReaderThread::readFromFile(const QStringList &fileNameList)
{
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mHistoryFileName = fileNameList;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void HistoryReaderThread::run()
{
  // TODO
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  HistogramPMFHistory result;
  if (mHistoryFileName.isEmpty()) {
    emit error("No PMF history files");
    return;
  }
  // use the first history file to initialize the axis
  QFile histFile(mHistoryFileName[0]);
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
  for (int i = 0; i < mHistoryFileName.size(); ++i) {
    if (ok) {
      QFile histFile(mHistoryFileName[i]);
      if (histFile.open(QFile::ReadOnly)) {
        stream.setDevice(&histFile);
        ok = readFromStream(stream, result, i, histFile.size());
      }
    }
  }
  emit done(result);
  mutex.unlock();
}

bool HistoryReaderThread::readFromStream(QTextStream &ifs, HistogramPMFHistory &PMFHistory, int fileIndex, int fileSize)
{
  qDebug() << Q_FUNC_INFO;
  QString line;
  std::vector<double> pos(PMFHistory.dimension(), 0);
  std::vector<double> pmfData(PMFHistory.histogramSize(), 0);
  QStringList tmp_fields;
  bool firsttime = true;
  double readSize = 0;
  int previousProgress = 0;
  while (!ifs.atEnd()) {
    line.clear();
    tmp_fields.clear();
    ifs.readLineInto(&line);
    readSize += line.size() + 1;
    const int readingProgress = std::nearbyint(readSize / fileSize * 100);
    if (readingProgress % refreshPeriod == 0 || readingProgress == 100) {
      if (previousProgress != readingProgress) {
        previousProgress = readingProgress;
        qDebug() << Q_FUNC_INFO << "reading " << readingProgress << "%";
        if (readingProgress == 100) fileIndex += 1;
        emit progress(fileIndex, readingProgress);
      }
    }
    tmp_fields = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    if (tmp_fields.size() == 0) continue;
    // header lines
    if (tmp_fields[0].startsWith("#")) {
      if (tmp_fields.size() == 2 && !firsttime) {
        PMFHistory.appendHistogram(pmfData);
        continue;
      } else {
        continue;
      }
    }
    // data lines
    if (tmp_fields.size() == int(PMFHistory.dimension() + 1)) {
      firsttime = false;
      for (size_t i = 0; i < PMFHistory.dimension(); ++i) {
        pos[i] = tmp_fields[i].toDouble();
      }
      const size_t addr = PMFHistory.address(pos);
      pmfData[addr] = tmp_fields[PMFHistory.dimension()].toDouble();
    }
  }
  PMFHistory.appendHistogram(pmfData);
  return true;
}
