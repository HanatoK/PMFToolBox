#ifndef METADYNAMICS_H
#define METADYNAMICS_H

#include "base/common.h"
#include "base/helper.h"
#include "base/histogram.h"

#include <tuple>
#include <thread>
#include <vector>
#include <condition_variable>
#include <mutex>
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
  Metadynamics(size_t numThreads = std::thread::hardware_concurrency());
  Metadynamics(const std::vector<Axis>& ax, size_t numThreads = std::thread::hardware_concurrency());
  ~Metadynamics();
  void setupHistogram(const std::vector<Axis>& ax);
  void projectHill(const HillRef& h);
  void launchThreads(const HillRef& h);
  void projectHillParallel();
  size_t dimension() const;
  const HistogramScalar<double>& PMF() const;
  const HistogramVector<double>& gradients() const;
  static void writePMF(const HistogramScalar<double>& PMF, const QString& filename, bool wellTempered, double biasTemperature, double temperature);
  static void writeGradients(const HistogramVector<double> gradients, const QString& filename, bool wellTempered, double biasTemperature, double temperature);
private:
  void projectHillParallelWorker(size_t threadIndex, const HillRef &h);
  std::vector<std::thread> mThreads;
  std::vector<std::condition_variable> mCondVars;
  std::vector<std::mutex> mMutexes;
  std::vector<int> mTaskStates;
  bool mFirstTime;
  bool mShutdown;
  size_t mNumBlocks;
  HistogramScalar<double> mPMF;
  HistogramVector<double> mGradients;
};

class SumHillsThread: public QThread {
  Q_OBJECT
public:
  SumHillsThread(QObject *parent = nullptr);
  void sumHills(const std::vector<Axis>& ax, const qint64 strides,
                const QString& HillsTrajectoryFilename);
  void saveFiles(const QString& pmfFilename, const QString& gradFilename);
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
  Metadynamics mMetaD;
  static const int refreshPeriod = 5;
};

#endif // METADYNAMICS_H
