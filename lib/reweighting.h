#ifndef REWEIGHTINGTHREAD_H
#define REWEIGHTINGTHREAD_H

#include "lib/histogram.h"

#include <QThread>
#include <QObject>

struct doReweighting {
  doReweighting(const HistogramScalar<double> &from, HistogramProbability &to,
                const QVector<size_t>& from_index, const QVector<size_t>& to_index,
                double kbT)
      : originHistogram(from), targetHistogram(to),
        originPositionIndex(from_index), targetPositionIndex(to_index),
        mKbT(kbT) {}
  void operator()(const QVector<double>& fields);
  const HistogramScalar<double> &originHistogram;
  HistogramProbability &targetHistogram;
  QVector<size_t> originPositionIndex;
  QVector<size_t> targetPositionIndex;
  double mKbT;
};

class ReweightingThread : public QThread
{
  Q_OBJECT
public:
  ReweightingThread(QObject *parent = nullptr);
  void reweighting(QStringList trajectoryFileName, HistogramScalar<double> source, QVector<size_t> from, QVector<size_t> to, double kbT);
signals:
  void error(QString err);
  void done(HistogramProbability targetHistogram);
  void currentLineNumber(int lineNumber);
};

#endif // REWEIGHTINGTHREAD_H
