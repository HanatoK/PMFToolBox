#include "metadynamics.h"

#include <QElapsedTimer>
#include <QDebug>

Metadynamics::Metadynamics(size_t numThreads):
#ifdef SUM_HILLS_USE_STD_THREAD
  mThreads(numThreads), mCondVars(numThreads), mMutexes(numThreads),
  mTaskStates(numThreads, 0), mFirstTime(true), mShutdown(false)
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  mThreads(numThreads)
#endif
{
//  mThreads.resize(numThreads);
  mNumBlocks = 0;
#ifdef SUM_HILLS_USE_STD_THREAD
  std::cout << "Will use " << numThreads << " thread(s) to sum hills.\n";
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  qDebug() << "Will use" << numThreads << "thread(s) to sum hills.";
#endif
}

Metadynamics::Metadynamics(const std::vector<Axis> &ax, size_t numThreads):
#ifdef SUM_HILLS_USE_STD_THREAD
  mThreads(numThreads), mCondVars(numThreads), mMutexes(numThreads),
  mTaskStates(numThreads, 0), mFirstTime(true), mShutdown(false)
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  mThreads(numThreads)
#endif
{
//  mThreads.resize(numThreads);
  setupHistogram(ax);
#ifdef SUM_HILLS_USE_STD_THREAD
  std::cout << "Will use " << numThreads << " thread(s) to sum hills.\n";
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  qDebug() << "Will use" << numThreads << "thread(s) to sum hills.";
#endif
}

Metadynamics::~Metadynamics()
{
  std::cout << "Calling Metadynamics::~Metadynamics()\n";
#ifdef SUM_HILLS_USE_STD_THREAD
  mShutdown = true;
  for (size_t i = 0; i < mCondVars.size(); ++i) {
    std::unique_lock<std::mutex> lk(mMutexes[i]);
    lk.unlock();
    mTaskStates[i] = 0;
    mCondVars[i].notify_one();
  }
  for (size_t i = 0; i < mThreads.size(); ++i) {
    if (mThreads[i].joinable()) mThreads[i].join();
  }
#endif
}

void Metadynamics::setupHistogram(const std::vector<Axis> &ax) {
  mPMF = HistogramScalar<double>(ax);
  mGradients = HistogramVector<double>(ax, ax.size());
  mNumBlocks = mPMF.histogramSize() / mThreads.size() + 1;
  mPointMap.assign(mPMF.histogramSize(), std::vector<double>(mPMF.dimension(), 0));
  mAddressMap.assign(mPMF.histogramSize(), 0);
  const std::vector<std::vector<double>>& tmpPointTable = mPMF.pointTable();
  for (size_t i = 0; i < mPMF.histogramSize(); ++i) {
    for (size_t j = 0; j < mPMF.dimension(); ++j) {
      mPointMap[i][j] = tmpPointTable[j][i];
    }
    mAddressMap[i] = mPMF.address(mPointMap[i]);
  }
}

void Metadynamics::launchThreads(const Metadynamics::HillRef &h) {
#ifdef SUM_HILLS_USE_STD_THREAD
  for (size_t i = 0; i < mThreads.size(); ++i) {
    mTaskStates[i] = 0;
    if (mFirstTime) mThreads[i] = std::thread(&Metadynamics::projectHillParallelWorker, this, i, std::cref(h));
    if (!mFirstTime) mCondVars[i].notify_one();
  }
  if (mFirstTime) mFirstTime = false;
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  for (int i = 0; i < mThreads.size(); ++i) {
    mThreads[i] = QtConcurrent::run(this, &Metadynamics::projectHillParallelWorker, i, std::cref(h));
  }
#endif
}

