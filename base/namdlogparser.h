#ifndef NAMDLOGPARSER_H
#define NAMDLOGPARSER_H

#include "base/histogram.h"

#include <QMap>
#include <QMutex>
#include <QTextStream>
#include <QThread>
#include <QVector3D>
#include <QVector>
#include <cstring>

using ForceType = QVector3D;

class NAMDLog {
public:
  NAMDLog();
  void clearData();
  void readFromStream(QTextStream &ifs);
  QVector<double> getStep() const;
  QVector<double> getVdW() const;
  QVector<double> getElectrostatic() const;
  QVector<double> getEnergyData(const QString &title, bool *ok = nullptr) const;
  QVector<ForceType> getVdWForce() const;
  QVector<ForceType> getElectrostaticForce() const;

private:
  QStringList mEnergyTitle;
  QMap<QString, QVector<double>> mEnergyData;
  QMap<QString, QVector<ForceType>> mPairData;
};

struct doBinning {
public:
  doBinning(HistogramScalar<double> &histogram, HistogramScalar<size_t> &count,
            const QVector<int> &column);
  void operator()(const QVector<double> &fields, double energy);
  HistogramScalar<double> &mHistogram;
  HistogramScalar<size_t> &mCount;
  const QVector<int> mColumn;
};

class ParsePairInteractionThread : public QThread {
  Q_OBJECT
public:
  ParsePairInteractionThread(QObject *parent = nullptr);
  ~ParsePairInteractionThread();
  void invokeThread(const QString &logFileName, const QString &title,
                    const QString &trajectoryFileName,
                    const QVector<Axis>& ax,
                    const QVector<int> &column);

signals:
  void error(QString err);
  void done();
  void doneHistogram(HistogramScalar<double> histogram);
  void doneLog(NAMDLog logObject);
  void progress(QString stage, int percent);

protected:
  void run();

private:
  QMutex mutex;
  QString mLogFileName;
  QString mTitle;
  QString mTrajectoryFileName;
  QVector<Axis> mAxis;
  QVector<int> mColumn;
  static const int refreshPeriod = 5;
};

#endif // NAMDLOGPARSER_H
