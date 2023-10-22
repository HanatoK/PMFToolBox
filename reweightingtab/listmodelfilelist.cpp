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

#include "listmodelfilelist.h"

#include <QDebug>

ListModelFileList::ListModelFileList(QObject *parent)
    : QAbstractListModel(parent) {}

QVariant ListModelFileList::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return QString{"Trajectory file(s)"};
      break;
    }
  }
  if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
    return section;
  }
  return QVariant();
}

int ListModelFileList::rowCount(const QModelIndex &parent) const {
  // For list models only the root node (an invalid parent) should return the
  // list's size. For all other (valid) parents, rowCount() should return 0 so
  // that it does not become a tree model.
  if (parent.isValid())
    return 0;

  return mFileNameList.size();
}

QVariant ListModelFileList::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    return mFileNameList[index.row()];
  }
  return QVariant();
}

bool ListModelFileList::insertRows(int row, int count,
                                   const QModelIndex &parent) {
  row = row < 0 ? 0 : row;
  beginInsertRows(parent, row, row + count - 1);
//  mFileNameList.insert(row, count, "");
  for (int i = 0; i < count; ++i) mFileNameList.insert(row, "");
  endInsertRows();
  return true;
}

bool ListModelFileList::removeRows(int row, int count,
                                   const QModelIndex &parent) {
  beginRemoveRows(parent, row, row + count - 1);
  if (row >= 0 && row < mFileNameList.size()) {
    if (row + count > mFileNameList.size()) {
      count = mFileNameList.size() - row;
    }
    mFileNameList.erase(mFileNameList.begin() + row, mFileNameList.begin() + row + count);
  }
  endRemoveRows();
  return true;
}

void ListModelFileList::addItem(const QString &name,
                                const QModelIndex &currentIndex) {
  qDebug() << Q_FUNC_INFO;
  insertRows(currentIndex.row(), 1);
  const int row = currentIndex.row() < 0 ? 0 : currentIndex.row();
  mFileNameList[row] = name;
  emit layoutChanged();
  dumpList();
}

void ListModelFileList::addItems(const QStringList &names,
                                 const QModelIndex &currentIndex) {
  qDebug() << Q_FUNC_INFO;
  const int numRows = names.length();
  insertRows(currentIndex.row(), numRows);
  const int row = currentIndex.row() < 0 ? 0 : currentIndex.row();
  for (int i = 0; i < numRows; ++i) {
    mFileNameList[row + i] = names[i];
  }
  emit layoutChanged();
  dumpList();
}

void ListModelFileList::removeItem(const QModelIndex &currentIndex) {
  qDebug() << Q_FUNC_INFO;
  removeRows(currentIndex.row(), 1);
  emit layoutChanged();
}

void ListModelFileList::clearAll() {
  qDebug() << Q_FUNC_INFO;
  if (!mFileNameList.empty()) {
    removeRows(0, mFileNameList.size());
  }
  emit layoutChanged();
  dumpList();
}

QStringList ListModelFileList::trajectoryFileNameList() const {
  return mFileNameList;
}

// for debug
void ListModelFileList::dumpList() const {
  qDebug() << "Current file name list:";
  qDebug() << mFileNameList;
}
