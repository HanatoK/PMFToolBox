#include "metadynamics.h"

#include <QElapsedTimer>
#include <QDebug>
#include <numeric>

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
    mTaskStates[i] = 0;
    lk.unlock();
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
    std::unique_lock<std::mutex> lk(mMutexes[i]);
    mTaskStates[i] = 0;
    lk.unlock();
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
    const size_t lineBufferSize = h.mActuallBufferedLines;
    for (size_t bufferIndex = 0; bufferIndex < lineBufferSize; ++bufferIndex) {
      for (size_t blockIndex = 0; blockIndex < mNumBlocks; ++blockIndex) {
        const size_t i = blockIndex * stride + threadIndex;
        if (i < mPMF.histogramSize()) {
          h.calcEnergyAndGradient(bufferIndex, mPointMap[i], mPMF.axes(), &energy, &gradients);
          const size_t& addr = mAddressMap[i];
          mPMF[addr] += -1.0 * energy;
          // mGradients shares the same axes
          for (size_t j = 0; j < mPMF.dimension(); ++j) {
            mGradients[addr * mPMF.dimension() + j] += -1.0 * gradients[j];
          }
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

Metadynamics::HillRef::HillRef(const std::vector<std::vector<double>>& centers,
  const std::vector<std::vector<double>>& sigmas,
  const std::vector<double>& heights, const qint64& actualBufferedLines)
    : mCentersRef(centers), mSigmasRef(sigmas), mHeightsRef(heights),
      mActuallBufferedLines(actualBufferedLines) {}

void Metadynamics::HillRef::calcEnergyAndGradient(const qint64 index,
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
    const double dist = axes[i].dist(position[i], mCentersRef[index][i]);
    const double dist2 = dist * dist;
    const double sigma2 = mSigmasRef[index][i] * mSigmasRef[index][i];
    *energyPtr += dist2 / (2.0 * sigma2);
    (*gradientsPtr)[i] = -1.0 * dist / sigma2;
  }
  // magic number: reduce some expensive std::exp calculation
  // TODO: this can be further simplified by using a neighbor list
  if (*energyPtr < 100) {
    *energyPtr = mHeightsRef[index] * std::exp(-1.0 * (*energyPtr));
  } else {
    *energyPtr = 0;
  }
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
    QList<QStringView> tmpFields;
    double readSize = 0;
    qint64 previousProgress = 0;
    bool read_ok = true;
    // use buffered reading
    qint64 lineBufferSize = mLineBufferSize;
    qint64 actualBufferedLine = 0;
    if (mStrides > 0) {
      // currently buffered reading is incompatible with strides
      lineBufferSize = 1;
    }
    std::vector<std::vector<double>> centers(
      lineBufferSize, std::vector<double>(mMetaD->dimension(), 0.0));
    std::vector<std::vector<double>> sigmas(
      lineBufferSize, std::vector<double>(mMetaD->dimension(), 0.0));
    std::vector<double> heights(lineBufferSize, 0.0);
    const Metadynamics::HillRef h(centers, sigmas, heights, actualBufferedLine);
    while (!ifs.atEnd()) {
      qint64 numStep = 0;
      actualBufferedLine = 0;
      for (qint64 bufferIndex = 0; bufferIndex < lineBufferSize; ++bufferIndex) {
        if (!ifs.readLineInto(&line)) {
          // reach EOF, break the loop
          break;
        }
        ++actualBufferedLine;
        readSize += line.size() + 1;
        const int readingProgress = std::nearbyint(readSize / fileSize * 100);
        if (readingProgress % refreshPeriod == 0 || readingProgress == 100) {
          if (previousProgress != readingProgress) {
            previousProgress = readingProgress;
            emit progress(readingProgress);
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
        // a metadynamics trajectory has 2N+2 columns, where N is the number of
        // CVs
        read_ok = read_ok && (tmpFields.size() ==
                              static_cast<int>(2 * mMetaD->dimension()) + 2);
        numStep = tmpFields[0].toLongLong(&read_ok);
        for (size_t i = 0; i < mMetaD->dimension(); ++i) {
          centers[bufferIndex][i] = tmpFields[i + 1].toDouble(&read_ok);
          sigmas[bufferIndex][i] =
            0.5 * tmpFields[mMetaD->dimension() + i + 1].toDouble(&read_ok);
        }
        heights[bufferIndex] = tmpFields[2 * mMetaD->dimension() + 1].toDouble(&read_ok);
        if (!read_ok) {
          emit error(QString("Failed to read line:") + line);
        }
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
