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
  void PlotRMSD(const std::vector<double>& rmsd);
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
  // TODO: unify the behavior of clearing previous figure
  bool plotPMF2D(const HistogramScalar<double>& histogram);
  bool plotPMF1D(const HistogramScalar<double>& histogram);
  void plotPath2D(const std::vector<std::vector<double> > &pathPositions, bool clearFigure = false);
  void plotEnergyAlongPath(const std::vector<double>& energies, bool clearFigure = false);
protected:
  virtual void initialize();
  // fonts for plotting
  QFont mTitleFont;
  QFont mPlotFont;
  QFont mColorbarFont;
};

#endif // PMFPLOT_H
