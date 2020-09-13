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

void ReweightingThread::reweighting(QStringList trajectoryFileName,
                                    HistogramScalar<double> source,
                                    QVector<size_t> from, QVector<size_t> to,
                                    double kbT) {
  qDebug() << Q_FUNC_INFO;
  HistogramProbability result(source.axes());
  doReweighting reweightingObject(source, result, from, to, kbT);
  for (auto it = trajectoryFileName.begin(); it != trajectoryFileName.end(); ++it) {
    qDebug() << "Reading file " << (*it);
    QFile trajectoryFile(*it);
    if (trajectoryFile.open(QFile::ReadOnly)) {
      QTextStream ifs(&trajectoryFile);
      QString line;
      QStringList tmpFields;
      QVector<double> fields;
      size_t lineNumber = 0;
      bool read_ok = true;
      while (!ifs.atEnd()) {
        ifs.readLineInto(&line);
        emit currentLineNumber(lineNumber);
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
        ++lineNumber;
      }
      emit done(result);
    } else {
      emit error("Failed to open file!");
    }
  }
}
