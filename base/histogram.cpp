#include "histogram.h"

#include <algorithm>
#include <cmath>
#include <iterator>

HistogramBase::HistogramBase(): mNdim(0), mHistogramSize(0), mAxes(0), mPointTable(0), mAccu(0) {

}

HistogramBase::~HistogramBase() { qDebug() << "Calling " << Q_FUNC_INFO; }

HistogramBase::HistogramBase(const QVector<Axis> &ax)
    : mNdim(ax.size()), mAxes(ax), mAccu(mNdim) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  if (mNdim == 0)
    return;
  mHistogramSize = 1;
  for (size_t i = 0; i < mNdim; ++i) {
    mAccu[i] = (i == 0) ? 1 : (mAccu[i - 1] * mAxes[i - 1].bin());
    mHistogramSize *= mAxes[i].bin();
  }
  qDebug() << "Dimensionality is " << mNdim;
  qDebug() << "Histogram size is " << mHistogramSize;
  fillTable();
}

bool HistogramBase::readFromStream(QTextStream &ifs) {
  if (ifs.status() != QTextStream::Ok)
    return false;
  QString line;
  ifs.readLineInto(&line);
  QStringList tmp = line.split(QRegExp("\\s+"));
  if (tmp.size() < 2)
    return false;
  mNdim = tmp[1].toULongLong();
  mAxes.resize(mNdim);
  mAccu.resize(mNdim);
  // now we know how many axes should be read
  for (size_t i = 0; i < mNdim; ++i) {
    tmp.clear();
    line.clear();
    ifs.readLineInto(&line);
    tmp = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    if (tmp.size() < 5)
      return false;
    // initialize each axis
    // format: # lower_bound bin_width num_bins is_periodic
    mAxes[i].mLowerBound = tmp[1].toDouble();
    mAxes[i].mWidth = tmp[2].toDouble();
    mAxes[i].mBins = tmp[3].toULongLong();
    mAxes[i].mUpperBound =
        mAxes[i].mLowerBound + mAxes[i].mWidth * double(mAxes[i].mBins);
    mAxes[i].mPeriodic = (tmp[4].toInt() == 0) ? false : true;
    if (mAxes[i].mPeriodic) {
      mAxes[i].mPeriodicLowerBound = mAxes[i].mLowerBound;
      mAxes[i].mPeriodicUpperBound = mAxes[i].mUpperBound;
    }
  }
  // initialize other variables
  mHistogramSize = 1;
  for (size_t i = 0; i < mNdim; ++i) {
    mAccu[i] = (i == 0) ? 1 : (mAccu[i - 1] * mAxes[i - 1].bin());
    mHistogramSize *= mAxes[i].bin();
  }
  // initialize the table
  fillTable();
  return true;
}

bool HistogramBase::writeToStream(QTextStream &ofs) const {
  if (ofs.status() != QTextStream::Ok)
    return false;
  ofs << "# " << mNdim << '\n';
  for (const auto &ax : mAxes) {
    ofs << ax.infoHeader() << '\n';
  }
  return true;
}

bool HistogramBase::isInGrid(const QVector<double> &position) const {
  auto it_val = position.cbegin();
  auto it_ax = mAxes.cbegin();
  while (it_ax != mAxes.cend()) {
    if (!(it_ax->inBoundary(*it_val)))
      return false;
  }
  return true;
}

QVector<size_t> HistogramBase::index(const QVector<double> &position,
                                     bool *inBoundary) const {
  QVector<size_t> idx(mNdim, 0);
  for (size_t i = 0; i < mNdim; ++i) {
    if (inBoundary != nullptr) {
      idx[i] = mAxes[i].index(position[i], inBoundary);
      if (*inBoundary == false) {
        qDebug() << "Warning: position " << position << " is not in boundary!";
        break;
      }
    } else {
      idx[i] = mAxes[i].index(position[i]);
    }
  }
  return idx;
}

