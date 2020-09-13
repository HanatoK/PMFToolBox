#ifndef HISTOGRAMBASE_H
#define HISTOGRAMBASE_H

#include "helper.h"

#include <QDebug>
#include <QFile>
#include <QObject>
#include <QPair>
#include <QString>
#include <QTextStream>
#include <QThread>
#include <QVector>

#include <functional>
#include <cctype>

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
  QVector<double> getMiddlePoints() const;
  double lowerBound() const;
  double upperBound() const;
  double setLowerBound(double newLowerBound);
  double setUpperBound(double newUpperBound);
  double setWidth(double new_width);
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

class HistogramBase {
public:
  HistogramBase();
  virtual ~HistogramBase();
  explicit HistogramBase(const QVector<Axis> &ax);
  virtual bool readFromStream(QTextStream &ifs);
  virtual bool writeToStream(QTextStream &ofs) const;
  bool isInGrid(const QVector<double> &position) const;
  virtual QVector<size_t> index(const QVector<double> &position,
                                bool *inBoundary = nullptr) const;
  virtual size_t address(const QVector<double> &position,
                         bool *inBoundary = nullptr) const;
  QVector<double> reverseAddress(size_t address,
                                 bool *inBoundary = nullptr) const;
  virtual QPair<size_t, bool> neighbor(const QVector<double> &position,
                                       size_t axisIndex,
                                       bool previous = false) const;
  virtual QPair<size_t, bool> neighborByAddress(size_t address,
                                                size_t axisIndex,
                                                bool previous = false) const;
  virtual QVector<QPair<size_t, bool>>
  allNeighbor(const QVector<double> &position) const;
  virtual QVector<QPair<size_t, bool>>
  allNeighborByAddress(size_t address) const;
  size_t histogramSize() const;
  size_t dimension() const;
  QVector<Axis> axes() const;
  QVector<QVector<double>> pointTable() const;

protected:
  size_t mNdim;
  size_t mHistogramSize;
  QVector<Axis> mAxes;
  QVector<QVector<double>> mPointTable;
  QVector<size_t> mAccu;

private:
  void fillTable();
};

// 1D histogram
template <typename T> class HistogramScalar : public virtual HistogramBase {
public:
  static_assert(std::is_arithmetic<T>::value,
                "HistogramScalar requires a scalar type!");
  HistogramScalar();
  explicit HistogramScalar(const QVector<Axis> &ax);
  virtual ~HistogramScalar();
  virtual bool readFromStream(QTextStream &ifs) override;
  virtual bool readFromFile(const QString &filename);
  virtual bool writeToStream(QTextStream &ofs) const override;
  virtual bool writeToFile(const QString &filename) const;
  virtual T operator()(const QVector<double> &position);
  virtual const T operator()(const QVector<double> &position) const;
  virtual T &operator[](size_t addr);
  virtual const T &operator[](size_t addr) const;
  virtual void applyFunction(std::function<T(T)> f);
  T sum() const;
  T minimum() const;
  const QVector<T> &data() const;
  QVector<T> &data();
  HistogramScalar reduceDimension(const QVector<size_t> &new_dims) const;
  virtual QVector<T> getDerivative(const QVector<double> &pos,
                                   bool *inBoundary = nullptr) const;
  virtual void generate(std::function<T(const QVector<double> &)> &func);
  virtual bool set(const QVector<double> &pos, const T &value);
  virtual void merge(const HistogramScalar<T> &source);

protected:
  QVector<T> mData;

private:
  static const int OUTPUT_PRECISION = 7;
  static const int OUTPUT_POSITION_PRECISION = 5;
  static const int OUTPUT_WIDTH = 14;
};

template <typename T> HistogramScalar<T>::HistogramScalar() {
  qDebug() << "Calling " << Q_FUNC_INFO;
}

template <typename T>
HistogramScalar<T>::HistogramScalar(const QVector<Axis> &ax)
    : HistogramBase(ax) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  mData.resize(mHistogramSize);
}

template <typename T> HistogramScalar<T>::~HistogramScalar() {
  qDebug() << "Calling " << Q_FUNC_INFO;
}

