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
        mPairData["VDW_FORCE"].push_back(ForceType{
            fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble()});
        mPairData["ELECT_FORCE"].push_back(ForceType{
            fields[4].toDouble(), fields[5].toDouble(), fields[6].toDouble()});
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

std::vector<ForceType> NAMDLog::getForceData(const QString &title, bool *ok) const
{
  const auto keyFound = mPairData.find(title);
  if (keyFound == mPairData.end()) {
    if (ok != nullptr)
      *ok = false;
    return std::vector<ForceType>();
  } else {
    if (ok != nullptr)
      *ok = true;
    return mPairData.value(title);
  }
}

QStringList NAMDLog::getEnergyTitle() const { return mEnergyTitle; }

QStringList NAMDLog::getForceTitle() const
{
  QStringList title;
  for (auto it = mPairData.begin(); it != mPairData.end(); ++it) {
    title.append(it.key());
  }
  title.removeDuplicates();
  return title;
}

size_t NAMDLog::size() const
{
  if (mEnergyData.isEmpty()) return 0;
  else return mEnergyData.begin()->size();
}

doBinningScalar::doBinningScalar(HistogramScalar<double> &histogram,
                     const std::vector<int> &column)
    : mHistogram(histogram), mColumn(column) {}

void doBinningScalar::operator()(const std::vector<double> &fields, double energy) {
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

BinNAMDLogEnergyThread::BinNAMDLogEnergyThread(QObject *parent) : QThread(parent) {}

BinNAMDLogEnergyThread::~BinNAMDLogEnergyThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void BinNAMDLogEnergyThread::invokeThread(const NAMDLog &log,
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

void BinNAMDLogEnergyThread::run() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  std::vector<HistogramScalar<double>> histEnergy(mTitle.size(),
                                              HistogramScalar<double>(mAxis));
  std::vector<doBinningScalar> binning;
  for (int i = 0; i < mTitle.size(); ++i) {
    binning.push_back(doBinningScalar(histEnergy[i], mColumn));
  }
  HistogramScalar<double> histCount(mAxis);
  doBinningScalar countBinning(histCount, mColumn);
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

BinNAMDLogForceThread::BinNAMDLogForceThread(QObject *parent): QThread(parent)
{

}


BinNAMDLogForceThread::~BinNAMDLogForceThread() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void BinNAMDLogForceThread::invokeThread(const NAMDLog &log,
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

void BinNAMDLogForceThread::run() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  std::vector<HistogramVector<double>> histEnergy(mTitle.size(),
                                              HistogramVector<double>(mAxis, 3));
  std::vector<doBinningVector> binning;
  for (int i = 0; i < mTitle.size(); ++i) {
    binning.push_back(doBinningVector(histEnergy[i], mColumn));
  }
  HistogramScalar<double> histCount(mAxis);
  doBinningScalar countBinning(histCount, mColumn);
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
          binning[i](fields, mLog.getForceData(mTitle[i])[lineNumber]);
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

doBinningVector::doBinningVector(HistogramVector<double> &histogram, const std::vector<int> &column)
  : mHistogram(histogram), mColumn(column) {}

void doBinningVector::operator()(const std::vector<double> &fields, const std::vector<double> data)
{
  // get the position of current point from trajectory
  std::vector<double> pos(mHistogram.dimension());
  for (size_t i = 0; i < pos.size(); ++i) {
    pos[i] = fields[mColumn[i]];
  }
  bool inBoundary = false;
  const size_t addr = mHistogram.address(pos, &inBoundary);
  if (inBoundary) {
    for (size_t j = 0; j < mHistogram.multiplicity(); ++j){
      mHistogram[addr + j] += data[j];
    }
  }
}