size_t HistogramBase::address(const QVector<double> &position,
                              bool *inBoundary) const {
  size_t addr = 0;
  for (size_t i = 0; i < mNdim; ++i) {
    addr += mAccu[i] * mAxes[i].index(position[i], inBoundary);
    if (inBoundary != nullptr) {
      if (*inBoundary == false) {
        break;
      }
    }
  }
  return addr;
}

QVector<double> HistogramBase::reverseAddress(size_t address,
                                              bool *inBoundary) const {
  QVector<double> pos(mNdim, 0);
  if (address >= mHistogramSize) {
    if (inBoundary != nullptr) {
      *inBoundary = false;
    }
  } else {
    for (int i = mNdim - 1; i >= 0; --i) {
      const size_t index_i = static_cast<size_t>(
          std::floor(static_cast<double>(address) / mAccu[i]));
      pos[i] = mAxes[i].mLowerBound + (0.5 + index_i) * mAxes[i].bin();
      address -= index_i * mAccu[i];
    }
    if (inBoundary != nullptr) {
      *inBoundary = true;
    }
  }
  return pos;
}

QPair<size_t, bool> HistogramBase::neighbor(const QVector<double> &position,
                                            size_t axisIndex,
                                            bool previous) const {
  const double bin_width_i = mAxes[axisIndex].width();
  QVector<double> pos_next(position);
  if (previous == true) {
    pos_next[axisIndex] -= bin_width_i;
  } else {
    pos_next[axisIndex] += bin_width_i;
  }
  bool inBoundary;
  const size_t addr_neighbor = address(pos_next, &inBoundary);
  return QPair(addr_neighbor, inBoundary);
}

QPair<size_t, bool> HistogramBase::neighborByAddress(size_t address,
                                                     size_t axisIndex,
                                                     bool previous) const {
  if (address >= mHistogramSize)
    return QPair(0, false);
  QVector<size_t> index(mNdim, 0);
  for (int i = mNdim - 1; i >= 0; --i) {
    index[i] = static_cast<size_t>(
        std::floor(static_cast<double>(address) / mAccu[i]));
    address -= index[i] * mAccu[i];
  }
  if (previous == true) { // find previous neighbour
    if (mAxes[axisIndex].periodic()) {
      if (index[axisIndex] == 0) {
        index[axisIndex] = mAxes[axisIndex].mBins - 1;
      } else {
        index[axisIndex] -= 1;
      }
    } else {
      if (index[axisIndex] == 0) {
        return QPair(0, false);
      } else {
        index[axisIndex] -= 1;
      }
    }
  } else { // find next neighbour
    if (mAxes[axisIndex].periodic()) {
      if (index[axisIndex] == mAxes[axisIndex].mBins - 1) {
        index[axisIndex] = 0;
      } else {
        index[axisIndex] += 1;
      }
    } else {
      if (index[axisIndex] == mAxes[axisIndex].mBins - 1) {
        return QPair(0, false);
      } else {
        index[axisIndex] += 1;
      }
    }
  }
  size_t neighbour_address = 0;
  for (size_t i = 0; i < mNdim; ++i) {
    neighbour_address += index[i] * mAccu[i];
  }
  return QPair(neighbour_address, true);
}

QVector<QPair<size_t, bool>>
HistogramBase::allNeighbor(const QVector<double> &position) const {
  QVector<QPair<size_t, bool>> results(mNdim * 2);
  for (size_t i = 0; i < mNdim; ++i) {
    results[i * 2] = neighbor(position, i, true);
    results[i * 2 + 1] = neighbor(position, i, false);
  }
  return results;
}

QVector<QPair<size_t, bool>>
HistogramBase::allNeighborByAddress(size_t address) const {
  QVector<QPair<size_t, bool>> results(mNdim * 2);
  for (size_t i = 0; i < mNdim; ++i) {
    results[i * 2] = neighborByAddress(address, i, true);
    results[i * 2 + 1] = neighborByAddress(address, i, false);
  }
  return results;
}

