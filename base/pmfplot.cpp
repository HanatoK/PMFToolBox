#include "pmfplot.h"
#include "base/turbocolormap.h"

#include <qwt_scale_widget.h>
#include <qwt_matrix_raster_data.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_curve.h>

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
  qDebug() << "Calling " << Q_FUNC_INFO;
  if (histogram.dimension() != 2) return false;
  detachItems();
  // setup axis
  enableAxis(QwtPlot::Axis::yLeft, true);
  enableAxis(QwtPlot::Axis::xBottom, true);
  axisWidget(QwtPlot::Axis::yLeft)->setFont(mPlotFont);
  axisWidget(QwtPlot::Axis::xBottom)->setFont(mPlotFont);
  // prepare data
  QwtMatrixRasterData *matrix;
  matrix = new QwtMatrixRasterData();
  const size_t numXbins = histogram.axes()[0].bin();
  const size_t numYbins = histogram.axes()[1].bin();
  qDebug() << "X bins = " << numXbins << " ; Y bins = " << numYbins;
  // from Qt 5.14, range constructor
  const QVector<double>& zData = histogram.data();
  // setup the scales of x,y axes
  setAxisScale(QwtPlot::xBottom, histogram.axes()[0].lowerBound(),
               histogram.axes()[0].upperBound());
  setAxisScale(QwtPlot::yLeft, histogram.axes()[1].lowerBound(),
               histogram.axes()[1].upperBound());
  // setup xyz interval
  const double zMin = *std::min_element(zData.begin(), zData.end());
  const double zMax = *std::max_element(zData.begin(), zData.end());
  matrix->setValueMatrix(zData, numXbins);
  matrix->setInterval(Qt::XAxis,
                      QwtInterval(histogram.axes()[0].lowerBound(),
                                  histogram.axes()[0].upperBound()));
  matrix->setInterval(Qt::YAxis,
                      QwtInterval(histogram.axes()[1].lowerBound(),
                                  histogram.axes()[1].upperBound()));
  matrix->setInterval(Qt::ZAxis, QwtInterval(zMin, zMax));
  // create contour plot
  QwtPlotSpectrogram *image2D = new QwtPlotSpectrogram("image");
  image2D->setColorMap(new TurboColorMap());
  image2D->setRenderThreadCount(0);
  image2D->setData(matrix);
  image2D->attach(this);
  const QwtInterval zInterval = image2D->data()->interval(Qt::ZAxis);
  // colorbar
  QwtScaleWidget *rightAxis = axisWidget(QwtPlot::yRight);
  rightAxis->setFont(mColorbarFont);
  QwtText free_energy_title("ΔG (kcal/mol)");
  free_energy_title.setFont(mTitleFont);
  rightAxis->setTitle(free_energy_title);
  rightAxis->setColorBarEnabled(true);
  rightAxis->setColorMap(zInterval, new TurboColorMap());
  qDebug() << "zInterval.minValue() = " << zInterval.minValue()
           << " ; zInterval.maxValue() = " << zInterval.maxValue();
  setAxisScale(QwtPlot::yRight, zInterval.minValue(),
                     zInterval.maxValue());
  enableAxis(QwtPlot::yRight);
  plotLayout()->setAlignCanvasToScales(true);
  replot();
  return true;
}

bool PMFPlot::plotPMF1D(const HistogramScalar<double> &histogram)
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  if (histogram.dimension() != 1) return false;
  detachItems();
  enableAxis(QwtPlot::Axis::yLeft, true);
  enableAxis(QwtPlot::Axis::xBottom, true);
  enableAxis(QwtPlot::Axis::yRight, false);
  enableAxis(QwtPlot::Axis::xTop, false);
  setAxisScale(QwtPlot::xBottom, histogram.axes()[0].lowerBound(),
                     histogram.axes()[0].upperBound());
  const double yMin =
      *std::min_element(histogram.data().begin(), histogram.data().end());
  const double yMax =
      *std::max_element(histogram.data().begin(), histogram.data().end());
  setAxisScale(QwtPlot::yLeft, yMin, yMax);
  axisWidget(QwtPlot::Axis::yLeft)->setFont(mPlotFont);
  axisWidget(QwtPlot::Axis::xBottom)->setFont(mPlotFont);
  QwtPlotCurve *curve = new QwtPlotCurve("curve");
  curve->setPen(Qt::red, 2);
  curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  QPolygonF points;
  const QVector<QVector<double>> &point_table = histogram.pointTable();
  QVector<double> pos(1, 0);
  for (size_t i = 0; i < histogram.histogramSize(); ++i) {
    pos[0] = point_table[0][i];
    const auto val = histogram(pos);
    points.append(QPointF(pos[0], val));
  }
  curve->setSamples(points);
  curve->attach(this);
  replot();
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
