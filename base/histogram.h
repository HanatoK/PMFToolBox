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

#ifndef HISTOGRAMBASE_H
#define HISTOGRAMBASE_H

#include "base/graph.h"
#include "base/helper.h"
#include "base/common.h"

#include <QDebug>
#include <QFile>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QThread>

#include <cctype>
#include <functional>
#include <utility>
#include <vector>

using std::size_t;

class HistogramBase;

class Axis {
public:
  Axis();
  Axis(double lowerBound, double upperBound, size_t bins,
       bool periodic = false);
  void setPeriodicity(bool periodic, double periodicLower,
                      double periodicUpper);
  double width() const;
  size_t bin() const;
  bool inBoundary(double x) const;
  size_t index(double x, bool *inBoundary = nullptr) const;
  double wrap(double x) const;
  QString infoHeader() const;
  std::vector<double> getMiddlePoints() const;
  std::vector<double> getBoundaryPoints() const;
  double lowerBound() const;
  double upperBound() const;
  double setLowerBound(double newLowerBound);
  double setUpperBound(double newUpperBound);
  double setWidth(double new_width);
  double dist(double x, double reference) const;
  bool realPeriodic() const;
  bool periodic() const;
  double period() const;
  friend class HistogramBase;

private:
  double mLowerBound;
  double mUpperBound;
  size_t mBins;
  double mWidth;
  bool mPeriodic;
  double mPeriodicLowerBound;
  double mPeriodicUpperBound;
};

struct GridDataPatch {
  std::vector<double> mCenter;
  std::vector<double> mLength;
  double mValue;
};

struct AxisView {
  AxisView();
  int mColumn;
  Axis mAxis;
  bool mInPMF;
  bool mReweightingTo;
};

QDebug operator<<(QDebug dbg, const Axis &ax);

class HistogramBase {
public:
  HistogramBase();
  virtual ~HistogramBase();
  explicit HistogramBase(const std::vector<Axis> &ax);
  virtual bool readFromStream(QTextStream &ifs);
  virtual bool writeToStream(QTextStream &ofs) const;
  bool isInGrid(const std::vector<double> &position) const;
  virtual std::vector<size_t> index(const std::vector<double> &position,
                                    bool *inBoundary = nullptr) const;
  virtual size_t address(const std::vector<double> &position,
                         bool *inBoundary = nullptr) const;
  virtual size_t address(const std::vector<size_t> &idx) const;
  std::vector<double> reverseAddress(size_t address,
                                     bool *inBoundary = nullptr) const;
  virtual std::pair<size_t, bool> neighbor(const std::vector<double> &position,
                                           size_t axisIndex,
                                           bool previous = false) const;
  virtual std::pair<size_t, bool>
  neighborByAddress(size_t address, size_t axisIndex,
                    bool previous = false) const;
  virtual std::vector<std::pair<size_t, bool>>
  allNeighbor(const std::vector<double> &position) const;
  virtual std::vector<std::pair<size_t, bool>>
  allNeighborByAddress(size_t address) const;
  virtual std::pair<size_t, bool>
  neighborByIndex(std::vector<size_t> indexes,
                  size_t axisIndex, bool previous = false) const;
  size_t histogramSize() const;
  size_t dimension() const;
  const std::vector<Axis> &axes() const;
  const std::vector<std::vector<double>>& pointTable() const;

protected:
  size_t mNdim;
  size_t mHistogramSize;
  std::vector<Axis> mAxes;
  std::vector<std::vector<double>> mPointTable;
  std::vector<size_t> mAccu;

private:
  void fillTable();
};

