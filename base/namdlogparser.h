/*
  PMFToolBox: A toolbox to analyze and post-process the output of
  potential of mean force calculations.
  Copyright (C) 2020  Haochuan Chen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef NAMDLOGPARSER_H
#define NAMDLOGPARSER_H

#include "base/histogram.h"

#include <QMap>
#include <QMutex>
#include <QTextStream>
#include <QThread>
#include <QVector3D>
#include <vector>
#include <cstring>

using ForceType = std::vector<double>;

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
  std::vector<double> getStep() const;
  std::vector<double> getVdW() const;
  std::vector<double> getElectrostatic() const;
  std::vector<double> getEnergyData(const QString &title, bool *ok = nullptr) const;
  std::vector<ForceType> getVdWForce() const;
  std::vector<ForceType> getElectrostaticForce() const;
  std::vector<ForceType> getForceData(const QString &title, bool *ok = nullptr) const;
  QStringList getEnergyTitle() const;
  size_t size() const;
  friend class NAMDLogReaderThread;

private:
  QStringList mEnergyTitle;
  QMap<QString, std::vector<double>> mEnergyData;
  QMap<QString, std::vector<ForceType>> mPairData;
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

struct doBinningScalar {
public:
  doBinningScalar(HistogramScalar<double> &histogram,
            const std::vector<int> &column);
  void operator()(const std::vector<double> &fields, double energy);
  HistogramScalar<double> &mHistogram;
  const std::vector<int> mColumn;
};

class BinNAMDLogEnergyThread : public QThread {
  Q_OBJECT
public:
  BinNAMDLogEnergyThread(QObject *parent = nullptr);
  ~BinNAMDLogEnergyThread();
  void invokeThread(const NAMDLog &log, const QStringList &title,
                    const QString &trajectoryFileName, const std::vector<Axis> &ax,
                    const std::vector<int> &column);

signals:
  void error(QString err);
  void done();
  void doneHistogram(std::vector<HistogramScalar<double>> histogram);
  void progress(QString stage, int percent);

protected:
  void run() override;

private:
  QMutex mutex;
  NAMDLog mLog;
  QStringList mTitle;
  QString mTrajectoryFileName;
  std::vector<Axis> mAxis;
  std::vector<int> mColumn;
  static const int refreshPeriod = 5;
};

struct doBinningVector {
public:
  doBinningVector(HistogramVector<double> &histogram,
            const std::vector<int> &column);
  void operator()(const std::vector<double> &fields, const std::vector<double> data);
  HistogramVector<double> &mHistogram;
  const std::vector<int> mColumn;
};

class BinNAMDLogForceThread: public QThread {
  Q_OBJECT
public:
  BinNAMDLogForceThread(QObject *parent = nullptr);
  ~BinNAMDLogForceThread();
  void invokeThread(const NAMDLog &log, const QStringList &title,
                    const QString &trajectoryFileName, const std::vector<Axis> &ax,
                    const std::vector<int> &column);
signals:
  void error(QString err);
  void done();
  void doneHistogram(std::vector<HistogramVector<double>> histogram);
  void progress(QString stage, int percent);

protected:
  void run() override;

private:
  QMutex mutex;
  NAMDLog mLog;
  QStringList mTitle;
  QString mTrajectoryFileName;
  std::vector<Axis> mAxis;
  std::vector<int> mColumn;
  static const int refreshPeriod = 5;
};

Q_DECLARE_METATYPE(NAMDLog);

#endif // NAMDLOGPARSER_H
