#include "base/integrate_gradients.h"
//#include <eigen3/Eigen/IterativeLinearSolvers>
#include <fstream>
#include <set>

HistogramGradient::HistogramGradient(): HistogramVector<double>()
{

}

HistogramGradient::HistogramGradient(const std::vector<Axis>& ax, const size_t mult):
  HistogramVector<double>(ax, mult)
{
  if (ax.size() != mult) {
    throw std::runtime_error("The multiplicity does not match the number of axis.");
  }
}

HistogramGradient::~HistogramGradient()
{

}

HistogramScalar<double> HistogramGradient::divergence() const
{
  HistogramScalar<double> result(mAxes);
  // TODO: this is inefficient
  std::vector<double> pos(mNdim);
  for (size_t i = 0; i < mHistogramSize; ++i) {
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = mPointTable[j][i];
    }
    const double div = divergence(pos);
    const size_t addr = address(pos);
    result[addr] = div;
  }
  return result;
}

void HistogramGradient::buildFiniteDifferenceMatrix()
{
  // mData stores the gradients of N dimensions
  mFiniteDifferenceMatrix = arma::mat(mPotentialHistogram.histogramSize(), mPotentialHistogram.histogramSize(), arma::fill::zeros);
  mDivergenceVector = arma::vec(mPotentialHistogram.histogramSize(), arma::fill::zeros);
  auto solution = mDivergenceVector;
  // iterate over all points
  std::vector<double> pos(mNdim);
  const auto& potential_point_table = mPotentialHistogram.pointTable();
  for (size_t i = 0; i < mPotentialHistogram.histogramSize(); ++i) {
    solution(i) = 0;
    for (size_t j = 0; j < mNdim; ++j) {
      pos[j] = potential_point_table[j][i];
    }
    const double div = divergence(pos);
    const std::vector<size_t> id = index(pos);
    const size_t addr = address(id);
    mDivergenceVector(addr) += div;
//    std::cout << "Addr = " << addr << "; pos[0] = " << pos[0] << " ; pos[1] = " << pos[1] << std::endl;
    double div_scale = 1.0;
    std::set<size_t> modified_index;
    modified_index.insert(addr);
    for (size_t j = 0; j < mNdim; ++j) {
      const auto [neighbor_addr_prev, in_bound_prev] = neighborByIndex(id, j, true);
      const auto [neighbor_addr_next, in_bound_next] = neighborByIndex(id, j, false);
      const auto factor = 1.0 / (mAxes[j].width() * mAxes[j].width());
      if (in_bound_prev) {
        mFiniteDifferenceMatrix(addr, neighbor_addr_prev) += 1.0 * factor;
        mFiniteDifferenceMatrix(addr, addr) += -1.0 * factor;
        modified_index.insert(neighbor_addr_prev);
      } else {
        const double& current_grad = mData[addr * mNdim + j];
        mFiniteDifferenceMatrix(addr, addr) += -1.0 * factor;
        mFiniteDifferenceMatrix(addr, neighbor_addr_next) += 1.0 * factor;
        mDivergenceVector(addr) += 2.0 * current_grad / mAxes[j].width();
        div_scale *= 0.5;
        modified_index.insert(neighbor_addr_next);
      }
      if (in_bound_next) {
        mFiniteDifferenceMatrix(addr, neighbor_addr_next) += 1.0 * factor;
        mFiniteDifferenceMatrix(addr, addr) += -1.0 * factor;
        modified_index.insert(neighbor_addr_next);
      } else {
        const double& current_grad = mData[addr * mNdim + j];
        mFiniteDifferenceMatrix(addr, addr) += -1.0 * factor;
        mFiniteDifferenceMatrix(addr, neighbor_addr_prev) += 1.0 * factor;
        mDivergenceVector(addr) += -2.0 * current_grad / mAxes[j].width();
        div_scale *= 0.5;
        modified_index.insert(neighbor_addr_prev);
      }
    }
    mDivergenceVector(addr) *= div_scale;
    for (auto& j : modified_index) {
      mFiniteDifferenceMatrix(addr, j) *= div_scale;
    }
  }
  // for debug
  std::ofstream ofs_A("matrix_A.dat");
  ofs_A << mFiniteDifferenceMatrix;
  ofs_A.close();
  std::ofstream ofs_b("vector_B.dat");
  ofs_b << mDivergenceVector;
  ofs_b.close();
  // solve
//  Eigen::ConjugateGradient<Eigen::MatrixXd, Eigen::Lower|Eigen::Upper> cg;
//  cg.compute(mFiniteDifferenceMatrix);
//  solution = cg.solve(mDivergenceVector);
//  std::cout << "Iteration " << cg.iterations() << " , error = " << cg.error() << std::endl;
//  solution = mFiniteDifferenceMatrix.bdcSvd().solve(mDivergenceVector);
  solution = arma::solve(mFiniteDifferenceMatrix, mDivergenceVector, arma::solve_opts::refine	+ arma::solve_opts::likely_sympd);
  const double min_val = solution.min();
  for (size_t i = 0; i < mPotentialHistogram.histogramSize(); ++i) {
    mPotentialHistogram[i] = solution(i) - min_val;
  }
}

