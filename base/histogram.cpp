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

#include "histogram.h"

#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <iterator>

HistogramBase::HistogramBase()
    : mNdim(0), mHistogramSize(0), mAxes(0), mPointTable(0), mAccu(0) {}

HistogramBase::~HistogramBase() { qDebug() << "Calling" << Q_FUNC_INFO; }

HistogramBase::HistogramBase(const std::vector<Axis> &ax)
    : mNdim(ax.size()), mAxes(ax), mAccu(mNdim) {
  qDebug() << "Calling" << Q_FUNC_INFO;
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
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (ifs.status() != QTextStream::Ok)
    return false;
  QString line;
  ifs.readLineInto(&line);
  const QRegularExpression split_regex("\\s+");
  QVector<QStringRef> tmp = line.splitRef(split_regex, Qt::SkipEmptyParts);
  if (tmp.size() < 2)
    return false;
  mNdim = tmp[1].toULongLong();
  mAxes.resize(mNdim);
  mAccu.resize(mNdim);
  // now we know how many axes should be read
  for (size_t i = 0; i < mNdim; ++i) {
    line.clear();
    ifs.readLineInto(&line);
    tmp = line.splitRef(split_regex, Qt::SkipEmptyParts);
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
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (ofs.status() != QTextStream::Ok)
    return false;
  ofs << "# " << mNdim << '\n';
  for (const auto &ax : mAxes) {
    ofs << ax.infoHeader() << '\n';
  }
  return true;
}

bool HistogramBase::isInGrid(const std::vector<double> &position) const {
  auto it_val = position.cbegin();
  auto it_ax = mAxes.cbegin();
  while (it_ax != mAxes.cend()) {
    if (!(it_ax->inBoundary(*it_val)))
      return false;
  }
  return true;
}

std::vector<size_t> HistogramBase::index(const std::vector<double> &position,
                                         bool *inBoundary) const {
  std::vector<size_t> idx(mNdim, 0);
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

size_t HistogramBase::address(const std::vector<double> &position,
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

std::vector<double> HistogramBase::reverseAddress(size_t address,
                                                  bool *inBoundary) const {
  std::vector<double> pos(mNdim, 0);
  if (address >= mHistogramSize) {
    if (inBoundary != nullptr) {
      *inBoundary = false;
    }
  } else {
    for (int i = mNdim - 1; i >= 0; --i) {
      const size_t index_i = static_cast<size_t>(
          std::floor(static_cast<double>(address) / mAccu[i]));
      pos[i] = mAxes[i].mLowerBound + (0.5 + index_i) * mAxes[i].width();
      address -= index_i * mAccu[i];
    }
    if (inBoundary != nullptr) {
      *inBoundary = true;
    }
  }
  return pos;
}

std::pair<size_t, bool>
HistogramBase::neighbor(const std::vector<double> &position, size_t axisIndex,
                        bool previous) const {
  const double bin_width_i = mAxes[axisIndex].width();
  std::vector<double> pos_next(position);
  if (previous == true) {
    pos_next[axisIndex] -= bin_width_i;
  } else {
    pos_next[axisIndex] += bin_width_i;
  }
  bool inBoundary;
  const size_t addr_neighbor = address(pos_next, &inBoundary);
  return std::make_pair(addr_neighbor, inBoundary);
}

std::pair<size_t, bool> HistogramBase::neighborByAddress(size_t address,
                                                         size_t axisIndex,
                                                         bool previous) const {
  if (address >= mHistogramSize)
    return std::make_pair(0, false);
  std::vector<size_t> index(mNdim, 0);
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
        return std::make_pair(0, false);
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
        return std::make_pair(0, false);
      } else {
        index[axisIndex] += 1;
      }
    }
  }
  size_t neighbour_address = 0;
  for (size_t i = 0; i < mNdim; ++i) {
    neighbour_address += index[i] * mAccu[i];
  }
  return std::make_pair(neighbour_address, true);
}

std::vector<std::pair<size_t, bool>>
HistogramBase::allNeighbor(const std::vector<double> &position) const {
  std::vector<std::pair<size_t, bool>> results(mNdim * 2);
  for (size_t i = 0; i < mNdim; ++i) {
    results[i * 2] = neighbor(position, i, true);
    results[i * 2 + 1] = neighbor(position, i, false);
  }
  return results;
}

std::vector<std::pair<size_t, bool>>
HistogramBase::allNeighborByAddress(size_t address) const {
  std::vector<std::pair<size_t, bool>> results(mNdim * 2);
  for (size_t i = 0; i < mNdim; ++i) {
    results[i * 2] = neighborByAddress(address, i, true);
    results[i * 2 + 1] = neighborByAddress(address, i, false);
  }
  return results;
}

size_t HistogramBase::histogramSize() const { return mHistogramSize; }

