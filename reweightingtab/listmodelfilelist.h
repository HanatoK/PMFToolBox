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

#ifndef LISTMODELTRAJECTORY_H
#define LISTMODELTRAJECTORY_H

#include <QAbstractListModel>

class ListModelFileList : public QAbstractListModel {
  Q_OBJECT

public:
  explicit ListModelFileList(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool insertRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  void addItem(const QString &name, const QModelIndex &currentIndex);
  void addItems(const QStringList &names, const QModelIndex &currentIndex);
  void removeItem(const QModelIndex &currentIndex);
  void clearAll();
  QStringList trajectoryFileNameList() const;
  void dumpList() const;

private:
  QStringList mFileNameList;
};

#endif // LISTMODELTRAJECTORY_H