// 1D histogram
template <typename T> class HistogramScalar : public virtual HistogramBase {
public:
  static_assert(std::is_arithmetic<T>::value,
                "HistogramScalar requires a scalar type!");
  HistogramScalar();
  explicit HistogramScalar(const std::vector<Axis> &ax);
  virtual ~HistogramScalar();
  virtual bool readFromStream(QTextStream &ifs) override;
  virtual bool readFromFile(const QString &filename);
  virtual bool writeToStream(QTextStream &ofs) const override;
  virtual bool writeToFile(const QString &filename) const;
  virtual T operator()(const std::vector<double> &position);
  virtual const T operator()(const std::vector<double> &position) const;
  virtual T &operator[](size_t addr);
  virtual const T &operator[](size_t addr) const;
  virtual void applyFunction(std::function<T(T)> f);
  T sum() const;
  T minimum() const;
  const std::vector<T> &data() const;
  std::vector<T> &data();
  virtual std::vector<T> getDerivative(const std::vector<double> &pos,
                                       bool *inBoundary = nullptr) const;
  virtual void generate(std::function<T(const std::vector<double> &)> &func);
  virtual bool set(const std::vector<double> &pos, const T &value);
  virtual void merge(const HistogramScalar<T> &source);

protected:
  std::vector<T> mData;
};

template <typename T> HistogramScalar<T>::HistogramScalar() : mData(0) {
  qDebug() << "Calling" << Q_FUNC_INFO;
}

template <typename T>
HistogramScalar<T>::HistogramScalar(const std::vector<Axis> &ax)
    : HistogramBase(ax) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  mData.resize(mHistogramSize);
}

template <typename T> HistogramScalar<T>::~HistogramScalar() {
  qDebug() << "Calling" << Q_FUNC_INFO;
}

template <typename T>
bool HistogramScalar<T>::readFromStream(QTextStream &ifs) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  bool file_opened = HistogramBase::readFromStream(ifs);
  if (!file_opened)
    return file_opened;
  // read data into m_data
  QString line;
  std::vector<double> pos(mNdim, 0);
  QList<QStringView> tmpFields;
  mData.resize(mHistogramSize);
  size_t dataLines = 0;
  const QRegularExpression split_regex("\\s+");
  while (!ifs.atEnd()) {
    ifs.readLineInto(&line);
    QStringView line_view(line);
    tmpFields = line_view.split(split_regex, Qt::SkipEmptyParts);
    // skip blank lines
    if (tmpFields.size() == int(mNdim) + 1) {
      // skip unnecessary comment lines starting with #
      if (!tmpFields[0].startsWith(QChar('#'))) {
        bool ok = true;
        for (size_t i = 0; i < mNdim; ++i) {
          pos[i] = tmpFields[i].toDouble(&ok);
          if (!ok)
            return false;
        }
        // find the position
        const size_t addr = address(pos);
        if (ok) {
          mData[addr] = stringToNumber<T>(tmpFields[mNdim], &ok);
          if (!ok)
            return false;
          else
            ++dataLines;
        }
      }
    } else if (tmpFields.size() == 0) {
      continue;
    } else {
      return false;
    }
  }
  qDebug() << Q_FUNC_INFO << ": expect " << mHistogramSize << " lines, read "
           << dataLines << "lines";
  return true;
}

template <typename T>
bool HistogramScalar<T>::readFromFile(const QString &filename) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": opening " << filename;
  QFile inputFile(filename);
  if (inputFile.open(QFile::ReadOnly)) {
    QTextStream stream(&inputFile);
    return readFromStream(stream);
  } else {
    qWarning() << "Failed to open file:" << filename;
    return false;
  }
}

template <typename T>
bool HistogramScalar<T>::writeToStream(QTextStream &ofs) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  bool file_opened = HistogramBase::writeToStream(ofs);
  if (!file_opened)
    return file_opened;
  std::vector<double> pos(mNdim, 0);
  ofs.setRealNumberNotation(QTextStream::ScientificNotation);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
      ofs << qSetFieldWidth(OUTPUT_WIDTH);
      ofs.setRealNumberPrecision(OUTPUT_POSITION_PRECISION);
      ofs << pos[j];
      ofs << qSetFieldWidth(0) << ' ';
    }
    ofs.setRealNumberPrecision(OUTPUT_PRECISION);
    // find the position
    const size_t addr = address(pos);
    ofs << qSetFieldWidth(OUTPUT_WIDTH) << mData[addr] << qSetFieldWidth(0);
    ofs << '\n';
  }
  // restore flags
  ofs.reset();
  return true;
}

