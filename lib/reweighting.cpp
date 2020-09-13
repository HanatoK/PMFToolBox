#include "lib/reweighting.h"

void doReweighting::operator()(const QVector<double> &fields) {
  QVector<double> pos_origin(originHistogram.dimension());
  QVector<double> pos_target(targetHistogram.dimension());
  for (int i = 0; i < pos_origin.size(); ++i) {
    pos_origin[i] = fields[originPositionIndex[i]];
  }
  for (int j = 0; j < pos_target.size(); ++j) {
    pos_target[j] = fields[targetPositionIndex[j]];
  }
  bool in_origin_grid = true;
  bool in_target_grid = true;
  const size_t addr_origin =
      originHistogram.address(pos_origin, &in_origin_grid);
  const size_t addr_target =
      originHistogram.address(pos_target, &in_target_grid);
  if (in_origin_grid && in_target_grid) {
    const double weight = -1.0 * originHistogram[addr_origin] / mKbT;
    targetHistogram[addr_target] += 1.0 * std::exp(weight);
  }
}

ReweightingThread::ReweightingThread(QObject *parent) : QThread(parent) {}

void ReweightingThread::reweighting(const QStringList &trajectoryFileName, const QString &outputFileName,
                                    const HistogramScalar<double> &source,
                                    const QVector<int> &from, const QVector<int> &to,
                                    double kbT) {
  qDebug() << Q_FUNC_INFO;
  mTrajectoryFileName = trajectoryFileName;
  mOutputFileName = outputFileName;
  mSourceHistogram = source;
  mFromColumn = from;
  mToColumn = to;
  mKbT = kbT;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void ReweightingThread::run()
{
  HistogramProbability result(mSourceHistogram.axes());
  doReweighting reweightingObject(mSourceHistogram, result, mFromColumn, mToColumn, mKbT);
  size_t numFile = 0;
  for (auto it = mTrajectoryFileName.begin(); it != mTrajectoryFileName.end(); ++it) {
    qDebug() << "Reading file " << (*it);
    QFile trajectoryFile(*it);
    const auto fileSize = it->size();
    if (trajectoryFile.open(QFile::ReadOnly)) {
      ++numFile;
      QTextStream ifs(&trajectoryFile);
      QString line;
      QStringList tmpFields;
      QVector<double> fields;
      double readSize = 0;
      bool read_ok = true;
      while (!ifs.atEnd()) {
        ifs.readLineInto(&line);
        readSize += line.size();
        const double readingProgress = readSize / fileSize * 100;
        emit progress(numFile, readingProgress);
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
        reweightingObject(fields);
      }
    } else {
      emit error("Failed to open file " + (*it));
    }
  }
  result.writeToFile(mOutputFileName);
  emit done(result);
}
