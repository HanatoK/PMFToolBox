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

#ifndef PROJECTPMF_H
#define PROJECTPMF_H

#include "base/plot.h"

#include <QWidget>

namespace Ui {
class ProjectPMFTab;
}

// the computation here is not intensive so we do not need another thread
class ProjectPMFTab : public QWidget
{
  Q_OBJECT

public slots:
  void loadPMF();
  void saveFile();
  void projectPMF();
  void plotOriginPMF();
  void plotProjectedPMF();

public:
  explicit ProjectPMFTab(QWidget *parent = nullptr);
  ~ProjectPMFTab();

private:
  Ui::ProjectPMFTab *ui;
  HistogramPMF mOriginPMF;
  HistogramPMF mProjectedPMF;
};

#endif // PROJECTPMF_H