size_t HistogramBase::histogramSize() const { return mHistogramSize; }

size_t HistogramBase::dimension() const { return mNdim; }

QVector<Axis> HistogramBase::axes() const { return mAxes; }

QVector<QVector<double>> HistogramBase::pointTable() const {
  return mPointTable;
}

void HistogramBase::fillTable() {
  qDebug() << "Calling " << Q_FUNC_INFO;
  QVector<QVector<double>> middlePoint(mNdim);
  for (size_t i = 0; i < mNdim; ++i) {
    middlePoint[i] = mAxes[i].getMiddlePoints();
  }
  mPointTable = QVector(mNdim, QVector<double>(mHistogramSize, 0.0));
  for (size_t i = 0; i < mNdim; ++i) {
    size_t repeatAll = 1, repeatOne = 1;
    for (size_t j = i + 1; j < mNdim; ++j) {
      repeatOne *= middlePoint[j].size();
    }
    for (size_t j = 0; j < i; ++j) {
      repeatAll *= middlePoint[j].size();
    }
    const size_t in_i_sz = middlePoint[i].size();
    for (size_t l = 0; l < in_i_sz; ++l) {
      std::fill_n(mPointTable[i].begin() + l * repeatOne, repeatOne,
                  middlePoint[i][l]);
    }
    for (size_t k = 0; k < repeatAll - 1; ++k) {
      std::copy_n(mPointTable[i].begin(), repeatOne * in_i_sz,
                  mPointTable[i].begin() + repeatOne * in_i_sz * (k + 1));
    }
  }
}

Axis::Axis()
    : mLowerBound(0.0), mUpperBound(0.0), mBins(0), mWidth(0.0),
      mPeriodic(false), mPeriodicLowerBound(0.0), mPeriodicUpperBound(0.0) {
  qDebug() << "Calling " << Q_FUNC_INFO;
}

Axis::Axis(double lowerBound, double upperBound, size_t bins, bool periodic)
    : mLowerBound(lowerBound), mUpperBound(upperBound), mBins(bins),
      mPeriodic(periodic) {
  qDebug() << "Calling " << Q_FUNC_INFO;
  mWidth = (mUpperBound - mLowerBound) / static_cast<double>(mBins);
  mPeriodicLowerBound = mLowerBound;
  mPeriodicUpperBound = mUpperBound;
}

double Axis::width() const { return mWidth; }

size_t Axis::bin() const { return mBins; }

bool Axis::inBoundary(double x) const {
  x = wrap(x);
  if (x < mLowerBound || x > mUpperBound) {
    return false;
  } else {
    return true;
  }
}

size_t Axis::index(double x, bool *inBoundary) const {
  x = wrap(x);
  bool checkResult = true;
  if (inBoundary != nullptr) {
    checkResult = this->inBoundary(x);
    *inBoundary = checkResult;
  }
  if (checkResult == false) {
    return 0;
  }
  size_t idx = std::floor((x - mLowerBound) / mWidth);
  if (idx == mBins)
    --idx;
  return idx;
}

double Axis::wrap(double x) const {
  if (!mPeriodic)
    return x;
  if (x >= mPeriodicLowerBound && x <= mPeriodicUpperBound)
    return x;
  const double periodicity = mPeriodicUpperBound - mPeriodicLowerBound;
  if (x < mPeriodicLowerBound) {
    const double dist_to_lower = mPeriodicLowerBound - x;
    const int num_period_add = int(dist_to_lower / periodicity);
    const double tmp = std::abs(dist_to_lower / periodicity -
                                (std::nearbyint(dist_to_lower / periodicity)));
    if (almost_equal(tmp, 0.0)) {
      x += num_period_add * periodicity;
    } else {
      x += (num_period_add + 1) * periodicity;
    }
  }
  if (x > mPeriodicUpperBound) {
    const double dist_to_upper = x - mPeriodicUpperBound;
    const int num_period_subtract = int(dist_to_upper / periodicity);
    const double tmp = std::abs(dist_to_upper / periodicity -
                                (std::nearbyint(dist_to_upper / periodicity)));
    if (almost_equal(tmp, 0.0)) {
      x -= num_period_subtract * periodicity;
    } else {
      x -= (num_period_subtract + 1) * periodicity;
    }
  }
  return x;
}

