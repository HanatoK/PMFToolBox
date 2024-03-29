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

#ifndef REWEIGHTINGTHREAD_H
#define REWEIGHTINGTHREAD_H

#include "base/histogram.h"

#include <QObject>
#include <QThread>
#include <QMutex>

struct doReweighting {
  doReweighting(const HistogramScalar<double> &from, HistogramProbability &to,
                const std::vector<int> &from_index,
                const std::vector<int> &to_index, double kbT)
      : originHistogram(from), targetHistogram(to),
        originPositionIndex(from_index), targetPositionIndex(to_index),
        mKbT(kbT), posOrigin(originHistogram.dimension(), 0),
        posTarget(targetHistogram.dimension(), 0) {}
  void operator()(const std::vector<double> &fields);
  void operator()(const QList<QStringView> &fields, bool& read_ok);
  const HistogramScalar<double> &originHistogram;
  HistogramProbability &targetHistogram;
  std::vector<int> originPositionIndex;
  std::vector<int> targetPositionIndex;
  double mKbT;
  // temporary variables
  std::vector<double> posOrigin;
  std::vector<double> posTarget;
};

class ReweightingThread : public QThread {
  Q_OBJECT
public:
  ReweightingThread(QObject *parent = nullptr);
  void reweighting(const QStringList& trajectoryFileName, const QString& outputFileName,
                   const HistogramScalar<double>& source, const std::vector<int>& from,
                   const std::vector<int>& to, const std::vector<Axis>& targetAxis, double kbT, bool usePMF);
  ~ReweightingThread();
signals:
  void error(QString err);
  void doneReturnTarget(HistogramProbability targetHistogram);
  void done();
  void progress(int fileRead, qint64 percent);
protected:
  void run() override;
private:
  // do we need a lock here?
  QMutex mutex;
  QStringList mTrajectoryFileName;
  QString mOutputFileName;
  HistogramScalar<double> mSourceHistogram;
  std::vector<int> mFromColumn;
  std::vector<int> mToColumn;
  std::vector<Axis> mTargetAxis;
  double mKbT;
  bool mUsePMF;
  static const int refreshPeriod = 5;
};

#endif // REWEIGHTINGTHREAD_H
