#include "metadynamics.h"

#include <QElapsedTimer>

Metadynamics::Metadynamics(size_t numThreads) {
  mThreads.resize(numThreads);
  mNumBlocks = 0;
}

Metadynamics::Metadynamics(const std::vector<Axis> &ax, size_t numThreads) {
  mThreads.resize(numThreads);
  setupHistogram(ax);
}

void Metadynamics::setupHistogram(const std::vector<Axis> &ax) {
  mPMF = HistogramScalar<double>(ax);
  mGradients = HistogramVector<double>(ax, ax.size());
  mNumBlocks = mPMF.histogramSize() / mThreads.size() + 1;
}

void Metadynamics::projectHill(const Metadynamics::HillRef &h) {
  const std::vector<std::vector<double>> &pointTable = mPMF.pointTable();
  std::vector<double> pos(mPMF.dimension(), 0.0);
  std::vector<double> gradients(mPMF.dimension(), 0.0);
  double energy = 0.0;
  // very time-consuming, we need to parallelize this!
  for (size_t i = 0; i < mPMF.histogramSize(); ++i) {
    for (size_t j = 0; j < mPMF.dimension(); ++j) {
      pos[j] = pointTable[j][i];
    }
    h.calcEnergy(pos, mPMF.axes(), &energy);
    const size_t addr = mPMF.address(pos);
    mPMF[addr] += -1.0 * energy;
    // mGradients shares the same axes
    h.calcGradient(pos, mPMF.axes(), &gradients);
    for (size_t j = 0; j < mPMF.dimension(); ++j) {
      mGradients[addr + j] += -1.0 * gradients[j];
    }
  }
}

void Metadynamics::launchThreads(const Metadynamics::HillRef &h) {
  for (int i = 0; i < mThreads.size(); ++i) {
    //    mThreads[i] = std::thread(&Metadynamics::projectHillParallelWorker,
    //    this, i, std::cref(h));
    mThreads[i] = QtConcurrent::run(
        this, &Metadynamics::projectHillParallelWorker, i, std::cref(h));
  }
}

void Metadynamics::projectHillParallel() {
  for (int i = 0; i < mThreads.size(); ++i) {
    //    mThreads[i].join();
    mThreads[i].waitForFinished();
  }
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
  const std::vector<std::vector<double>> &pointTable = mPMF.pointTable();
  std::vector<double> pos(mPMF.dimension(), 0.0);
  std::vector<double> gradients(mPMF.dimension(), 0.0);
  double energy = 0.0;
  const size_t stride = mThreads.size();
  for (size_t blockIndex = 0; blockIndex < mNumBlocks; ++blockIndex) {
    const size_t i = blockIndex * stride + threadIndex;
    if (i < mPMF.histogramSize()) {
      for (size_t j = 0; j < mPMF.dimension(); ++j) {
        pos[j] = pointTable[j][i];
      }
      h.calcEnergyAndGradient(pos, mPMF.axes(), &energy, &gradients);
      const size_t addr = mPMF.address(pos);
      mPMF[addr] += -1.0 * energy;
      // mGradients shares the same axes
      for (size_t j = 0; j < mPMF.dimension(); ++j) {
        mGradients[addr + j] += -1.0 * gradients[j];
      }
    }
  }
}

Metadynamics::HillRef::HillRef(const std::vector<double> &centers,
                               const std::vector<double> &sigma,
                               const double &height)
    : mCentersRef(centers), mSigmasRef(sigma), mHeightRef(height) {}

void Metadynamics::HillRef::calcEnergy(const std::vector<double> &position,
                                       const std::vector<Axis> &axes,
                                       double *energyPtr) const {
  if (energyPtr) {
    *energyPtr = 0.0;
  } else {
    return;
  }
  for (size_t i = 0; i < position.size(); ++i) {
    const double dist2 = axes[i].dist2(position[i], mCentersRef[i]);
    const double sigma2 = mSigmasRef[i] * mSigmasRef[i];
    *energyPtr += dist2 / (2.0 * sigma2);
  }
  *energyPtr = mHeightRef * std::exp(-1.0 * (*energyPtr));
}

void Metadynamics::HillRef::calcGradient(
    const std::vector<double> &position, const std::vector<Axis> &axes,
    std::vector<double> *gradientsPtr) const {
  if (gradientsPtr) {
    gradientsPtr->assign(position.size(), 0.0);
  } else {
    return;
  }
  double energy = 0.0;
  calcEnergy(position, axes, &energy);
  for (size_t i = 0; i < position.size(); ++i) {
    const double dist2_grad =
        2.0 * std::sqrt(axes[i].dist2(position[i], mCentersRef[i]));
    const double factor = -1.0 * energy / (2.0 * mSigmasRef[i] * mSigmasRef[i]);
    (*gradientsPtr)[i] = dist2_grad * factor;
  }
}

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
    const double dist2 = axes[i].dist2(position[i], mCentersRef[i]);
    const double sigma2 = mSigmasRef[i] * mSigmasRef[i];
    *energyPtr += dist2 / (2.0 * sigma2);
    (*gradientsPtr)[i] = -1.0 * std::sqrt(dist2) / sigma2;
  }
  *energyPtr = mHeightRef * std::exp(-1.0 * (*energyPtr));
  for (size_t i = 0; i < position.size(); ++i) {
    (*gradientsPtr)[i] *= *energyPtr;
  }
}

SumHillsThread::SumHillsThread(QObject *parent) : QThread(parent) {}

void SumHillsThread::sumHills(const std::vector<Axis> &ax, const qint64 strides,
                              const QString &HillsTrajectoryFilename) {
  qDebug() << Q_FUNC_INFO;
  QMutexLocker locker(&mutex);
  mMetaD.setupHistogram(ax);
  mHillsTrajectoryFilename = HillsTrajectoryFilename;
  //  mOutputPrefix = outputPrefix;
  mStrides = strides;
  if (!isRunning()) {
    start(LowPriority);
  }
}

void SumHillsThread::saveFiles(const QString &pmfFilename,
                               const QString &gradFilename) {
  const auto &pmf = mMetaD.PMF();
  const auto &grad = mMetaD.gradients();
  pmf.writeToFile(pmfFilename);
  grad.writeToFile(gradFilename);
}

SumHillsThread::~SumHillsThread() {}

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
    std::vector<double> center(mMetaD.dimension(), 0.0);
    std::vector<double> sigma(mMetaD.dimension(), 0.0);
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
                            static_cast<int>(2 * mMetaD.dimension()) + 2);
      const qint64 numStep = tmpFields[0].toLongLong(&read_ok);
      for (size_t i = 0; i < mMetaD.dimension(); ++i) {
        center[i] = tmpFields[i + 1].toDouble(&read_ok);
        sigma[i] =
            0.5 * tmpFields[mMetaD.dimension() + i + 1].toDouble(&read_ok);
      }
      height = tmpFields[2 * mMetaD.dimension() + 1].toDouble(&read_ok);
      if (!read_ok) {
        emit error(QString("Failed to read line:") + line);
      }
      mMetaD.launchThreads(h);
      mMetaD.projectHillParallel();
      if (numStep > 0 && mStrides > 0 && (numStep % mStrides == 0)) {
        emit stridedResult(numStep, mMetaD.PMF(), mMetaD.gradients());
      }
    }
  } else {
    emit error(QString("Cannot open file") + mHillsTrajectoryFilename);
  }
  qDebug() << "The summation of metadynamics hills takes" << timer.elapsed()
           << "ms.";
  emit done(mMetaD.PMF(), mMetaD.gradients());
  mutex.unlock();
}
