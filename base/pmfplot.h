#ifndef PMFPLOT_H
#define PMFPLOT_H

#include "base/histogram.h"

#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_text.h>
#include <QFont>

class PMFPlot : public QwtPlot
{
public:
  PMFPlot(QWidget *parent = nullptr);
  PMFPlot(const QwtText &title, QWidget *parent = nullptr);
  bool plotPMF2D(const HistogramScalar<double>& histogram);
protected:
  virtual void initialize();
private:
  // fonts for plotting
  QFont mTitleFont;
  QFont mPlotFont;
  QFont mColorbarFont;
};

#endif // PMFPLOT_H
