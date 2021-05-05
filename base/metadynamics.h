#ifndef METADYNAMICS_H
#define METADYNAMICS_H

#include "base/common.h"
#include "base/helper.h"
#include "base/histogram.h"

#define SUM_HILLS_USE_QT_CONCURRENT

#ifdef SUM_HILLS_USE_STD_THREAD
#include <tuple>
#include <thread>
#include <condition_variable>
#include <mutex>
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#endif
#include <vector>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QList>
#include <QString>


class Metadynamics
{
public:
  class HillRef {
  public:
    HillRef(const std::vector<double>& center,
            const std::vector<double>& sigma,
            const double& height);
    const std::vector<double>& mCentersRef;
    const std::vector<double>& mSigmasRef;
    const double& mHeightRef;
    void calcEnergyAndGradient(const std::vector<double>& position,
                               const std::vector<Axis>& axes,
                               double* energyPtr = nullptr,
                               std::vector<double>* gradientsPtr = nullptr) const;
  };
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  Metadynamics(size_t numThreads = QThread::idealThreadCount() - 1);
  Metadynamics(const std::vector<Axis>& ax, size_t numThreads = QThread::idealThreadCount() - 1);
#endif
#ifdef SUM_HILLS_USE_STD_THREAD
  Metadynamics(size_t numThreads = std::thread::hardware_concurrency() - 1);
  Metadynamics(const std::vector<Axis>& ax, size_t numThreads = std::thread::hardware_concurrency() - 1);
#endif
  ~Metadynamics();
  void setupHistogram(const std::vector<Axis>& ax);
  void launchThreads(const HillRef& h);
  void projectHillParallel();
  size_t dimension() const;
  const HistogramScalar<double>& PMF() const;
  const HistogramVector<double>& gradients() const;
  static void writePMF(const HistogramScalar<double>& PMF, const QString& filename, bool wellTempered, double biasTemperature, double temperature);
  static void writeGradients(const HistogramVector<double> gradients, const QString& filename, bool wellTempered, double biasTemperature, double temperature);
private:
  void projectHillParallelWorker(size_t threadIndex, const HillRef &h);
#ifdef SUM_HILLS_USE_STD_THREAD
  std::vector<std::thread> mThreads;
  std::vector<std::condition_variable> mCondVars;
  std::vector<std::mutex> mMutexes;
  std::vector<int> mTaskStates;
  bool mFirstTime;
  bool mShutdown;
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  QVector<QFuture<void>> mThreads;
#endif
  size_t mNumBlocks;
  HistogramScalar<double> mPMF;
  HistogramVector<double> mGradients;
  std::vector<std::vector<double>> mPointMap;
  std::vector<size_t> mAddressMap;
};

class SumHillsThread: public QThread {
  Q_OBJECT
public:
  SumHillsThread(QObject *parent = nullptr);
  void sumHills(const std::vector<Axis>& ax, const qint64 strides,
                const QString& HillsTrajectoryFilename);
  ~SumHillsThread();
signals:
  void done(HistogramScalar<double> PMFresult, HistogramVector<double> GradientsResult);
  void stridedResult(qint64 step, HistogramScalar<double> PMFresult, HistogramVector<double> GradientsResult);
  void progress(qint64 percent);
  void error(QString msg);
protected:
  void run() override;
private:
  QMutex mutex;
  QString mHillsTrajectoryFilename;
//  QString mOutputPrefix;
  qint64 mStrides;
  Metadynamics* mMetaD;
  static const int refreshPeriod = 5;
};

#endif // METADYNAMICS_H
