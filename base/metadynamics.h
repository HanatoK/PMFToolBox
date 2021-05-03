#ifndef METADYNAMICS_H
#define METADYNAMICS_H

#include "base/common.h"
#include "base/helper.h"
#include "base/histogram.h"

#include <tuple>
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
    void calcEnergy(const std::vector<double>& position,
                    const std::vector<Axis>& axes,
                    double* energyPtr = nullptr) const;
    void calcGradients(const std::vector<double>& position,
                       const std::vector<Axis>& axes,
                       std::vector<double>* gradientsPtr = nullptr) const;
  };
  Metadynamics();
  Metadynamics(const std::vector<Axis>& ax);
  void setupHistogram(const std::vector<Axis>& ax);
  void projectHill(const HillRef& h);
  size_t dimension() const;
  const HistogramScalar<double>& PMF() const;
  const HistogramVector<double>& gradients() const;
private:
  HistogramScalar<double> mPMF;
  HistogramVector<double> mGradients;
};

class SumHillsThread: public QThread {
  Q_OBJECT
public:
  SumHillsThread(QObject *parent = nullptr);
  void sumHills(const std::vector<Axis>& ax, const qint64 strides,
                const QString& outputPrefix,
                const QString& HillsTrajectoryFilename);
  void saveFiles(const QString& pmfFilename, const QString& gradFilename);
  ~SumHillsThread();
signals:
  void done(Metadynamics result);
  void progress(qint64 percent);
  void error(QString msg);
protected:
  void run() override;
private:
  QMutex mutex;
  QString mHillsTrajectoryFilename;
  QString mOutputPrefix;
  qint64 mStrides;
  Metadynamics mMetaD;
  static const int refreshPeriod = 5;
};

Q_DECLARE_METATYPE(Metadynamics);

#endif // METADYNAMICS_H