template <typename T>
bool HistogramScalar<T>::writeToFile(const QString &filename) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": writing to " << filename;
  QFile outputFile(filename);
  if (outputFile.open(QFile::WriteOnly)) {
    QTextStream stream(&outputFile);
    return writeToStream(stream);
  } else {
    qDebug() << Q_FUNC_INFO << ": failed to open file!";
    return false;
  }
}

template <typename T>
T HistogramScalar<T>::operator()(const std::vector<double> &position) {
  bool inBoundary = true;
  const size_t addr = address(position, &inBoundary);
  if (inBoundary == false) {
    return T();
  } else {
    return (*this)[addr];
  }
}

template <typename T>
const T
HistogramScalar<T>::operator()(const std::vector<double> &position) const {
  bool inBoundary = true;
  const size_t addr = address(position, &inBoundary);
  if (inBoundary == false) {
    return T();
  } else {
    return (*this)[addr];
  }
}

template <typename T> T &HistogramScalar<T>::operator[](size_t addr) {
  return mData[addr];
}

template <typename T>
const T &HistogramScalar<T>::operator[](size_t addr) const {
  return mData[addr];
}

template <typename T>
void HistogramScalar<T>::applyFunction(std::function<T(T)> f) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  for (size_t i = 0; i < mData.size(); ++i) {
    mData[i] = f(mData[i]);
  }
}

template <typename T> T HistogramScalar<T>::sum() const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  return std::accumulate(mData.begin(), mData.end(), T(0));
}

template <typename T> T HistogramScalar<T>::minimum() const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const T result = *std::min_element(mData.begin(), mData.end());
  return result;
}

template <typename T> const std::vector<T> &HistogramScalar<T>::data() const {
  return mData;
}

template <typename T> std::vector<T> &HistogramScalar<T>::data() {
  return mData;
}

template <typename T>
std::vector<T> HistogramScalar<T>::getDerivative(const std::vector<double> &pos,
                                                 bool *inBoundary) const {
  size_t addr;
  addr = address(pos, inBoundary);
  if (inBoundary != nullptr) {
    if (*inBoundary == false)
      return std::vector<T>(mNdim);
  }
  const T data_this = mData[addr];
  std::vector<T> result(mNdim, T());
  for (size_t i = 0; i < mNdim; ++i) {
    const double bin_width = mAxes[i].width();
    const size_t addr_first = addr - mAccu[i] * mAxes[i].index(pos[i]) + 0;
    const size_t addr_last = addr_first + mAccu[i] * (mAxes[i].bin() - 1);
    if (addr == addr_first) {
      if (mAxes[i].realPeriodic()) {
        const T &data_next = mData[addr + mAccu[i]];
        const T &data_prev = mData[addr_last];
        result[i] = (data_next - data_prev) / (2.0 * bin_width);
      } else {
        const T &data_next = mData[addr + mAccu[i]];
        const T &data_next2 = mData[addr + mAccu[i] * 2];
        result[i] = (data_next2 * -1.0 + data_next * 4.0 - data_this * 3.0) /
                    (2.0 * bin_width);
      }
    } else if (addr == addr_last) {
      if (mAxes[i].realPeriodic()) {
        const T &data_prev = mData[addr - mAccu[i]];
        const T &data_next = mData[addr_first];
        result[i] = (data_next - data_prev) / (2.0 * bin_width);
      } else {
        const T &data_prev = mData[addr - mAccu[i]];
        const T &data_prev2 = mData[addr - mAccu[i] * 2];
        result[i] = (data_this * 3.0 - data_prev * 4.0 + data_prev2) /
                    (2.0 * bin_width);
      }
    } else {
      const T &data_prev = mData[addr - mAccu[i]];
      const T &data_next = mData[addr + mAccu[i]];
      result[i] = (data_next - data_prev) / (2.0 * bin_width);
    }
  }
  return result;
}

