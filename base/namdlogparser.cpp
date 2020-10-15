#include "namdlogparser.h"

NAMDLog::NAMDLog() {}

void NAMDLog::clearData() {
  mEnergyTitle.clear();
  mEnergyData.clear();
  mPairData.clear();
}

void NAMDLog::readFromStream(QTextStream &ifs, NAMDLogReaderThread *thread,
                             void (NAMDLogReaderThread::*progress)(int x),
                             qint64 fileSize) {
  QString line;
  bool firsttime = true;
  qint64 readSize = 0;
  int previousProgress = 0;
  int currentProgress = 0;
  while (!ifs.atEnd()) {
    ifs.readLineInto(&line);
    if (fileSize > 0) {
      readSize += line.size() + 1;
      currentProgress = std::nearbyint(double(readSize) / fileSize * 100.0);
    }
    if (currentProgress != previousProgress &&
        currentProgress % refreshPeriod == 0 && progress != nullptr) {
      (thread->*progress)(currentProgress);
      previousProgress = currentProgress;
    }
    if (firsttime) {
      if (line.startsWith("ETITLE:")) {
        firsttime = false;
        QStringList titles =
            line.split(QRegExp("(ETITLE:|\\s+)"), Qt::SkipEmptyParts);
        for (auto it = titles.begin(); it != titles.end(); ++it) {
          const auto keyFound = mEnergyData.find(*it);
          if (keyFound == mEnergyData.end()) {
            mEnergyTitle.append(*it);
            mEnergyData[*it] = std::vector<double>();
          }
        }
      }
    }
    if (line.startsWith("ENERGY:")) {
      QStringList fields =
          line.split(QRegExp("[A-Z:\\s]+"), Qt::SkipEmptyParts);
      for (int i = 0; i < mEnergyTitle.size(); ++i) {
        mEnergyData[mEnergyTitle[i]].push_back(fields[i].toDouble());
      }
    }
    if (line.startsWith("PAIR INTERACTION:")) {
      QStringList fields =
          line.split(QRegExp("[A-Z:_\\s]+"), Qt::SkipEmptyParts);
      if (fields.size() == 7) {
        mPairData["VDW_FORCE"].push_back(QVector3D(
            fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble()));
        mPairData["ELECT_FORCE"].push_back(QVector3D(
            fields[4].toDouble(), fields[5].toDouble(), fields[6].toDouble()));
      }
    }
  }
}

std::vector<double> NAMDLog::getStep() const { return getEnergyData("TS"); }

std::vector<double> NAMDLog::getVdW() const { return getEnergyData("VDW"); }

std::vector<double> NAMDLog::getElectrostatic() const {
  return getEnergyData("ELECT");
}

std::vector<double> NAMDLog::getEnergyData(const QString &title, bool *ok) const {
  const auto keyFound = mEnergyData.find(title);
  if (keyFound == mEnergyData.end()) {
    if (ok != nullptr)
      *ok = false;
    return std::vector<double>();
  } else {
    if (ok != nullptr)
      *ok = true;
    return mEnergyData.value(title);
  }
}

std::vector<ForceType> NAMDLog::getVdWForce() const {
  return mPairData.value("VDW_FORCE");
}

std::vector<ForceType> NAMDLog::getElectrostaticForce() const {
  return mPairData.value("ELECT_FORCE");
}

QStringList NAMDLog::getEnergyTitle() const { return mEnergyTitle; }

size_t NAMDLog::size() const
{
  if (mEnergyData.isEmpty()) return 0;
  else return mEnergyData.begin()->size();
}

doBinning::doBinning(HistogramScalar<double> &histogram,
                     const std::vector<int> &column)
    : mHistogram(histogram), mColumn(column) {}

void doBinning::operator()(const std::vector<double> &fields, double energy) {
  // get the position of current point from trajectory
  std::vector<double> pos(mHistogram.dimension());
  for (size_t i = 0; i < pos.size(); ++i) {
    pos[i] = fields[mColumn[i]];
  }
  bool inBoundary = false;
  const size_t addr = mHistogram.address(pos, &inBoundary);
  if (inBoundary) {
    mHistogram[addr] += energy;
  }
}