QString Axis::infoHeader() const {
  const int pbc = mPeriodic ? 1 : 0;
  const QString str = QString("# %1 %2 %3 %4")
                          .arg(mLowerBound, 0, 'f', 9)
                          .arg(mWidth, 0, 'f', 9)
                          .arg(mBins)
                          .arg(pbc);
  return str;
}

QVector<double> Axis::getMiddlePoints() const {
  double tmp = mLowerBound - 0.5 * mWidth;
  QVector<double> result(mBins, 0.0);
  for (auto &i : result) {
    tmp += mWidth;
    i = tmp;
  }
  return result;
}

double Axis::lowerBound() const { return mLowerBound; }

double Axis::upperBound() const { return mUpperBound; }

double Axis::setLowerBound(double newLowerBound) {
  // keep bin width and reset lower bound
  mBins = std::nearbyintl((mUpperBound - newLowerBound) / mWidth);
  mLowerBound = mUpperBound - double(mBins) * mWidth;
  return mLowerBound;
}

double Axis::setUpperBound(double newUpperBound) {
  // keep bin width and reset upper bound
  mBins = std::nearbyintl((newUpperBound - mLowerBound) / mWidth);
  mUpperBound = mLowerBound + double(mBins) * mWidth;
  return mUpperBound;
}

double Axis::setWidth(double new_width) {
  if (new_width <= 0)
    return -1.0;
  mBins = std::nearbyintl((mUpperBound - mLowerBound) / new_width);
  mWidth = (mUpperBound - mLowerBound) / double(mBins);
  return mWidth;
}

