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
  double readSize = 0;
  int previousProgress = 0;
  int currentProgress = 0;
  const QRegularExpression split_title_regex("(ETITLE:|\\s+)");
  const QRegularExpression split_energy_regex("[A-Z:\\s]+");
  const QRegularExpression split_pair_regex("\\s+");
  while (!ifs.atEnd()) {
    ifs.readLineInto(&line);
    if (fileSize > 0) {
      readSize += line.size() + 1;
      currentProgress = std::nearbyint(readSize / fileSize * 100.0);
    }
    if (currentProgress != previousProgress &&
        currentProgress % refreshPeriod == 0 && progress != nullptr) {
      (thread->*progress)(currentProgress);
      previousProgress = currentProgress;
    }
    if (firsttime) {
      if (line.startsWith("ETITLE:")) {
        firsttime = false;
        QStringView line_view(line);
        const auto titles =
            line_view.split(split_title_regex, Qt::SkipEmptyParts);
        for (auto it = titles.begin(); it != titles.end(); ++it) {
          const auto key = it->toString();
          const auto keyFound = mEnergyData.find(key);
          if (keyFound == mEnergyData.end()) {
            mEnergyTitle.append(key);
            mEnergyData[key] = std::vector<double>();
          }
        }
      }
    }
    if (line.startsWith("ENERGY:")) {
      QStringView line_view(line);
      const auto fields = line_view.split(split_energy_regex, Qt::SkipEmptyParts);
      for (int i = 0; i < mEnergyTitle.size(); ++i) {
        mEnergyData[mEnergyTitle[i]].push_back(fields[i].toDouble());
      }
    }
    if (line.startsWith("PAIR INTERACTION:")) {
      QStringView line_view(line);
      const auto fields = line_view.split(split_pair_regex, Qt::SkipEmptyParts);
      int pos_vdw_force = -1;
      int pos_elect_force = -1;
      for (int i = 0; i < fields.size(); ++i) {
        if (fields[i].compare(QLatin1String("VDW_FORCE:")) == 0) {
          pos_vdw_force = i;
        }
        if (fields[i].compare(QLatin1String("ELECT_FORCE:")) == 0) {
          pos_elect_force = i;
        }
      }
      if (pos_vdw_force > -1) {
        mPairData["VDW_FORCE"].push_back(
            ForceType{fields[pos_vdw_force + 1].toDouble(),
                      fields[pos_vdw_force + 2].toDouble(),
                      fields[pos_vdw_force + 3].toDouble()});
      }
      if (pos_elect_force > -1) {
        mPairData["ELECT_FORCE"].push_back(
            ForceType{fields[pos_elect_force + 1].toDouble(),
                      fields[pos_elect_force + 2].toDouble(),
                      fields[pos_elect_force + 3].toDouble()});
      }
    }
  }
}

std::vector<double> NAMDLog::getStep() const { return getEnergyData("TS"); }

std::vector<double> NAMDLog::getVdW() const { return getEnergyData("VDW"); }

std::vector<double> NAMDLog::getElectrostatic() const {
  return getEnergyData("ELECT");
}