void Metadynamics::projectHillParallel() {
#ifdef SUM_HILLS_USE_STD_THREAD
  for (size_t i = 0; i < mThreads.size(); ++i) {
    std::unique_lock<std::mutex> lk(mMutexes[i]);
    mCondVars[i].wait(lk, [this, i](){return mTaskStates[i] == 1;});
  }
#endif
#ifdef SUM_HILLS_USE_QT_CONCURRENT
  for (int i = 0; i < mThreads.size(); ++i) {
    mThreads[i].waitForFinished();
  }
#endif
}

size_t Metadynamics::dimension() const { return mPMF.dimension(); }

const HistogramScalar<double> &Metadynamics::PMF() const { return mPMF; }

const HistogramVector<double> &Metadynamics::gradients() const {
  return mGradients;
}

void Metadynamics::writePMF(const HistogramScalar<double> &PMF,
                            const QString &filename, bool wellTempered,
                            double biasTemperature, double temperature) {
  if (wellTempered) {
    auto tmpPMF = PMF;
    const double factor = (biasTemperature + temperature) / biasTemperature;
    tmpPMF.applyFunction([=](double x) { return factor * x; });
    tmpPMF.writeToFile(filename);
  } else {
    PMF.writeToFile(filename);
  }
}

void Metadynamics::writeGradients(const HistogramVector<double> gradients,
                                  const QString &filename, bool wellTempered,
                                  double biasTemperature, double temperature) {
  if (wellTempered) {
    auto tmpGradients = gradients;
    const double factor = (biasTemperature + temperature) / biasTemperature;
    tmpGradients.applyFunction([=](double x) { return factor * x; });
    tmpGradients.writeToFile(filename);
  } else {
    gradients.writeToFile(filename);
  }
}

void Metadynamics::projectHillParallelWorker(size_t threadIndex,
                                             const HillRef &h) {
#ifdef SUM_HILLS_USE_STD_THREAD
  while (mTaskStates[threadIndex] == 0 && !mShutdown) {
    std::unique_lock<std::mutex> lk(mMutexes[threadIndex]);
#endif
    std::vector<double> gradients(mPMF.dimension(), 0.0);
    double energy = 0.0;
    const size_t stride = mThreads.size();
    for (size_t blockIndex = 0; blockIndex < mNumBlocks; ++blockIndex) {
      const size_t i = blockIndex * stride + threadIndex;
      if (i < mPMF.histogramSize()) {
        h.calcEnergyAndGradient(mPointMap[i], mPMF.axes(), &energy, &gradients);
        const size_t& addr = mAddressMap[i];
        mPMF[addr] += -1.0 * energy;
        // mGradients shares the same axes
        for (size_t j = 0; j < mPMF.dimension(); ++j) {
          mGradients[addr * mPMF.dimension() + j] += -1.0 * gradients[j];
        }
      }
    }
#ifdef SUM_HILLS_USE_STD_THREAD
    mTaskStates[threadIndex] = 1;
    mCondVars[threadIndex].notify_one();
    mCondVars[threadIndex].wait(lk, [this, threadIndex](){return mTaskStates[threadIndex] == 0;});
  }
#endif
}

Metadynamics::HillRef::HillRef(const std::vector<double> &centers,
                               const std::vector<double> &sigma,
                               const double &height)
    : mCentersRef(centers), mSigmasRef(sigma), mHeightRef(height) {}

void Metadynamics::HillRef::calcEnergyAndGradient(
    const std::vector<double> &position, const std::vector<Axis> &axes,
    double *energyPtr, std::vector<double> *gradientsPtr) const {
  if (energyPtr) {
    *energyPtr = 0.0;
  } else {
    return;
  }
  if (gradientsPtr) {
    gradientsPtr->assign(position.size(), 0.0);
  } else {
    return;
  }
  for (size_t i = 0; i < position.size(); ++i) {
    const double dist = axes[i].dist(position[i], mCentersRef[i]);
    const double dist2 = dist * dist;
    const double sigma2 = mSigmasRef[i] * mSigmasRef[i];
    *energyPtr += dist2 / (2.0 * sigma2);
    (*gradientsPtr)[i] = -1.0 * dist / sigma2;
  }
  *energyPtr = mHeightRef * std::exp(-1.0 * (*energyPtr));
  for (size_t i = 0; i < position.size(); ++i) {
    (*gradientsPtr)[i] *= *energyPtr;
  }
}