size_t HistogramBase::dimension() const { return mNdim; }

std::vector<Axis> HistogramBase::axes() const { return mAxes; }

const std::vector<std::vector<double>> &HistogramBase::pointTable() const {
  return mPointTable;
}

void HistogramBase::fillTable() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  std::vector<std::vector<double>> middlePoint(mNdim);
  for (size_t i = 0; i < mNdim; ++i) {
    middlePoint[i] = mAxes[i].getMiddlePoints();
  }
  mPointTable = std::vector(mNdim, std::vector<double>(mHistogramSize, 0.0));
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
  qDebug() << "Calling" << Q_FUNC_INFO;
}

Axis::Axis(double lowerBound, double upperBound, size_t bins, bool periodic)
    : mLowerBound(lowerBound), mUpperBound(upperBound), mBins(bins),
      mPeriodic(periodic) {
  qDebug() << "Calling" << Q_FUNC_INFO;
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

std::vector<double> Axis::getMiddlePoints() const {
  double tmp = mLowerBound - 0.5 * mWidth;
  std::vector<double> result(mBins, 0.0);
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
  mBins =
      mWidth > 0 ? std::nearbyintl((mUpperBound - newLowerBound) / mWidth) : 0;
  mLowerBound =
      mBins == 0 ? newLowerBound : mUpperBound - double(mBins) * mWidth;
  return mLowerBound;
}

double Axis::setUpperBound(double newUpperBound) {
  // keep bin width and reset upper bound
  mBins =
      mWidth > 0 ? std::nearbyintl((newUpperBound - mLowerBound) / mWidth) : 0;
  mUpperBound =
      mBins == 0 ? newUpperBound : mLowerBound + double(mBins) * mWidth;
  return mUpperBound;
}

double Axis::setWidth(double new_width) {
  if (new_width <= 0)
    return -1.0;
  mBins = std::nearbyintl((mUpperBound - mLowerBound) / new_width);
  mWidth = mBins == 0 ? new_width : (mUpperBound - mLowerBound) / double(mBins);
  return mWidth;
}

double Axis::dist2(double x, double reference) const {
  double dist = x - reference;
  if (!periodic()) {
    return dist * dist;
  } else {
    // wrap the absolute value of dist
    dist = std::abs(dist);
    while (dist > 0.5 * period()) {
      dist -= period();
    }
    return dist * dist;
  }
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

AxisView::AxisView()
    : mColumn(0), mAxis(), mInPMF(false), mReweightingTo(false) {}

HistogramPMF::HistogramPMF() : HistogramBase(), HistogramScalar<double>() {}

HistogramPMF::HistogramPMF(const std::vector<Axis> &ax)
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

HistogramProbability::HistogramProbability()
    : HistogramBase(), HistogramScalar<double>() {}

HistogramProbability::HistogramProbability(const std::vector<Axis> &ax)
    : HistogramBase(ax), HistogramScalar<double>(ax) {}

HistogramProbability::~HistogramProbability() {}

void HistogramProbability::convertToFreeEnergy(double kbt) {
  std::vector<double> f_data(this->data());
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
  const std::vector<double> &p_data = this->data();
  for (size_t i = 0; i < p_data.size(); ++i) {
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

HistogramProbability HistogramProbability::reduceDimension(
    const std::vector<size_t> &new_dims) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  std::vector<Axis> new_ax;
  for (size_t i = 0; i < new_dims.size(); ++i) {
    new_ax.push_back(this->mAxes[new_dims[i]]);
  }
  HistogramProbability new_hist(new_ax);
  std::vector<double> pos(mNdim, 0.0);
  std::vector<double> new_pos(new_hist.dimension(), 0.0);
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

QDebug operator<<(QDebug dbg, const Axis &ax) {
  dbg << "{"
      << "lowerbound:" << ax.lowerBound() << ", upperbound:" << ax.upperBound()
      << ", width:" << ax.width() << ", bin:" << ax.bin() << "}";
  return dbg;
}

HistogramPMFHistory::HistogramPMFHistory() : HistogramBase() {}

HistogramPMFHistory::HistogramPMFHistory(const std::vector<Axis> &ax)
    : HistogramBase(ax) {}

void HistogramPMFHistory::appendHistogram(const std::vector<double> &data) {
  mHistoryData.push_back(data);
}

std::vector<double> HistogramPMFHistory::computeRMSD() const {
  const std::vector<double> &lastFrame = mHistoryData.back();
  return computeRMSD(lastFrame);
}

std::vector<double> HistogramPMFHistory::computeRMSD(
    const std::vector<double> &referenceData) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  std::vector<double> result;
  for (int i = 0; i < mHistoryData.size(); ++i) {
    const std::vector<double> &currentData = mHistoryData[i];
    double rmsd = 0;
    for (size_t j = 0; j < referenceData.size(); ++j) {
      const double diff = referenceData[j] - currentData[j];
      rmsd += diff * diff;
    }
    rmsd /= referenceData.size();
    result.push_back(std::sqrt(rmsd));
  }
  return result;
}

// Does it need threading?
void HistogramPMFHistory::splitToFile(const QString &prefix) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  const int numPMFs = mHistoryData.size();
  const int numDigits = QString::number(numPMFs).size();
  qDebug() << Q_FUNC_INFO << ": number of PMFs = " << numPMFs;
  for (int i = 0; i < numPMFs; ++i) {
    const QString suffix =
        QStringLiteral("%1").arg(i, numDigits, 10, QLatin1Char('0'));
    const QString filename = prefix + "_" + suffix + ".pmf";
    QFile outputFile(filename);
    if (outputFile.open(QIODevice::WriteOnly)) {
      qDebug() << Q_FUNC_INFO << ": writing " << filename;
      std::vector<double> pos(mNdim, 0);
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
        ofs << qSetFieldWidth(OUTPUT_WIDTH) << mHistoryData[i][addr]
            << qSetFieldWidth(0);
        ofs << '\n';
      }
      ofs.flush();
    }
    outputFile.close();
  }
}

PMFPathFinder::PMFPathFinder() { hasData = false; }

PMFPathFinder::PMFPathFinder(const HistogramScalar<double> &histogram,
                             const std::vector<GridDataPatch> &patchList) {
  mHistogram = histogram;
  mPatchList = patchList;
  mHistogramBackup = histogram;
  applyPatch();
  hasData = true;
}

PMFPathFinder::PMFPathFinder(const HistogramScalar<double> &histogram,
                             const std::vector<GridDataPatch> &patchList,
                             const std::vector<double> &pos_start,
                             const std::vector<double> &pos_end,
                             Graph::FindPathMode mode,
                             Graph::FindPathAlgorithm algorithm) {
  setup(histogram, patchList, pos_start, pos_end, mode, algorithm);
}

void PMFPathFinder::setup(const HistogramScalar<double> &histogram,
                          const std::vector<GridDataPatch> &patchList,
                          const std::vector<double> &pos_start,
                          const std::vector<double> &pos_end,
                          Graph::FindPathMode mode,
                          Graph::FindPathAlgorithm algorithm) {
  mHistogram = histogram;
  mPatchList = patchList;
  mPosStart = pos_start;
  mPosEnd = pos_end;
  mHistogramBackup = histogram;
  mMode = mode;
  mAlgorithm = algorithm;
  applyPatch();
  hasData = true;
}

bool PMFPathFinder::initialized() const { return hasData; }

void PMFPathFinder::findPath() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  // setup the graph
  setupGraph();
  // find the starting address and ending address
  bool startOk = false;
  bool endOk = false;
  const size_t start = mHistogram.address(mPosStart, &startOk);
  const size_t end = mHistogram.address(mPosEnd, &endOk);
  // check boundary
  if (startOk && endOk) {
    switch (mAlgorithm) {
    case Graph::FindPathAlgorithm::Dijkstra: {
      mResult = mGraph.Dijkstra(start, end, mMode);
      break;
    }
    case Graph::FindPathAlgorithm::SPFA: {
      mResult = mGraph.SPFA(start, end, mMode);
      break;
    }
    default: {
      mResult = Graph::FindPathResult();
      qDebug() << "Unimplemented algorithm!\n";
    }
    }
  }
}

