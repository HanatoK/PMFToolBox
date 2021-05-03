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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "reweightingtab/reweightingtab.h"
#include "projectpmftab/projectpmftab.h"
#include "historypmftab/historypmftab.h"
#include "namdlogtab/namdlogtab.h"
#include "findpathtab/findpathtab.h"
#include "metadynamicstab/metadynamicstab.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  void resizeEvent(QResizeEvent* event) override;
//  bool event(QEvent* event) override;

public slots:
  void updateSizes(int index);
  void openAboutDialog();

private:
  Ui::MainWindow *ui;
  ReweightingTab *mReweightingTab;
  ProjectPMFTab *mProjectPMFTab;
  HistoryPMFTab *mHistoryPMFTab;
  NAMDLogTab *mNAMDLogTab;
  FindPathTab *mFindPathTab;
  MetadynamicsTab *mMetadynamicsTab;
};
#endif // MAINWINDOW_H