SumHillsThread::SumHillsThread(QObject *parent) : QThread(parent), mMetaD(nullptr) {}

void SumHillsThread::sumHills(const std::vector<Axis> &ax, const qint64 strides,
                              const QString &HillsTrajectoryFilename) {
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  if (mMetaD != nullptr) delete mMetaD;
  mMetaD = new Metadynamics(ax);
  mHillsTrajectoryFilename = HillsTrajectoryFilename;
  //  mOutputPrefix = outputPrefix;
  mStrides = strides;
  if (!isRunning()) {
    start(LowPriority);
  }
}

SumHillsThread::~SumHillsThread() {
  if (mMetaD != nullptr) delete mMetaD;
}

void SumHillsThread::run() {
  qDebug() << Q_FUNC_INFO;
  mutex.lock();
  QFile trajectoryFile(mHillsTrajectoryFilename);
  const QRegularExpression split_regex("[(),\\s]+");
  qDebug() << "Reading file:" << mHillsTrajectoryFilename;
  QElapsedTimer timer;
  timer.start();
  if (trajectoryFile.open(QFile::ReadOnly)) {
    const double fileSize = trajectoryFile.size();
    QTextStream ifs(&trajectoryFile);
    QString line;
    QVector<QStringRef> tmpFields;
    double readSize = 0;
    qint64 previousProgress = 0;
    bool read_ok = true;
    std::vector<double> center(mMetaD->dimension(), 0.0);
    std::vector<double> sigma(mMetaD->dimension(), 0.0);
    double height = 0.0;
    const Metadynamics::HillRef h(center, sigma, height);
    while (!ifs.atEnd()) {
      // do we need to clear the line?
      ifs.readLineInto(&line);
      readSize += line.size() + 1;
      const int readingProgress = std::nearbyint(readSize / fileSize * 100);
      if (readingProgress % refreshPeriod == 0 || readingProgress == 100) {
        if (previousProgress != readingProgress) {
          previousProgress = readingProgress;
          qDebug() << Q_FUNC_INFO << "reading " << readingProgress << "%";
          emit progress(readingProgress);
        }
      }
      tmpFields = line.splitRef(split_regex, Qt::SkipEmptyParts);
      // skip blank lines
      if (tmpFields.size() <= 0)
        continue;
      // skip comment lines start with #
      if (tmpFields[0].startsWith("#"))
        continue;
      // a metadynamics trajectory has 2N+2 columns, where N is the number of
      // CVs
      read_ok = read_ok && (tmpFields.size() ==
                            static_cast<int>(2 * mMetaD->dimension()) + 2);
      const qint64 numStep = tmpFields[0].toLongLong(&read_ok);
      for (size_t i = 0; i < mMetaD->dimension(); ++i) {
        center[i] = tmpFields[i + 1].toDouble(&read_ok);
        sigma[i] =
            0.5 * tmpFields[mMetaD->dimension() + i + 1].toDouble(&read_ok);
      }
      height = tmpFields[2 * mMetaD->dimension() + 1].toDouble(&read_ok);
      if (!read_ok) {
        emit error(QString("Failed to read line:") + line);
      }
      mMetaD->launchThreads(h);
      mMetaD->projectHillParallel();
      if (numStep > 0 && mStrides > 0 && (numStep % mStrides == 0)) {
        emit stridedResult(numStep, mMetaD->PMF(), mMetaD->gradients());
      }
    }
  } else {
    emit error(QString("Cannot open file") + mHillsTrajectoryFilename);
  }
  qDebug() << "The summation of metadynamics hills takes" << timer.elapsed()
           << "ms.";
  emit done(mMetaD->PMF(), mMetaD->gradients());
  mutex.unlock();
}
