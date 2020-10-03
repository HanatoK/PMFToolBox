#include "namdlogparser.h"

NAMDLog::NAMDLog() {}

void NAMDLog::clearData() {
  mEnergyTitle.clear();
  mEnergyData.clear();
  mPairData.clear();
}

void NAMDLog::readFromStream(QTextStream &ifs, NAMDLogReaderThread *thread, void(NAMDLogReaderThread::*progress)(int x), qint64 fileSize) {
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
    if (currentProgress != previousProgress && currentProgress % refreshPeriod == 0 && progress != nullptr) {
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
            mEnergyData[*it] = QVector<double>();
          }
        }
      }
    }
    if (line.startsWith("ENERGY:")) {
      QStringList fields =
          line.split(QRegExp("[A-Z:\\s]+"), Qt::SkipEmptyParts);
      for (int i = 0; i < mEnergyTitle.size(); ++i) {
        mEnergyData[mEnergyTitle[i]].append(fields[i].toDouble());
      }
    }
    if (line.startsWith("PAIR INTERACTION:")) {
      QStringList fields =
          line.split(QRegExp("[A-Z:_\\s]+"), Qt::SkipEmptyParts);
      if (fields.size() == 7) {
        mPairData["VDW_FORCE"].append(QVector3D(
            fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble()));
        mPairData["ELECT_FORCE"].append(QVector3D(
            fields[4].toDouble(), fields[5].toDouble(), fields[6].toDouble()));
      }
    }
  }
}

QVector<double> NAMDLog::getStep() const { return getEnergyData("TS"); }

QVector<double> NAMDLog::getVdW() const { return getEnergyData("VDW"); }

QVector<double> NAMDLog::getElectrostatic() const {
  return getEnergyData("ELECT");
}

QVector<double> NAMDLog::getEnergyData(const QString &title, bool *ok) const {
  const auto keyFound = mEnergyData.find(title);
  if (keyFound == mEnergyData.end()) {
    if (ok != nullptr)
      *ok = false;
    return QVector<double>();
  } else {
    if (ok != nullptr)
      *ok = true;
    return mEnergyData.value(title);
  }
}

QVector<ForceType> NAMDLog::getVdWForce() const {
  return mPairData.value("VDW_FORCE");
}

QVector<ForceType> NAMDLog::getElectrostaticForce() const {
  return mPairData.value("ELECT_FORCE");
}

QStringList NAMDLog::getEnergyTitle() const
{
  return mEnergyTitle;
}

doBinning::doBinning(HistogramScalar<double> &histogram, const QVector<int> &column)
    : mHistogram(histogram), mColumn(column) {}

void doBinning::operator()(const QVector<double> &fields, double energy) {
  // get the position of current point from trajectory
  QVector<double> pos(mHistogram.dimension());
  for (int i = 0; i < pos.size(); ++i) {
    pos[i] = fields[mColumn[i]];
  }
  bool inBoundary = false;
  const size_t addr = mHistogram.address(pos, &inBoundary);
  if (inBoundary) {
    mHistogram[addr] += energy;
  }
}

BinNAMDLogThread::BinNAMDLogThread(QObject *parent)
  : QThread(parent) {}

BinNAMDLogThread::~BinNAMDLogThread()
{
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void BinNAMDLogThread::invokeThread(const NAMDLog &log,
                                              const QStringList &title,
                                              const QString &trajectoryFileName, const QVector<Axis> &ax,
                                              const QVector<int> &column) {
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
  mutex.lock();
  QVector<HistogramScalar<double>> histEnergy(mTitle.size(), HistogramScalar<double>(mAxis));
  QVector<doBinning> binning;
  for (int i = 0; i < mTitle.size(); ++i) {
    binning.append(doBinning(histEnergy[i], mColumn));
  }
  HistogramScalar<double> histCount(mAxis);
  doBinning countBinning(histCount, mColumn);
  // TODO: rewrite it to support binning multiple energy terms
  // parse the trajectory file
  QFile trajFile(mTrajectoryFileName);
  if (trajFile.open(QIODevice::ReadOnly)) {
    QTextStream ifs_traj(&trajFile);
    QStringList tmpFields;
    size_t lineNumber = 0;
    QString line;
    QVector<double> fields;
    double readSize = 0;
    int previousProgress = 0;
    bool read_ok = true;
    const size_t fileSize = trajFile.size();
    while (!ifs_traj.atEnd()) {
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
      if (tmpFields.size() <= 0) continue;
      // skip comment lines start with #
      if (tmpFields[0].startsWith("#")) continue;
      for (const auto& i : tmpFields) {
        fields.append(i.toDouble(&read_ok));
        if (read_ok == false) {
          emit error("Failed to convert " + i + " to number!");
          break;
        }
      }
      for (int i = 0; i < mTitle.size(); ++i) {
        binning[i](fields, mLog.getEnergyData(mTitle[i])[lineNumber]);
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

NAMDLogReaderThread::NAMDLogReaderThread(QObject *parent): QThread(parent)
{

}

NAMDLogReaderThread::~NAMDLogReaderThread()
{
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void NAMDLogReaderThread::invokeThread(const QString &filename)
{
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mLogFileName = filename;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void NAMDLogReaderThread::run()
{
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  NAMDLog logObject;
  QFile logFile(mLogFileName);
  if (logFile.open(QIODevice::ReadOnly)) {
    QTextStream ifs(&logFile);
    logObject.readFromStream(ifs, this, &NAMDLogReaderThread::progress, logFile.size());
    emit done(logObject);
  }
  mutex.unlock();
}