template <typename T>
bool HistogramScalar<T>::readFromStream(QTextStream &ifs) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  bool file_opened = HistogramBase::readFromStream(ifs);
  if (!file_opened)
    return file_opened;
  // read data into m_data
  QString line;
  QVector<double> pos(mNdim, 0);
  QStringList tmp_fields;
  mData.resize(mHistogramSize);
  size_t dataLines = 0;
  while (!ifs.atEnd()) {
    line.clear();
    tmp_fields.clear();
    ifs.readLineInto(&line);
    tmp_fields = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    // skip blank lines
    if (tmp_fields.size() == int(mNdim) + 1) {
      // skip unnecessary comment lines starting with #
      if (!tmp_fields[0].startsWith("#")) {
        bool ok = true;
        for (size_t i = 0; i < mNdim; ++i) {
          pos[i] = tmp_fields[i].toDouble(&ok);
          if (!ok)
            return false;
        }
        // find the position
        const size_t addr = address(pos);
        if (ok) {
          mData[addr] = stringToNumber<T>(tmp_fields[mNdim], &ok);
          if (!ok)
            return false;
          else
            ++dataLines;
        }
      }
    } else if (tmp_fields.size() == 0) {
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
  qDebug() << "Calling " << Q_FUNC_INFO;
  qDebug() << Q_FUNC_INFO << ": opening " << filename;
  QFile inputFile(filename);
  if (inputFile.open(QFile::ReadOnly)) {
    QTextStream stream(&inputFile);
    return readFromStream(stream);
  } else {
    qDebug() << Q_FUNC_INFO << ": failed to open file!";
    return false;
  }
}

template <typename T>
bool HistogramScalar<T>::writeToStream(QTextStream &ofs) const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  bool file_opened = HistogramBase::writeToStream(ofs);
  if (!file_opened)
    return file_opened;
  QVector<double> pos(mNdim, 0);
  ofs.setRealNumberNotation(QTextStream::FixedNotation);
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
  qDebug() << "Calling " << Q_FUNC_INFO;
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
T HistogramScalar<T>::operator()(const QVector<double> &position) {
  bool inBoundary = true;
  const size_t addr = address(position, &inBoundary);
  if (inBoundary == false) {
    return T();
  } else {
    return (*this)[addr];
  }
}

template <typename T>
const T HistogramScalar<T>::operator()(const QVector<double> &position) const {
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
  qDebug() << "Calling " << Q_FUNC_INFO;
  for (int i = 0; i < mData.size(); ++i) {
    mData[i] = f(mData[i]);
  }
}

template <typename T> T HistogramScalar<T>::sum() const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  return std::accumulate(mData.begin(), mData.end(), T(0));
}

template <typename T> T HistogramScalar<T>::minimum() const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  const T result = *std::min_element(mData.begin(), mData.end());
  return result;
}

template<typename T>
const QVector<T> &HistogramScalar<T>::data() const
{
  return mData;
}

template<typename T>
QVector<T> &HistogramScalar<T>::data()
{
  return mData;
}

template <typename T>
QVector<T> HistogramScalar<T>::getDerivative(const QVector<double> &pos,
                                             bool *inBoundary) const {
  size_t addr;
  addr = address(pos, inBoundary);
  if (inBoundary != nullptr) {
    if (*inBoundary == false)
      return QVector<T>(mNdim);
  }
  const T data_this = mData[addr];
  QVector<T> result(mNdim, T());
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
    std::function<T(const QVector<double> &)> &func) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  QVector<double> pos(mNdim, 0.0);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
    }
    const size_t addr = address(pos);
    mData[addr] = func(pos);
  }
}

template <typename T>
bool HistogramScalar<T>::set(const QVector<double> &pos, const T &value) {
  qDebug() << "Calling " << Q_FUNC_INFO;
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
  qDebug() << "Calling " << Q_FUNC_INFO;
  for (size_t i = 0; i < source.histogramSize(); ++i) {
    bool inSourceBoundary = true;
    bool inThisBoundary = true;
    const QVector<double> pos = source.reverseAddress(i, &inSourceBoundary);
    const size_t this_addr = this->address(pos, &inThisBoundary);
    if (inSourceBoundary && inThisBoundary) {
      this->mData[this_addr] += source[i];
    }
  }
}

template <typename T>
HistogramScalar<T>
HistogramScalar<T>::reduceDimension(const QVector<size_t> &new_dims) const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  QVector<Axis> new_ax;
  for (int i = 0; i < new_dims.size(); ++i) {
    new_ax.push_back(this->mAxes[new_dims[i]]);
  }
  HistogramScalar<T> new_hist(new_ax);
  QVector<double> pos(mNdim, 0.0);
  QVector<double> new_pos(new_hist.getDimension(), 0.0);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
    }
    for (size_t k = 0; k < new_hist.getDimension(); ++k) {
      new_pos[k] = pos[new_dims[k]];
    }
    bool in_grid = true;
    bool in_new_grid = true;
    const size_t addr = address(pos, &in_grid);
    const size_t new_addr = new_hist.address(new_pos, &in_new_grid);
    if (in_grid && in_new_grid) {
      new_hist[new_addr] += (*this)[addr];
    }
  }
  return new_hist;
}

