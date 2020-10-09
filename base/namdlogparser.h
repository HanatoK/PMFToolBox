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

class NAMDLogReaderThread;

class NAMDLog {
public:
  NAMDLog();
  void clearData();
  // I don't want to use QObject since it prohibits the copy constructor
  // but I need to show the progress, so use a callback function here
  void readFromStream(QTextStream &ifs, NAMDLogReaderThread *thread = nullptr,
                      void (NAMDLogReaderThread::*progress)(int) = nullptr,
                      qint64 fileSize = -1);
  QVector<double> getStep() const;
  QVector<double> getVdW() const;
  QVector<double> getElectrostatic() const;
  QVector<double> getEnergyData(const QString &title, bool *ok = nullptr) const;
  QVector<ForceType> getVdWForce() const;
  QVector<ForceType> getElectrostaticForce() const;
  QStringList getEnergyTitle() const;
  size_t size() const;
  friend class NAMDLogReaderThread;

private:
  QStringList mEnergyTitle;
  QMap<QString, QVector<double>> mEnergyData;
  QMap<QString, QVector<ForceType>> mPairData;
  static const int refreshPeriod = 5;
};

class NAMDLogReaderThread : public QThread {
  Q_OBJECT
public:
  NAMDLogReaderThread(QObject *parent = nullptr);
  ~NAMDLogReaderThread();
  void invokeThread(const QString &filename);
signals:
  void done(NAMDLog logObject);
  void progress(int percent);

protected:
  void run() override;

private:
  QMutex mutex;
  QString mLogFileName;
};

struct doBinning {
public:
  doBinning(HistogramScalar<double> &histogram,
            const QVector<int> &column);
  void operator()(const QVector<double> &fields, double energy);
  HistogramScalar<double> &mHistogram;
  const QVector<int> mColumn;
};

class BinNAMDLogThread : public QThread {
  Q_OBJECT
public:
  BinNAMDLogThread(QObject *parent = nullptr);
  ~BinNAMDLogThread();
  void invokeThread(const NAMDLog &log, const QStringList &title,
                    const QString &trajectoryFileName, const QVector<Axis> &ax,
                    const QVector<int> &column);

signals:
  void error(QString err);
  void done();
  void doneHistogram(QVector<HistogramScalar<double>> histogram);
  void progress(QString stage, int percent);

protected:
  void run() override;

private:
  QMutex mutex;
  NAMDLog mLog;
  QStringList mTitle;
  QString mTrajectoryFileName;
  QVector<Axis> mAxis;
  QVector<int> mColumn;
  static const int refreshPeriod = 5;
};

Q_DECLARE_METATYPE(NAMDLog);

#endif // NAMDLOGPARSER_H
