#ifndef INTEGRATE_GRADIENTS_H
#define INTEGRATE_GRADIENTS_H

#include "base/histogram.h"
#include <armadillo>

class HistogramGradient: public HistogramVector<double> {
public:
  HistogramGradient();
  HistogramGradient(const std::vector<Axis> &ax, const size_t mult);
  virtual ~HistogramGradient();
  HistogramScalar<double> divergence() const;
  void buildFiniteDifferenceMatrix();
  HistogramScalar<double> potentialHistogram() const;
  virtual bool readFromFile(const QString &filename) override;
protected:
  double divergence(const std::vector<double>& pos) const;
private:
  void initPotentialHistogram();
  HistogramScalar<double> mPotentialHistogram;
  arma::mat mFiniteDifferenceMatrix;
  arma::vec mDivergenceVector;
};

#endif // INTEGRATE_GRADIENTS_H