template <typename T>
void HistogramScalar<T>::generate(
    std::function<T(const std::vector<double> &)> &func) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  std::vector<double> pos(mNdim, 0.0);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
    }
    const size_t addr = address(pos);
    mData[addr] = func(pos);
  }
}

template <typename T>
bool HistogramScalar<T>::set(const std::vector<double> &pos, const T &value) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  bool inBoundary = true;
  const size_t addr = address(pos, &inBoundary);
  if (inBoundary) {
    mData[addr] = value;
  }
  qDebug() << Q_FUNC_INFO << ": assigned " << value << " to " << pos;
  return inBoundary;
}

template <typename T>
void HistogramScalar<T>::merge(const HistogramScalar<T> &source) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  for (size_t i = 0; i < source.histogramSize(); ++i) {
    bool inSourceBoundary = true;
    bool inThisBoundary = true;
    const std::vector<double> pos = source.reverseAddress(i, &inSourceBoundary);
    const size_t this_addr = this->address(pos, &inThisBoundary);
    if (inSourceBoundary && inThisBoundary) {
      this->mData[this_addr] += source[i];
    }
  }
}

// nD histogram
template <typename T> class HistogramVector : public virtual HistogramBase {
public:
  static_assert(std::is_arithmetic<T>::value,
                "HistogramVector requires a scalar type!");
  HistogramVector();
  HistogramVector(const std::vector<Axis> &, const size_t);
  virtual ~HistogramVector();
  virtual bool readFromStream(QTextStream &ifs, const size_t multiplicity = 0);
  virtual bool readFromFile(const QString &filename);
  virtual bool writeToStream(QTextStream &ofs) const override;
  virtual bool writeToFile(const QString &filename) const;
  virtual std::vector<T> operator()(const std::vector<T> &) const;
  T &operator[](int);
  const T &operator[](int) const;
  virtual void applyFunction(std::function<T(T)> f);
  virtual void
  generate(std::function<std::vector<T>(const std::vector<double> &)> &func);
  size_t multiplicity() const;

protected:
  size_t mMultiplicity;
  std::vector<T> mData;
};

template <typename T>
HistogramVector<T>::HistogramVector()
    : HistogramBase(), mMultiplicity(0), mData(0) {
  qDebug() << "Calling" << Q_FUNC_INFO;
}

template <typename T>
HistogramVector<T>::HistogramVector(const std::vector<Axis> &ax,
                                    const size_t multiplicity)
    : HistogramBase(ax), mMultiplicity(multiplicity) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  mData.resize(mHistogramSize * mMultiplicity, T());
  qDebug() << Q_FUNC_INFO << ": multiplicity = " << mMultiplicity;
  qDebug() << Q_FUNC_INFO << ": data size alllocated = " << mData.size();
  qDebug() << Q_FUNC_INFO << ": axis dimension = " << mNdim;
}

template <typename T> HistogramVector<T>::~HistogramVector() {
  qDebug() << "Calling" << Q_FUNC_INFO;
}