HistogramScalar<double> HistogramGradient::potentialHistogram() const
{
  return mPotentialHistogram;
}

bool HistogramGradient::readFromFile(const QString& filename)
{
  qDebug() << "Calling" << Q_FUNC_INFO;
  const bool ok = HistogramVector<double>::readFromFile(filename);
  initPotentialHistogram();
  return ok;
}

double HistogramGradient::divergence(const std::vector<double>& pos) const
{
  std::vector<double> grad_deriv(mNdim, 0.0);
  if (!isInGrid(pos)) {
    return 0.0;
  }
  const size_t force_addr = address(pos) * mNdim;
//  std::vector<double> grad = this->operator()(pos);
  for (size_t i = 0; i < mNdim; ++i) {
    const double binWidth = mAxes[i].width();
    std::vector<double> first = pos;
    std::vector<double> last = pos;
    first[i] = mAxes[i].lowerBound() + binWidth * 0.5;
    last[i] = mAxes[i].upperBound() - binWidth * 0.5;
    const size_t addr_first = address(first) * mNdim;
    const size_t addr_last = address(last) * mNdim;
    if (force_addr == addr_first) {
      std::vector<double> next(pos);
      next[i] += binWidth;
      const size_t addr_next = address(next) * mNdim;
      if (mAxes[i].realPeriodic() == true) {
        grad_deriv[i] = (mData[addr_next + i] - mData[addr_last + i]) / (2.0 * binWidth);
      } else {
        std::vector<double> next_2(next);
        next_2[i] += binWidth;
        const size_t addr_next_2 = address(next_2) * mNdim;
        grad_deriv[i] =
          (mData[addr_next_2 + i] * -1.0 + mData[addr_next + i] * 4.0 - mData[force_addr + i] * 3.0) /
          (2.0 * binWidth);
//        grad_deriv[i] = (mData[addr_next + i] - 0.0) / (2.0 * binWidth);
      }
    } else if (force_addr == addr_last) {
      std::vector<double> prev(pos);
      prev[i] -= binWidth;
      const size_t addr_prev = address(prev) * mNdim;
      if (mAxes[i].realPeriodic() == true) {
        grad_deriv[i] = (mData[addr_first + i] - mData[addr_prev + i]) / (2.0 * binWidth);
      } else {
        std::vector<double> prev_2(prev);
        prev_2[i] -= binWidth;
        const size_t addr_prev_2 = address(prev_2) * mNdim;
        grad_deriv[i] =
          (mData[force_addr + i] * 3.0 - mData[addr_prev + i] * 4.0 + mData[addr_prev_2 + i] * 1.0) /
          (2.0 * binWidth);
//        grad_deriv[i] = (0.0 - mData[addr_prev + i]) / (2.0 * binWidth);
      }
    } else {
      std::vector<double> prev(pos);
      prev[i] -= binWidth;
      std::vector<double> next(pos);
      next[i] += binWidth;
      const size_t addr_prev = address(prev) * mNdim;
      const size_t addr_next = address(next) * mNdim;
      grad_deriv[i] = (mData[addr_next + i] - mData[addr_prev + i]) / (2.0 * binWidth);
    }
  }
  return std::accumulate(begin(grad_deriv), end(grad_deriv), 0.0);;
}

void HistogramGradient::initPotentialHistogram()
{
//  std::vector<Axis> ax_potential(mAxes.size());
//  for (size_t i = 0; i < mAxes.size(); ++i) {
//    ax_potential[i] = Axis(mAxes[i].lowerBound() - 0.5 * mAxes[i].width(),
//                           mAxes[i].upperBound() + 0.5 * mAxes[i].width(),
//                           mAxes[i].bin() + 1, mAxes[i].periodic());
//  }
  mPotentialHistogram = HistogramScalar<double>(mAxes);
}

