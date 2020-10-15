#ifndef REWEIGHTINGTHREAD_H
#define REWEIGHTINGTHREAD_H

#include "base/histogram.h"

#include <QObject>
#include <QThread>
#include <QMutex>

struct doReweighting {
  doReweighting(const HistogramScalar<double> &from, HistogramProbability &to,
                const std::vector<int> &from_index,
                const std::vector<int> &to_index, double kbT)
      : originHistogram(from), targetHistogram(to),
        originPositionIndex(from_index), targetPositionIndex(to_index),
        mKbT(kbT) {}
  void operator()(const std::vector<double> &fields);
  const HistogramScalar<double> &originHistogram;
  HistogramProbability &targetHistogram;
  std::vector<int> originPositionIndex;
  std::vector<int> targetPositionIndex;
  double mKbT;
};

class ReweightingThread : public QThread {
  Q_OBJECT
public:
  ReweightingThread(QObject *parent = nullptr);
  void reweighting(const QStringList& trajectoryFileName, const QString& outputFileName,
                   const HistogramScalar<double>& source, const std::vector<int>& from,
                   const std::vector<int>& to, const std::vector<Axis>& targetAxis, double kbT, bool usePMF);
  ~ReweightingThread();
signals:
  void error(QString err);
  void doneReturnTarget(HistogramProbability targetHistogram);
  void done();
  void progress(int fileRead, qint64 percent);
protected:
  void run() override;
private:
  // do we need a lock here?
  QMutex mutex;
  QStringList mTrajectoryFileName;
  QString mOutputFileName;
  HistogramScalar<double> mSourceHistogram;
  std::vector<int> mFromColumn;
  std::vector<int> mToColumn;
  std::vector<Axis> mTargetAxis;
  double mKbT;
  bool mUsePMF;
  static const int refreshPeriod = 5;
};

#endif // REWEIGHTINGTHREAD_H
