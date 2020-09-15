#include "pmfplot.h"

PMFPlot::PMFPlot(QWidget *parent): QwtPlot(parent)
{
  initialize();
}

PMFPlot::PMFPlot(const QwtText &title, QWidget *parent): QwtPlot(title, parent)
{
  initialize();
}

bool PMFPlot::plotPMF2D(const HistogramScalar<double> &histogram)
{
  detachItems();
  enableAxis(QwtPlot::Axis::yLeft, true);
  enableAxis(QwtPlot::Axis::xBottom, true);
  enableAxis(QwtPlot::Axis::yRight, false);
  enableAxis(QwtPlot::Axis::xTop, false);
  setAxisScale(QwtPlot::xBottom, histogram.axes()[0].lowerBound(),
               histogram.axes()[0].upperBound());
  return true;
}

void PMFPlot::initialize()
{
  setCanvasBackground(Qt::white);
  enableAxis(QwtPlot::Axis::yLeft, false);
  enableAxis(QwtPlot::Axis::xBottom, false);
  setAutoReplot(true);
  const QStringList font_list{"Arimo", "Liberation Sans", "Helvetica", "Arial",
                              "Sans Serif"};
  mPlotFont.setFamilies(font_list);
  mPlotFont.setPointSize(16);
  mTitleFont.setFamilies(font_list);
  mTitleFont.setPointSize(18);
  mColorbarFont.setFamilies(font_list);
  mColorbarFont.setPointSize(14);
}