void PMFPathFinder::writePath(const QString &filename) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  QFile ofs_file(filename);
  if (ofs_file.open(QFile::WriteOnly)) {
    QTextStream out_stream(&ofs_file);
    out_stream.setRealNumberNotation(QTextStream::FixedNotation);
    const auto &path = mResult.mPathNodes;
    for (size_t i = 0; i < path.size(); ++i) {
      const auto pos = mHistogram.reverseAddress(path[i]);
      for (size_t j = 0; j < mHistogram.dimension(); ++j) {
        out_stream << qSetFieldWidth(OUTPUT_WIDTH);
        out_stream.setRealNumberPrecision(OUTPUT_POSITION_PRECISION);
        out_stream << pos[j];
        out_stream << qSetFieldWidth(0) << ' ';
      }
      out_stream << qSetFieldWidth(OUTPUT_WIDTH);
      out_stream.setRealNumberPrecision(OUTPUT_PRECISION);
      out_stream << mHistogram[path[i]];
      out_stream << qSetFieldWidth(0);
      out_stream << '\n';
    }
    out_stream.flush();
  } else {
    qWarning() << "Failed to open file:" << filename;
  }
}

void PMFPathFinder::writeVisitedRegion(const QString &filename) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  QFile ofs_file(filename);
  if (ofs_file.open(QFile::WriteOnly)) {
    QTextStream out_stream(&ofs_file);
    out_stream.setRealNumberNotation(QTextStream::FixedNotation);
    for (size_t i = 0; i < mHistogram.histogramSize(); ++i) {
      if (mResult.mVisitedNodes[i] == true) {
        const auto pos = mHistogram.reverseAddress(i);
        for (size_t j = 0; j < mHistogram.dimension(); ++j) {
          out_stream << qSetFieldWidth(OUTPUT_WIDTH);
          out_stream.setRealNumberPrecision(OUTPUT_POSITION_PRECISION);
          out_stream << pos[j];
          out_stream << qSetFieldWidth(0) << ' ';
        }
        out_stream << qSetFieldWidth(0);
        out_stream << '\n';
      }
    }
    out_stream.flush();
    out_stream.reset();
  } else {
    qWarning() << "Failed to open file:" << filename;
  }
}

