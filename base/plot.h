#ifndef PMFPLOT_H
#define PMFPLOT_H

#include "base/histogram.h"

#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_text.h>
#include <QFont>

// these two classes should be derived from the same base class
class RMSDPlot: public QwtPlot
{
public:
  RMSDPlot(QWidget *parent = nullptr);
  RMSDPlot(const QwtText &title, QWidget *parent = nullptr);
  void PlotRMSD(const QVector<double>& rmsd);
  virtual ~RMSDPlot();
protected:
  virtual void initialize();
  // fonts for plotting
  QFont mTitleFont;
  QFont mPlotFont;
};

class PMFPlot : public QwtPlot
{
public:
  PMFPlot(QWidget *parent = nullptr);
  PMFPlot(const QwtText &title, QWidget *parent = nullptr);
  virtual ~PMFPlot();
  bool plotPMF2D(const HistogramScalar<double>& histogram);
  bool plotPMF1D(const HistogramScalar<double>& histogram);
protected:
  virtual void initialize();
  // fonts for plotting
  QFont mTitleFont;
  QFont mPlotFont;
  QFont mColorbarFont;
};

#endif // PMFPLOT_H