template <typename T>
bool HistogramVector<T>::readFromStream(QTextStream &ifs,
                                        const size_t multiplicity) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  bool file_opened = HistogramBase::readFromStream(ifs);
  if (!file_opened)
    return file_opened;
  // try to use the dimensionality as multiplicity if it is not specified
  mMultiplicity = multiplicity > 0 ? multiplicity : mNdim;
  QString line;
  std::vector<double> pos(mNdim, 0);
  QList<QStringView> tmpFields;
  mData.resize(mHistogramSize * mMultiplicity);
  size_t dataLines = 0;
  const QRegularExpression split_regex("\\s+");
  while (!ifs.atEnd()) {
    ifs.readLineInto(&line);
    QStringView line_view(line);
    tmpFields = line_view.split(split_regex, Qt::SkipEmptyParts);
    // skip blank lines
    if (tmpFields.size() == static_cast<int>(mNdim + mMultiplicity)) {
      // skip unnecessary comment lines starting with #
      if (!tmpFields[0].startsWith(QChar('#'))) {
        bool ok = true;
        for (size_t i = 0; i < mNdim; ++i) {
          pos[i] = tmpFields[i].toDouble(&ok);
          if (!ok)
            return false;
        }
        // find the position
        const size_t addr = address(pos);
        if (ok) {
          for (size_t j = 0; j < mMultiplicity; ++j) {
            mData[addr * mMultiplicity + j] =
                stringToNumber<T>(tmpFields[mNdim + j], &ok);
            if (!ok)
              return false;
            else
              ++dataLines;
          }
        }
      }
    }
  }
  qDebug() << Q_FUNC_INFO << ": expect " << mHistogramSize << " lines, read "
           << dataLines << "lines";
  return true;
}

template <typename T>
bool HistogramVector<T>::readFromFile(const QString &filename) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": opening " << filename;
  QFile inputFile(filename);
  if (inputFile.open(QFile::ReadOnly)) {
    QTextStream stream(&inputFile);
    return readFromStream(stream);
  } else {
    qWarning() << "Failed to open file:" << filename;
    return false;
  }
}

template <typename T>
bool HistogramVector<T>::writeToStream(QTextStream &ofs) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  bool file_opened = HistogramBase::writeToStream(ofs);
  if (!file_opened)
    return file_opened;
  std::vector<double> pos(mNdim, 0);
  ofs.setRealNumberNotation(QTextStream::ScientificNotation);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    ofs.setRealNumberPrecision(OUTPUT_POSITION_PRECISION);
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
      ofs << qSetFieldWidth(OUTPUT_WIDTH);
      ofs.setRealNumberPrecision(OUTPUT_POSITION_PRECISION);
      ofs << pos[j];
      ofs << qSetFieldWidth(0) << ' ';
    }
    ofs.setRealNumberPrecision(OUTPUT_PRECISION);
    const size_t addr = address(pos);
    for (size_t k = 0; k < mMultiplicity; ++k) {
      ofs << qSetFieldWidth(OUTPUT_WIDTH) << mData[addr * mMultiplicity + k];
      ofs << qSetFieldWidth(0) << ' ';
    }
    ofs << qSetFieldWidth(0) << '\n';
  }
  return true;
}

template <typename T>
bool HistogramVector<T>::writeToFile(const QString &filename) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": writing to " << filename;
  QFile outputFile(filename);
  if (outputFile.open(QFile::WriteOnly)) {
    QTextStream stream(&outputFile);
    return writeToStream(stream);
  } else {
    qDebug() << Q_FUNC_INFO << ": failed to open file!";
    return false;
  }
}

template <typename T>
std::vector<T> HistogramVector<T>::operator()(const std::vector<T> &pos) const {
  bool inBoundary = true;
  const size_t addr = address(pos, &inBoundary);
  if (inBoundary) {
    std::vector<T> result(mMultiplicity);
    std::copy_n(mData.begin() + addr * mMultiplicity, mMultiplicity,
                result.begin());
    return result;
  } else {
    return std::vector<T>(mMultiplicity, T());
  }
}

template <typename T> T &HistogramVector<T>::operator[](int addr_mult) {
  return mData[addr_mult];
}

template <typename T>
const T &HistogramVector<T>::operator[](int addr_mult) const {
  return mData[addr_mult];
}

template <typename T>
void HistogramVector<T>::applyFunction(std::function<T(T)> f) {
  for (size_t i = 0; i < mData.size(); ++i) {
    mData[i] = f(mData[i]);
  }
}

