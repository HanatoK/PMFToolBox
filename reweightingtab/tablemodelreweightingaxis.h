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

#ifndef REWEIGHTINGAXISTABLEMODEL_H
#define REWEIGHTINGAXISTABLEMODEL_H

#include "base/histogram.h"

#include <QAbstractTableModel>
#include <QObject>

class TableModelReweightingAxis : public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit TableModelReweightingAxis(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  void addItem(const Axis& ax, int col, bool inPMF, bool reweightingTo);
  std::vector<int> fromColumns() const;
  std::vector<int> toColumns() const;
  std::vector<Axis> targetAxis() const;
  void clearAll();
private:
  QList<AxisView> mAxisList;
};

#endif // REWEIGHTINGAXISTABLEMODEL_H