// nD histogram
template <typename T> class HistogramVector : public virtual HistogramBase {
public:
  static_assert(std::is_arithmetic<T>::value,
                "HistogramVector requires a scalar type!");
  HistogramVector();
  HistogramVector(const QVector<Axis> &, const size_t);
  virtual ~HistogramVector();
  virtual bool readFromStream(QTextStream &ifs, const size_t multiplicity = 0);
  virtual bool writeToStream(QTextStream &ofs) const override;
  virtual QVector<T> operator()(const QVector<T> &);
  T &operator[](int);
  const T &operator[](int) const;
  virtual void applyFunction(std::function<T(T)> f);
  virtual void
  generate(std::function<QVector<T>(const QVector<double> &)> &func);

protected:
  size_t mMultiplicity;
  QVector<T> mData;

private:
  static const int OUTPUT_PRECISION = 7;
  static const int OUTPUT_POSITION_PRECISION = 5;
  static const int OUTPUT_WIDTH = 14;
};

template <typename T> HistogramVector<T>::HistogramVector() : HistogramBase() {
  qDebug() << "Calling " << Q_FUNC_INFO;
  mMultiplicity = 0;
}

template <typename T>
HistogramVector<T>::HistogramVector(const QVector<Axis> &ax,
                                    const size_t multiplicity)
    : HistogramBase(ax), mMultiplicity(multiplicity) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  mData.resize(mHistogramSize * mMultiplicity, T());
  qDebug() << Q_FUNC_INFO << ": multiplicity = " << mMultiplicity;
  qDebug() << Q_FUNC_INFO << ": data size alllocated = " << mData.size();
}

template <typename T> HistogramVector<T>::~HistogramVector() {
  qDebug() << "Calling " << Q_FUNC_INFO;
}

template <typename T>
bool HistogramVector<T>::readFromStream(QTextStream &ifs,
                                        const size_t multiplicity) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  bool file_opened = HistogramBase::readFromStream(ifs);
  if (!file_opened)
    return file_opened;
  // try to use the dimensionality as multiplicity if it is not specified
  mMultiplicity = multiplicity > 0 ? multiplicity : mNdim;
  QString line;
  QVector<double> pos(mNdim, 0);
  QStringList tmp_fields;
  mData.resize(mHistogramSize * mMultiplicity);
  size_t dataLines = 0;
  while (!ifs.atEnd()) {
    line.clear();
    tmp_fields.clear();
    tmp_fields = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    // skip blank lines
    if (tmp_fields.size() == (mNdim + mMultiplicity)) {
      // skip unnecessary comment lines starting with #
      if (!tmp_fields[0].startsWith("#")) {
        bool ok = true;
        for (size_t i = 0; i < mNdim; ++i) {
          pos[i] = tmp_fields[i].toDouble(&ok);
          if (!ok)
            return false;
        }
        // find the position
        const size_t addr = address(pos);
        if (ok) {
          for (size_t j = 0; j < mMultiplicity; ++j) {
            mData[addr * mMultiplicity + j] =
                stringToNumber<T>(tmp_fields[mNdim + j], &ok);
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
bool HistogramVector<T>::writeToStream(QTextStream &ofs) const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  bool file_opened = HistogramBase::writeToStream(ofs);
  if (!file_opened)
    return file_opened;
  QVector<double> pos(mNdim, 0);
  ofs.setRealNumberNotation(QTextStream::FixedNotation);
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
QVector<T> HistogramVector<T>::operator()(const QVector<T> &pos) {
  bool inBoundary = true;
  const size_t addr = address(pos, &inBoundary);
  if (inBoundary) {
    QVector<T> result(mMultiplicity);
    std::copy_n(mData.begin() + addr * mMultiplicity, mMultiplicity,
                result.begin());
    return result;
  } else {
    return QVector<T>(mMultiplicity, T());
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
    std::function<QVector<T>(const QVector<double> &)> &func) {
  QVector<double> pos(mNdim, 0);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
    }
    const size_t addr = address(pos);
    const QVector<T> result = func(pos);
    for (size_t k = 0; k < mMultiplicity; ++k) {
      mData[addr + k] = result[k];
    }
  }
}

class HistogramPMF : public HistogramScalar<double> {
public:
  HistogramPMF();
  explicit HistogramPMF(const QVector<Axis> &ax);
  void toProbability(HistogramScalar<double> &probability, double kbt) const;
  void fromProbability(const HistogramScalar<double> &probability, double kbt);
};

class HistogramProbability : public HistogramScalar<double> {
public:
  HistogramProbability();
  explicit HistogramProbability(const QVector<Axis> &ax);
  virtual ~HistogramProbability();
  void convertToFreeEnergy(double kbt);
};

#endif // HISTOGRAMBASE_H