bool Axis::realPeriodic() const {
  if (mPeriodic) {
    if (std::abs(mLowerBound - mPeriodicLowerBound) < mWidth &&
        std::abs(mUpperBound - mPeriodicUpperBound) < mWidth) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool Axis::periodic() const { return mPeriodic; }

double Axis::period() const {
  return mPeriodicUpperBound - mPeriodicLowerBound;
}

HistogramPMF::HistogramPMF() : HistogramBase(), HistogramScalar<double>() {}

HistogramPMF::HistogramPMF(const QVector<Axis> &ax)
    : HistogramBase(ax), HistogramScalar<double>(ax) {}

void HistogramPMF::toProbability(HistogramScalar<double> &probability,
                                 double kbt) const {
  probability = *this;
  probability.applyFunction(
      [kbt](double x) { return std::exp(-1.0 * x / kbt); });
}

void HistogramPMF::fromProbability(const HistogramScalar<double> &probability,
                                   double kbt) {
  *this = HistogramPMF(probability.axes());
  const double norm_factor = probability.sum();
  if (norm_factor > 0) {
    for (size_t i = 0; i < mHistogramSize; ++i) {
      mData[i] = -1.0 * kbt * std::log(probability[i] / norm_factor);
    }
    const double minimum = this->minimum();
    this->applyFunction([minimum](double x) { return x - minimum; });
  }
}

HistogramProbability::HistogramProbability(): HistogramBase(), HistogramScalar<double>()
{

}

HistogramProbability::HistogramProbability(const QVector<Axis> &ax): HistogramBase(ax), HistogramScalar<double>(ax)
{

}

HistogramProbability::~HistogramProbability()
{

}

void HistogramProbability::convertToFreeEnergy(double kbt)
{
  QVector<double> f_data(this->data());
  bool first_non_zero_value = true;
  double max_val = 0;
  for (auto &i : f_data) {
    if (i > 0) {
      i = -kbt * std::log(i);
      if (first_non_zero_value) {
        max_val = i;
        first_non_zero_value = false;
      }
      max_val = std::max(max_val, i);
    }
  }
  const QVector<double> &p_data = this->data();
  for (int i = 0; i < p_data.size(); ++i) {
    if (p_data[i] == 0) {
      f_data[i] = max_val;
    }
  }
  const double min_val = *std::min_element(f_data.begin(), f_data.end());
  for (auto &i : f_data) {
    i = i - min_val;
  }
  this->mData = f_data;
}

HistogramProbability HistogramProbability::reduceDimension(const QVector<size_t> &new_dims) const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  QVector<Axis> new_ax;
  for (int i = 0; i < new_dims.size(); ++i) {
    new_ax.push_back(this->mAxes[new_dims[i]]);
  }
  HistogramProbability new_hist(new_ax);
  QVector<double> pos(mNdim, 0.0);
  QVector<double> new_pos(new_hist.dimension(), 0.0);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
    }
    for (size_t k = 0; k < new_hist.dimension(); ++k) {
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

QDebug operator<<(QDebug dbg, const Axis &ax)
{
  dbg << "{" << ax.lowerBound() << ", " << ax.upperBound() << ", " << ax.width() << "}";
  return dbg;
}

HistogramPMFHistory::HistogramPMFHistory(): HistogramBase()
{

}

HistogramPMFHistory::HistogramPMFHistory(const QVector<Axis> &ax): HistogramBase(ax)
{

}

void HistogramPMFHistory::appendHistogram(const QVector<double> &data)
{
  mHistoryData.append(data);
}

QVector<double> HistogramPMFHistory::computeRMSD() const
{
  const QVector<double>& lastFrame = mHistoryData.back();
  return computeRMSD(lastFrame);
}

QVector<double> HistogramPMFHistory::computeRMSD(const QVector<double>& referenceData) const
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  QVector<double> result;
  for (int i = 0; i < mHistoryData.size(); ++i) {
    const QVector<double>& currentData = mHistoryData[i];
    double rmsd = 0;
    for (int j = 0; j < referenceData.size(); ++j) {
      const double diff = referenceData[j] - currentData[j];
      rmsd += diff * diff;
    }
    rmsd /= referenceData.size();
    result.append(std::sqrt(rmsd));
  }
  return result;
}

// Does it need threading?
void HistogramPMFHistory::splitToFile(const QString &prefix) const
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  const int numPMFs = mHistoryData.size();
  const int numDigits = QString::number(numPMFs).size();
  qDebug() << Q_FUNC_INFO << ": number of PMFs = " << numPMFs;
  for (int i = 0; i < numPMFs; ++i) {
    const QString suffix = QStringLiteral("%1").arg(i, numDigits, 10, QLatin1Char('0'));
    const QString filename = prefix + "_" + suffix + ".pmf";
    QFile outputFile(filename);
    if (outputFile.open(QIODevice::WriteOnly)) {
      qDebug() << Q_FUNC_INFO << ": writing " << filename;
      QVector<double> pos(mNdim, 0);
      QTextStream ofs(&outputFile);
      HistogramBase::writeToStream(ofs);
      ofs.setRealNumberNotation(QTextStream::FixedNotation);
      for (size_t j = 0; j < mHistogramSize; ++j) {
        for (size_t k = 0; k < mNdim; ++k) {
          pos[k] = mPointTable[k][j];
          ofs << qSetFieldWidth(OUTPUT_WIDTH);
          ofs.setRealNumberPrecision(OUTPUT_POSITION_PRECISION);
          ofs << pos[k];
          ofs << qSetFieldWidth(0) << ' ';
        }
        ofs.setRealNumberPrecision(OUTPUT_PRECISION);
        const size_t addr = address(pos);
        ofs << qSetFieldWidth(OUTPUT_WIDTH) << mHistoryData[i][addr] << qSetFieldWidth(0);
        ofs << '\n';
      }
      ofs.flush();
    }
    outputFile.close();
  }
}
