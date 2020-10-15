#include "base/reweighting.h"

void doReweighting::operator()(const std::vector<double> &fields) {
  std::vector<double> pos_origin(originHistogram.dimension());
  std::vector<double> pos_target(targetHistogram.dimension());
  for (size_t i = 0; i < pos_origin.size(); ++i) {
    pos_origin[i] = fields[originPositionIndex[i]];
  }
  for (size_t j = 0; j < pos_target.size(); ++j) {
    pos_target[j] = fields[targetPositionIndex[j]];
  }
  bool in_origin_grid = true;
  bool in_target_grid = true;
  const size_t addr_origin =
      originHistogram.address(pos_origin, &in_origin_grid);
  const size_t addr_target =
      targetHistogram.address(pos_target, &in_target_grid);
  if (in_origin_grid && in_target_grid) {
    const double weight = -1.0 * originHistogram[addr_origin] / mKbT;
    targetHistogram[addr_target] += 1.0 * std::exp(weight);
  }
}

ReweightingThread::ReweightingThread(QObject *parent) : QThread(parent) {}

void ReweightingThread::reweighting(const QStringList &trajectoryFileName, const QString &outputFileName,
                                    const HistogramScalar<double> &source,
                                    const std::vector<int> &from, const std::vector<int> &to,
                                    const std::vector<Axis> &targetAxis, double kbT, bool usePMF) {
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

ReweightingThread::~ReweightingThread()
{
  // am I doing the right things?
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  mutex.unlock();
  wait();
  quit();
}

void ReweightingThread::run()
{
  qDebug() << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": from columns " << mFromColumn;
  qDebug() << Q_FUNC_INFO << ": to columns " << mToColumn;
  qDebug() << Q_FUNC_INFO << ": using kbt = " << mKbT;
  qDebug() << Q_FUNC_INFO << ": target axis " << mTargetAxis;
  mutex.lock();
  HistogramProbability result(mTargetAxis);
  doReweighting reweightingObject(mSourceHistogram, result, mFromColumn, mToColumn, mKbT);
  size_t numFile = 0;
  for (auto it = mTrajectoryFileName.begin(); it != mTrajectoryFileName.end(); ++it) {
    qDebug() << "Reading file " << (*it);
    QFile trajectoryFile(*it);
    const double fileSize = trajectoryFile.size();
    if (trajectoryFile.open(QFile::ReadOnly)) {
      QTextStream ifs(&trajectoryFile);
      QString line;
      QStringList tmpFields;
      std::vector<double> fields;
      double readSize = 0;
      int previousProgress = 0;
      bool read_ok = true;
      while (!ifs.atEnd()) {
        fields.clear();
        line.clear();
        ifs.readLineInto(&line);
        readSize += line.size() + 1;
        const int readingProgress = std::nearbyint(readSize / fileSize * 100);
        if (readingProgress % refreshPeriod == 0 || readingProgress == 100) {
          if (previousProgress != readingProgress) {
            previousProgress = readingProgress;
            qDebug() << Q_FUNC_INFO << "reading " << readingProgress << "%";
            if (readingProgress == 100)
              emit progress(numFile+1, readingProgress);
            else
              emit progress(numFile, readingProgress);
          }
        }
        tmpFields = line.split(QRegExp("[(),\\s]+"), Qt::SkipEmptyParts);
        // skip blank lines
        if (tmpFields.size() <= 0) continue;
        // skip comment lines start with #
        if (tmpFields[0].startsWith("#")) continue;
        for (const auto& i : tmpFields) {
          fields.push_back(i.toDouble(&read_ok));
          if (read_ok == false) {
            emit error("Failed to convert " + i + " to number!");
            break;
          }
        }
        reweightingObject(fields);
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
