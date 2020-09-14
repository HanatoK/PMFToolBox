#ifndef REWEIGHTINGTHREAD_H
#define REWEIGHTINGTHREAD_H

#include "lib/histogram.h"

#include <QObject>
#include <QThread>
#include <QMutex>

struct doReweighting {
  doReweighting(const HistogramScalar<double> &from, HistogramProbability &to,
                const QVector<int> &from_index,
                const QVector<int> &to_index, double kbT)
      : originHistogram(from), targetHistogram(to),
        originPositionIndex(from_index), targetPositionIndex(to_index),
        mKbT(kbT) {}
  void operator()(const QVector<double> &fields);
  const HistogramScalar<double> &originHistogram;
  HistogramProbability &targetHistogram;
  QVector<int> originPositionIndex;
  QVector<int> targetPositionIndex;
  double mKbT;
};

class ReweightingThread : public QThread {
  Q_OBJECT
public:
  ReweightingThread(QObject *parent = nullptr);
  void reweighting(const QStringList& trajectoryFileName, const QString& outputFileName,
                   const HistogramScalar<double>& source, const QVector<int>& from,
                   const QVector<int>& to, const QVector<Axis>& targetAxis, double kbT, bool usePMF);
signals:
  void error(QString err);
  void doneReturnTarget(HistogramProbability targetHistogram);
  void done();
  void progress(int fileRead, int percent);
protected:
  void run() override;
private:
  // do we need a lock here?
  QMutex mutex;
  QStringList mTrajectoryFileName;
  QString mOutputFileName;
  HistogramScalar<double> mSourceHistogram;
  QVector<int> mFromColumn;
  QVector<int> mToColumn;
  QVector<Axis> mTargetAxis;
  double mKbT;
  bool mUsePMF;
  static const int refreshPeriod = 5;
};

#endif // REWEIGHTINGTHREAD_H