BinNAMDLogThread::BinNAMDLogThread(QObject *parent) : QThread(parent) {}

BinNAMDLogThread::~BinNAMDLogThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void BinNAMDLogThread::invokeThread(const NAMDLog &log,
                                    const QStringList &title,
                                    const QString &trajectoryFileName,
                                    const std::vector<Axis> &ax,
                                    const std::vector<int> &column) {
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mLog = log;
  mTitle = title;
  mTrajectoryFileName = trajectoryFileName;
  mAxis = ax;
  mColumn = column;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void BinNAMDLogThread::run() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  std::vector<HistogramScalar<double>> histEnergy(mTitle.size(),
                                              HistogramScalar<double>(mAxis));
  std::vector<doBinning> binning;
  for (int i = 0; i < mTitle.size(); ++i) {
    binning.push_back(doBinning(histEnergy[i], mColumn));
  }
  HistogramScalar<double> histCount(mAxis);
  doBinning countBinning(histCount, mColumn);
  // parse the trajectory file
  QFile trajFile(mTrajectoryFileName);
  if (trajFile.open(QIODevice::ReadOnly)) {
    QTextStream ifs_traj(&trajFile);
    QStringList tmpFields;
    size_t lineNumber = 0;
    QString line;
    std::vector<double> fields;
    double readSize = 0;
    int previousProgress = 0;
    bool read_ok = true;
    const size_t fileSize = trajFile.size();
    while (!ifs_traj.atEnd()) {
      fields.clear();
      tmpFields.clear();
      ifs_traj.readLineInto(&line);
      readSize += line.size() + 1;
      const int readingProgress = std::nearbyint(readSize / fileSize * 100);
      if (readingProgress % refreshPeriod == 0 || readingProgress == 100) {
        if (previousProgress != readingProgress) {
          previousProgress = readingProgress;
          qDebug() << Q_FUNC_INFO << "reading " << readingProgress << "%";
          if (readingProgress == 100)
            emit progress("Reading trajectory file", readingProgress);
          else
            emit progress("Reading trajectory file", readingProgress);
        }
      }
      tmpFields = line.split(QRegExp("[(),\\s]+"), Qt::SkipEmptyParts);
      // skip blank lines
      if (tmpFields.size() <= 0)
        continue;
      // skip comment lines start with #
      if (tmpFields[0].startsWith("#"))
        continue;
      for (const auto &i : tmpFields) {
        fields.push_back(i.toDouble(&read_ok));
        if (read_ok == false) {
          emit error("Failed to convert " + i + " to number!");
          break;
        }
      }
      for (int i = 0; i < mTitle.size(); ++i) {
        if (lineNumber < mLog.size()) {
          binning[i](fields, mLog.getEnergyData(mTitle[i])[lineNumber]);
        } else {
          qDebug() << "warning:" << "trajectory may contain more lines than the log file";
        }
      }
      countBinning(fields, 1.0);
      ++lineNumber;
    }
    for (int i = 0; i < mTitle.size(); ++i) {
      for (size_t j = 0; j < histEnergy[i].histogramSize(); ++j) {
        if (histCount[j] > 0) {
          histEnergy[i][j] /= histCount[j];
        }
      }
    }
    emit doneHistogram(histEnergy);
  }
  mutex.unlock();
}

NAMDLogReaderThread::NAMDLogReaderThread(QObject *parent) : QThread(parent) {}

NAMDLogReaderThread::~NAMDLogReaderThread() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void NAMDLogReaderThread::invokeThread(const QString &filename) {
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mLogFileName = filename;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void NAMDLogReaderThread::run() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  NAMDLog logObject;
  QFile logFile(mLogFileName);
  if (logFile.open(QIODevice::ReadOnly)) {
    QTextStream ifs(&logFile);
    logObject.readFromStream(ifs, this, &NAMDLogReaderThread::progress,
                             logFile.size());
    emit done(logObject);
  }
  mutex.unlock();
}