void PMFPathFinder::writePatchedPMF(const QString &filename) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  mHistogram.writeToFile(filename);
}

Graph::FindPathResult PMFPathFinder::result() const { return mResult; }

HistogramScalar<double> PMFPathFinder::histogram() const { return mHistogram; }

HistogramScalar<double> PMFPathFinder::histogramBackup() const {
  return mHistogramBackup;
}

void PMFPathFinder::setupGraph() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  QElapsedTimer timer;
  timer.start();
  mGraph = Graph(mHistogram.histogramSize(), true);
  for (size_t i = 0; i < mHistogram.histogramSize(); ++i) {
    const auto allNeighbors = mHistogram.allNeighborByAddress(i);
    for (size_t j = 0; j < allNeighbors.size(); ++j) {
      if (allNeighbors[j].second == true) {
        //        const double& pmf_i = mHistogram[i];
        const double &pmf_j = mHistogram[allNeighbors[j].first];
        //        const double grad_ij =  pmf_j - pmf_i;
        //        const double weight = grad_ij;
        const double weight = pmf_j;
        mGraph.setEdge(i, allNeighbors[j].first, weight);
      }
    }
  }
  qDebug() << "Convert the PMF to a graph takes" << timer.elapsed()
           << "milliseconds.";
  mGraph.summary();
}

void PMFPathFinder::applyPatch() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  mHistogram = mHistogramBackup;
  for (size_t i = 0; i < mHistogram.histogramSize(); ++i) {
    const auto pos = mHistogram.reverseAddress(i);
    for (size_t j = 0; j < mPatchList.size(); ++j) {
      bool in_bound = true;
      for (size_t k = 0; k < mHistogram.dimension(); ++k) {
        const double width = mHistogram.axes()[k].width();
        const size_t num_bins = std::floor(mPatchList[j].mLength[k] / width);
        const double lower_bound =
            mPatchList[j].mCenter[k] - 0.5 * mPatchList[j].mLength[k];
        const double upper_bound =
            mPatchList[j].mCenter[k] + 0.5 * mPatchList[j].mLength[k];
        const Axis current_ax(lower_bound, upper_bound, num_bins, false);
        //        qDebug() << "Construct an axis of" << current_ax;
        if (!current_ax.inBoundary(pos[k])) {
          in_bound = false;
        } else {
          //          qDebug() << pos << "is in the boundary of the patch.";
        }
      }
      if (in_bound) {
        //        qDebug() << "Previous value:" << mHistogram[i];
        mHistogram[i] += mPatchList[j].mValue;
        //        qDebug() << "Updated value:" << mHistogram[i];
      }
    }
  }
}

std::vector<double> PMFPathFinder::posEnd() const { return mPosEnd; }

void PMFPathFinder::setPosEnd(const std::vector<double> &posEnd) {
  mPosEnd = posEnd;
}

std::vector<std::vector<double>> PMFPathFinder::pathPosition() const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  std::vector<std::vector<double>> path_position;
  const auto &path = mResult.mPathNodes;
  for (size_t i = 0; i < path.size(); ++i) {
    const auto pos = mHistogram.reverseAddress(path[i]);
    path_position.push_back(pos);
  }
  return path_position;
}

std::vector<double> PMFPathFinder::pathEnergy() const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  std::vector<double> energy;
  const auto &path = mResult.mPathNodes;
  for (size_t i = 0; i < path.size(); ++i) {
    const double e = mHistogram[path[i]];
    energy.push_back(e);
  }
  return energy;
}

std::vector<double> PMFPathFinder::posStart() const { return mPosStart; }

void PMFPathFinder::setPosStart(const std::vector<double> &posStart) {
  mPosStart = posStart;
}

std::vector<GridDataPatch> PMFPathFinder::patchList() const {
  return mPatchList;
}

void PMFPathFinder::setPatchList(const std::vector<GridDataPatch> &patchList) {
  mPatchList = patchList;
}
