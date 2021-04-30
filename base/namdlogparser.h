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
#include <cstring>
#include <vector>

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
  std::vector<double> getEnergyData(const QString &title,
                                    bool *ok = nullptr) const;
  QMap<QString, std::vector<double>>::const_iterator
  getEnergyDataIteratorBegin() const;
  QMap<QString, std::vector<double>>::const_iterator
  getEnergyDataIteratorEnd() const;
  QMap<QString, std::vector<double>>::const_iterator
  getEnergyDataIterator(const QString &title) const;
  std::vector<ForceType> getVdWForce() const;
  std::vector<ForceType> getElectrostaticForce() const;
  std::vector<ForceType> getForceData(const QString &title,
                                      bool *ok = nullptr) const;
  QMap<QString, std::vector<ForceType>>::const_iterator
  getForceDataIteratorBegin() const;
  QMap<QString, std::vector<ForceType>>::const_iterator
  getForceDataIteratorEnd() const;
  QMap<QString, std::vector<ForceType>>::const_iterator
  getForceDataIterator(const QString &title) const;
  QStringList getEnergyTitle() const;
  QStringList getForceTitle() const;
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
  void operator()(const QVector<QStringRef> &fields, double energy,
                  bool &read_ok);
  HistogramScalar<double> &mHistogram;
  const std::vector<int> mColumn;
  std::vector<double> mPosition;
};

class BinNAMDLogThread : public QThread {
  Q_OBJECT
public:
  BinNAMDLogThread(QObject *parent = nullptr);
  ~BinNAMDLogThread();
  void invokeThread(const NAMDLog &log, const QStringList &energyTitle,
                    const QStringList &forceTitle,
                    const QString &trajectoryFileName,
                    const std::vector<Axis> &ax,
                    const std::vector<int> &column);

signals:
  void error(QString err);
  void done(std::vector<HistogramScalar<double>> histogramEnergy,
            std::vector<HistogramVector<double>> histogramForce);
  void progress(QString stage, int percent);

protected:
  void run() override;

private:
  QMutex mutex;
  NAMDLog mLog;
  QStringList mEnergyTitle;
  QStringList mForceTitle;
  QString mTrajectoryFileName;
  std::vector<Axis> mAxes;
  std::vector<int> mColumns;
  static const int refreshPeriod = 5;
};

struct doBinningVector {
public:
  doBinningVector(HistogramVector<double> &histogram,
                  const std::vector<int> &column);
  void operator()(const QVector<QStringRef> &fields,
                  const std::vector<double> &data, bool &read_ok);
  HistogramVector<double> &mHistogram;
  const std::vector<int> mColumn;
  std::vector<double> mPosition;
};

Q_DECLARE_METATYPE(NAMDLog);

#endif // NAMDLOGPARSER_H