template <typename T>
void HistogramVector<T>::generate(
    std::function<std::vector<T>(const std::vector<double> &)> &func) {
  std::vector<double> pos(mNdim, 0);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
    }
    const size_t addr = address(pos);
    const std::vector<T> result = func(pos);
    for (size_t k = 0; k < mMultiplicity; ++k) {
      mData[addr + k] = result[k];
    }
  }
}

template <typename T> size_t HistogramVector<T>::multiplicity() const {
  return mMultiplicity;
}

class HistogramPMF : public HistogramScalar<double> {
public:
  HistogramPMF();
  explicit HistogramPMF(const std::vector<Axis> &ax);
  void toProbability(HistogramScalar<double> &probability, double kbt) const;
  void fromProbability(const HistogramScalar<double> &probability, double kbt);
};

class HistogramProbability : public HistogramScalar<double> {
public:
  HistogramProbability();
  explicit HistogramProbability(const std::vector<Axis> &ax);
  virtual ~HistogramProbability();
  void convertToFreeEnergy(double kbt);
  HistogramProbability
  reduceDimension(const std::vector<size_t> &new_dims) const;
};

class HistogramPMFHistory : public HistogramBase {
public:
  HistogramPMFHistory();
  HistogramPMFHistory(const std::vector<Axis> &ax);
  void appendHistogram(const std::vector<double> &data);
  std::vector<double> computeRMSD() const;
  std::vector<double>
  computeRMSD(const std::vector<double> &referenceData) const;
  void splitToFile(const QString &prefix) const;

private:
  QList<std::vector<double>> mHistoryData;
};

class PMFPathFinder {
public:
  PMFPathFinder();
  PMFPathFinder(const HistogramScalar<double> &histogram,
                const std::vector<GridDataPatch> &patchList);
  PMFPathFinder(const HistogramScalar<double> &histogram,
                const std::vector<GridDataPatch> &patchList,
                const std::vector<double> &pos_start,
                const std::vector<double> &pos_end, Graph::FindPathMode mode,
                Graph::FindPathAlgorithm algorithm);
  void setup(const HistogramScalar<double> &histogram,
             const std::vector<GridDataPatch> &patchList,
             const std::vector<double> &pos_start,
             const std::vector<double> &pos_end, Graph::FindPathMode mode,
             Graph::FindPathAlgorithm algorithm);
  bool initialized() const;
  void findPath();
  void writePath(const QString &filename) const;
  void writeVisitedRegion(const QString &filename) const;
  void writePatchedPMF(const QString &filename) const;
  Graph::FindPathResult result() const;
  HistogramScalar<double> histogram() const;
  HistogramScalar<double> histogramBackup() const;
  std::vector<GridDataPatch> patchList() const;
  void setPatchList(const std::vector<GridDataPatch> &patchList);
  std::vector<double> posStart() const;
  void setPosStart(const std::vector<double> &posStart);
  std::vector<double> posEnd() const;
  void setPosEnd(const std::vector<double> &posEnd);
  std::vector<std::vector<double>> pathPosition() const;
  std::vector<double> pathEnergy() const;

private:
  void setupGraph();
  void applyPatch();
  bool hasData;
  HistogramScalar<double> mHistogram;
  HistogramScalar<double> mHistogramBackup;
  std::vector<GridDataPatch> mPatchList;
  std::vector<double> mPosStart;
  std::vector<double> mPosEnd;
  Graph mGraph;
  Graph::FindPathAlgorithm mAlgorithm;
  Graph::FindPathMode mMode;
  Graph::FindPathResult mResult;
};

Q_DECLARE_METATYPE(HistogramPMF);
Q_DECLARE_METATYPE(HistogramProbability);
Q_DECLARE_METATYPE(HistogramPMFHistory);

#endif // HISTOGRAMBASE_H