std::vector<double> NAMDLog::getEnergyData(const QString &title,
                                           bool *ok) const {
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

QMap<QString, std::vector<double>>::const_iterator
NAMDLog::getEnergyDataIteratorBegin() const {
  return mEnergyData.constBegin();
}

QMap<QString, std::vector<double>>::const_iterator
NAMDLog::getEnergyDataIteratorEnd() const {
  return mEnergyData.constEnd();
}

QMap<QString, std::vector<double>>::const_iterator
NAMDLog::getEnergyDataIterator(const QString &title) const {
  return mEnergyData.constFind(title);
}

std::vector<ForceType> NAMDLog::getVdWForce() const {
  return mPairData.value("VDW_FORCE");
}

std::vector<ForceType> NAMDLog::getElectrostaticForce() const {
  return mPairData.value("ELECT_FORCE");
}

std::vector<ForceType> NAMDLog::getForceData(const QString &title,
                                             bool *ok) const {
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

QMap<QString, std::vector<ForceType>>::const_iterator
NAMDLog::getForceDataIteratorBegin() const {
  return mPairData.constBegin();
}

QMap<QString, std::vector<ForceType>>::const_iterator
NAMDLog::getForceDataIteratorEnd() const {
  return mPairData.constEnd();
}

QMap<QString, std::vector<ForceType>>::const_iterator
NAMDLog::getForceDataIterator(const QString &title) const {
  return mPairData.constFind(title);
}

QStringList NAMDLog::getEnergyTitle() const { return mEnergyTitle; }

QStringList NAMDLog::getForceTitle() const {
  QStringList title;
  for (auto it = mPairData.begin(); it != mPairData.end(); ++it) {
    title.append(it.key());
  }
  title.removeDuplicates();
  return title;
}

size_t NAMDLog::size() const {
  if (mEnergyData.isEmpty())
    return 0;
  else
    return mEnergyData.begin()->size();
}

doBinningScalar::doBinningScalar(HistogramScalar<double> &histogram,
                                 const std::vector<int> &column)
    : mHistogram(histogram), mColumn(column),
      mPosition(mHistogram.dimension(), 0.0) {}

void doBinningScalar::operator()(const QList<QStringView>& fields,
                                 double energy, bool &read_ok) {
  // get the position of current point from trajectory
  for (size_t i = 0; i < mPosition.size(); ++i) {
    mPosition[i] = fields[mColumn[i]].toDouble(&read_ok);
    if (!read_ok)
      return;
  }
  bool inBoundary = false;
  const size_t addr = mHistogram.address(mPosition, &inBoundary);
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
                                    const QStringList &energyTitle,
                                    const QStringList &forceTitle,
                                    const QString &trajectoryFileName,
                                    const std::vector<Axis> &ax,
                                    const std::vector<int> &column) {
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mLog = log;
  mEnergyTitle = energyTitle;
  mForceTitle = forceTitle;
  mTrajectoryFileName = trajectoryFileName;
  mAxes = ax;
  mColumns = column;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void BinNAMDLogThread::run() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  std::vector<HistogramScalar<double>> histEnergy(
      mEnergyTitle.size(), HistogramScalar<double>(mAxes));
  std::vector<HistogramVector<double>> histForce(
      mForceTitle.size(), HistogramVector<double>(mAxes, 3));
  std::vector<doBinningScalar> energyBinning;
  std::vector<doBinningVector> forceBinning;
  for (int i = 0; i < mEnergyTitle.size(); ++i) {
    energyBinning.push_back(doBinningScalar(histEnergy[i], mColumns));
  }
  for (int i = 0; i < mForceTitle.size(); ++i) {
    forceBinning.push_back(doBinningVector(histForce[i], mColumns));
  }
  HistogramScalar<double> histCount(mAxes);
  doBinningScalar countBinning(histCount, mColumns);
  // parse the trajectory file
  const QRegularExpression split_regex("[(),\\s]+");
  QFile trajFile(mTrajectoryFileName);
  if (trajFile.open(QIODevice::ReadOnly)) {
    QTextStream ifs_traj(&trajFile);
    QList<QStringView> tmpFields;
    size_t lineNumber = 0;
    QString line;
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
          emit progress("Reading trajectory file", readingProgress);
        }
      }
      QStringView line_view(line);
      tmpFields = line_view.split(split_regex, Qt::SkipEmptyParts);
      // skip blank lines
      if (tmpFields.size() <= 0)
        continue;
      // skip comment lines start with #
      if (tmpFields[0].startsWith(QChar('#')))
        continue;
      for (int i = 0; i < mEnergyTitle.size(); ++i) {
        if (lineNumber < mLog.size()) {
          const auto map_iterator = mLog.getEnergyDataIterator(mEnergyTitle[i]);
          if (map_iterator != mLog.getEnergyDataIteratorEnd()) {
            const auto item = map_iterator.value()[lineNumber];
            energyBinning[i](tmpFields, item, read_ok);
            if (!read_ok) {
              emit error(
                  "Failed to read file. Please check the format of the log.");
            }
          }
        } else {
          qDebug() << "warning:"
                   << "trajectory may contain more lines than the log file";
        }
      }
      for (int i = 0; i < mForceTitle.size(); ++i) {
        if (lineNumber < mLog.size()) {
          const auto map_iterator = mLog.getForceDataIterator(mForceTitle[i]);
          if (map_iterator != mLog.getForceDataIteratorEnd()) {
            const auto item = map_iterator.value()[lineNumber];
            forceBinning[i](tmpFields, item, read_ok);
          }
        } else {
          qDebug() << "warning:"
                   << "trajectory may contain more lines than the log file";
        }
      }
      countBinning(tmpFields, 1.0, read_ok);
      ++lineNumber;
    }
    for (int i = 0; i < mEnergyTitle.size(); ++i) {
      for (size_t j = 0; j < histEnergy[i].histogramSize(); ++j) {
        if (histCount[j] > 0) {
          histEnergy[i][j] /= histCount[j];
        }
      }
    }
    for (int i = 0; i < mForceTitle.size(); ++i) {
      for (size_t j = 0; j < histForce[i].histogramSize(); ++j) {
        if (histCount[j] > 0) {
          histForce[i][j] /= histCount[j];
        }
      }
    }
    emit done(histEnergy, histForce);
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

doBinningVector::doBinningVector(HistogramVector<double> &histogram,
                                 const std::vector<int> &column)
    : mHistogram(histogram), mColumn(column),
      mPosition(mHistogram.dimension(), 0.0) {}

void doBinningVector::operator()(const QList<QStringView> &fields,
                                 const std::vector<double> &data,
                                 bool &read_ok) {
  // get the position of current point from trajectory
  for (size_t i = 0; i < mPosition.size(); ++i) {
    mPosition[i] = fields[mColumn[i]].toDouble(&read_ok);
  }
  bool inBoundary = false;
  const size_t addr = mHistogram.address(mPosition, &inBoundary);
  if (inBoundary) {
    for (size_t j = 0; j < mHistogram.multiplicity(); ++j) {
      mHistogram[addr * mHistogram.multiplicity() + j] += data[j];
    }
  }
}
