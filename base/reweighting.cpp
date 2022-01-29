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

#include "base/reweighting.h"

void doReweighting::operator()(const std::vector<double> &fields) {
  for (size_t i = 0; i < posOrigin.size(); ++i) {
    posOrigin[i] = fields[originPositionIndex[i]];
  }
  for (size_t j = 0; j < posTarget.size(); ++j) {
    posTarget[j] = fields[targetPositionIndex[j]];
  }
  bool in_origin_grid = true;
  bool in_target_grid = true;
  const size_t addr_origin =
      originHistogram.address(posOrigin, &in_origin_grid);
  const size_t addr_target =
      targetHistogram.address(posTarget, &in_target_grid);
  if (in_origin_grid && in_target_grid) {
    const double weight = -1.0 * originHistogram[addr_origin] / mKbT;
    targetHistogram[addr_target] += 1.0 * std::exp(weight);
  }
}

void doReweighting::operator()(const QList<QStringView> &fields,
                               bool &read_ok) {
  for (size_t i = 0; i < posOrigin.size(); ++i) {
    posOrigin[i] = fields[originPositionIndex[i]].toDouble(&read_ok);
    if (read_ok == false)
      return;
  }
  for (size_t j = 0; j < posTarget.size(); ++j) {
    posTarget[j] = fields[targetPositionIndex[j]].toDouble(&read_ok);
    if (read_ok == false)
      return;
  }
  bool in_origin_grid = true;
  bool in_target_grid = true;
  const size_t addr_origin =
      originHistogram.address(posOrigin, &in_origin_grid);
  const size_t addr_target =
      targetHistogram.address(posTarget, &in_target_grid);
  if (in_origin_grid && in_target_grid) {
    const double weight = -1.0 * originHistogram[addr_origin] / mKbT;
    targetHistogram[addr_target] += 1.0 * std::exp(weight);
  }
}

ReweightingThread::ReweightingThread(QObject *parent) : QThread(parent) {}

void ReweightingThread::reweighting(const QStringList &trajectoryFileName,
                                    const QString &outputFileName,
                                    const HistogramScalar<double> &source,
                                    const std::vector<int> &from,
                                    const std::vector<int> &to,
                                    const std::vector<Axis> &targetAxis,
                                    double kbT, bool usePMF) {
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mTrajectoryFileName = trajectoryFileName;
  mOutputFileName = outputFileName;
  mSourceHistogram = source;
  mFromColumn = from;
  mToColumn = to;
  mTargetAxis = targetAxis;
  mKbT = kbT;
  mUsePMF = usePMF;
  if (!isRunning()) {
    start(LowPriority);
  }
}

ReweightingThread::~ReweightingThread() {
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void ReweightingThread::run() {
  qDebug() << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": from columns " << mFromColumn;
  qDebug() << Q_FUNC_INFO << ": to columns " << mToColumn;
  qDebug() << Q_FUNC_INFO << ": using kbt = " << mKbT;
  qDebug() << Q_FUNC_INFO << ": target axis " << mTargetAxis;
  mutex.lock();
  HistogramProbability result(mTargetAxis);
  doReweighting reweightingObject(mSourceHistogram, result, mFromColumn,
                                  mToColumn, mKbT);
  int numFile = 0;
  const QRegularExpression split_regex("[(),\\s]+");
  for (auto it = mTrajectoryFileName.begin(); it != mTrajectoryFileName.end();
       ++it) {
    qDebug() << "Reading file " << (*it);
    QFile trajectoryFile(*it);
    if (trajectoryFile.open(QFile::ReadOnly)) {
      const double fileSize = trajectoryFile.size();
      QTextStream ifs(&trajectoryFile);
      QString line;
      QList<QStringView> tmpFields;
      double readSize = 0;
      qint64 previousProgress = 0;
      bool read_ok = true;
      while (!ifs.atEnd()) {
        ifs.readLineInto(&line);
        readSize += line.size() + 1;
        const int readingProgress = std::nearbyint(readSize / fileSize * 100);
        if (readingProgress % refreshPeriod == 0 || readingProgress == 100) {
          if (previousProgress != readingProgress) {
            previousProgress = readingProgress;
            qDebug() << Q_FUNC_INFO << "reading " << readingProgress << "%";
            emit progress(numFile, readingProgress);
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
        reweightingObject(tmpFields, read_ok);
        if (read_ok == false) {
          emit error("Failed to convert to number!");
          break;
        }
      }
      ++numFile;
    } else {
      emit error("Failed to open file " + (*it));
    }
  }
  if (mUsePMF) {
    result.convertToFreeEnergy(mKbT);
  }
  result.writeToFile(mOutputFileName);
  emit done();
  emit doneReturnTarget(result);
  mutex.unlock();
}
