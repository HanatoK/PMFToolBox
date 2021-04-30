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

#include "plot.h"
#include "base/turbocolormap.h"

#include <qwt_matrix_raster_data.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_scale_widget.h>

PMFPlot::PMFPlot(QWidget *parent) : QwtPlot(parent) { initialize(); }

PMFPlot::PMFPlot(const QwtText &title, QWidget *parent)
    : QwtPlot(title, parent) {
  initialize();
}

PMFPlot::~PMFPlot() {}

bool PMFPlot::plotPMF2D(const HistogramScalar<double> &histogram) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (histogram.dimension() != 2)
    return false;
  detachItems();
  // setup axis
  enableAxis(QwtPlot::Axis::yLeft, true);
  enableAxis(QwtPlot::Axis::xBottom, true);
  axisWidget(QwtPlot::Axis::yLeft)->setFont(mPlotFont);
  axisWidget(QwtPlot::Axis::xBottom)->setFont(mPlotFont);
  setAxisTitle(QwtPlot::Axis::yLeft, "Y");
  setAxisTitle(QwtPlot::Axis::xBottom, "X");
  // prepare data
  QwtMatrixRasterData *matrix;
  matrix = new QwtMatrixRasterData();
  const size_t numXbins = histogram.axes()[0].bin();
  const size_t numYbins = histogram.axes()[1].bin();
  qDebug() << "X bins = " << numXbins << " ; Y bins = " << numYbins;
  // from Qt 5.14, range constructor
  const QVector<double> &zData{histogram.data().begin(),
                               histogram.data().end()};
  // setup the scales of x,y axes
  setAxisScale(QwtPlot::xBottom, histogram.axes()[0].lowerBound(),
               histogram.axes()[0].upperBound());
  setAxisScale(QwtPlot::yLeft, histogram.axes()[1].lowerBound(),
               histogram.axes()[1].upperBound());
  // setup xyz interval
  const double zMin = *std::min_element(zData.begin(), zData.end());
  const double zMax = *std::max_element(zData.begin(), zData.end());
  matrix->setValueMatrix(zData, numXbins);
  matrix->setInterval(Qt::XAxis, QwtInterval(histogram.axes()[0].lowerBound(),
                                             histogram.axes()[0].upperBound()));
  matrix->setInterval(Qt::YAxis, QwtInterval(histogram.axes()[1].lowerBound(),
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
  setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());
  enableAxis(QwtPlot::yRight);
  plotLayout()->setAlignCanvasToScales(true);
  replot();
  return true;
}

bool PMFPlot::plotPMF1D(const HistogramScalar<double> &histogram) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (histogram.dimension() != 1)
    return false;
  detachItems();
  enableAxis(QwtPlot::Axis::yLeft, true);
  enableAxis(QwtPlot::Axis::xBottom, true);
  enableAxis(QwtPlot::Axis::yRight, false);
  enableAxis(QwtPlot::Axis::xTop, false);
  QwtText free_energy_title("ΔG (kcal/mol)");
  free_energy_title.setFont(mTitleFont);
  setAxisTitle(QwtPlot::Axis::yLeft, free_energy_title);
  setAxisTitle(QwtPlot::Axis::xBottom, "X");
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
  const std::vector<std::vector<double>> &point_table = histogram.pointTable();
  std::vector<double> pos(1, 0);
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

void PMFPlot::plotPath2D(const std::vector<std::vector<double>> &pathPositions,
                         bool clearFigure) {
  if (clearFigure) {
    detachItems();
  }
  QwtPlotCurve *curve = new QwtPlotCurve("curve");
  curve->setPen(Qt::black, 2);
  curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  QPolygonF points;
  for (const auto &point : pathPositions) {
    points.append(QPointF(point[0], point[1]));
  }
  curve->setSamples(points);
  curve->setStyle(QwtPlotCurve::Dots);
  curve->attach(this);
  replot();
}

void PMFPlot::plotEnergyAlongPath(const std::vector<double> &energies,
                                  bool clearFigure) {
  const double xMin = 0.0;
  const double xMax = 1.0;
  const double yMin = *std::min_element(energies.begin(), energies.end());
  const double yMax = *std::max_element(energies.begin(), energies.end());
  if (clearFigure) {
    detachItems();
    enableAxis(QwtPlot::Axis::yLeft, true);
    enableAxis(QwtPlot::Axis::xBottom, true);
    enableAxis(QwtPlot::Axis::yRight, false);
    enableAxis(QwtPlot::Axis::xTop, false);
    setAxisScale(QwtPlot::xBottom, xMin, xMax);
    setAxisScale(QwtPlot::yLeft, yMin, yMax);
  }
  axisWidget(QwtPlot::Axis::yLeft)->setFont(mPlotFont);
  axisWidget(QwtPlot::Axis::xBottom)->setFont(mPlotFont);
  QwtText free_energy_title("ΔG (kcal/mol)");
  setAxisTitle(QwtPlot::Axis::yLeft, free_energy_title);
  setAxisTitle(QwtPlot::Axis::xBottom, "S");
  QwtPlotCurve *curve = new QwtPlotCurve("energyCurve");
  curve->setPen(Qt::red, 2);
  curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  QPolygonF points;
  const double stepSize = (xMax - xMin) / (energies.size() - 1);
  for (size_t i = 0; i < energies.size(); ++i) {
    points.append(QPointF(xMin + i * stepSize, energies[i]));
  }
  curve->setSamples(points);
  curve->attach(this);
  replot();
}

void PMFPlot::initialize() {
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

RMSDPlot::RMSDPlot(QWidget *parent) : QwtPlot(parent) { initialize(); }

RMSDPlot::RMSDPlot(const QwtText &title, QWidget *parent)
    : QwtPlot(title, parent) {
  qDebug() << "Calling" << Q_FUNC_INFO;
}

void RMSDPlot::PlotRMSD(const std::vector<double> &rmsd) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  detachItems();
  enableAxis(QwtPlot::Axis::yLeft, true);
  enableAxis(QwtPlot::Axis::xBottom, true);
  enableAxis(QwtPlot::Axis::yRight, false);
  enableAxis(QwtPlot::Axis::xTop, false);
  const int lowerBound = 0;
  const int upperBound = rmsd.size() - 1;
  setAxisScale(QwtPlot::xBottom, lowerBound, upperBound);
  const double yMin = *std::min_element(rmsd.begin(), rmsd.end());
  const double yMax = *std::max_element(rmsd.begin(), rmsd.end());
  setAxisScale(QwtPlot::yLeft, yMin, yMax);
  axisWidget(QwtPlot::Axis::yLeft)->setFont(mPlotFont);
  axisWidget(QwtPlot::Axis::xBottom)->setFont(mPlotFont);
  QwtPlotCurve *curve = new QwtPlotCurve("curve");
  curve->setPen(Qt::red, 2);
  curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  QPolygonF points;
  for (size_t i = 0; i < rmsd.size(); ++i) {
    points.append(QPointF(i, rmsd[i]));
  }
  curve->setSamples(points);
  curve->attach(this);
  replot();
}

RMSDPlot::~RMSDPlot() {}

void RMSDPlot::initialize() {
  setCanvasBackground(Qt::white);
  enableAxis(QwtPlot::Axis::yLeft, false);
  enableAxis(QwtPlot::Axis::xBottom, false);
  setAutoReplot(true);
  const QStringList font_list{"Liberation Sans", "Helvetica", "Arial",
                              "Sans Serif"};
  mPlotFont.setFamilies(font_list);
  mPlotFont.setPointSize(16);
  mTitleFont.setFamilies(font_list);
  mTitleFont.setPointSize(18);
}
