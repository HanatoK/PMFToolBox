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

#include "tablemodelbinning.h"

TableModelBinning::TableModelBinning(QObject *parent)
    : QAbstractTableModel(parent) {}

QVariant TableModelBinning::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return QString{"Column"};
      break;
    case 1:
      return QString{"Lower bound"};
      break;
    case 2:
      return QString{"Upper bound"};
      break;
    case 3:
      return QString{"Width"};
      break;
    }
  }
  if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
    return section;
  }
  return QVariant();
}

int TableModelBinning::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return mAxisList.size();
}

int TableModelBinning::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  // column 0: column number in trajectory
  // column 1: lower bound
  // column 2: upper bound
  // column 3: width
  return 4;
}

QVariant TableModelBinning::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    const int row_index = index.row();
    const int column_index = index.column();
    if (row_index >= 0 && column_index >= 0) {
      switch (column_index) {
      case 0:
        return mAxisList[row_index].mColumn;
        break;
      case 1:
        return mAxisList[row_index].mAxis.lowerBound();
        break;
      case 2:
        return mAxisList[row_index].mAxis.upperBound();
        break;
      case 3:
        return mAxisList[row_index].mAxis.width();
        break;
      }
    }
  }
  return QVariant();
}

bool TableModelBinning::insertRows(int position, int rows,
                                   const QModelIndex &index) {
  qDebug() << Q_FUNC_INFO;
  qDebug() << "insert " << rows << " row at position " << position;
  position = qBound(0, position, position);
  beginInsertRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    mAxisList.insert(position, AxisView());
  }
  endInsertRows();
  return true;
}

bool TableModelBinning::removeRows(int position, int rows,
                                   const QModelIndex &index) {
  qDebug() << Q_FUNC_INFO;
  qDebug() << "insert " << rows << " row at position " << position;
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    if (position >= 0 && position < mAxisList.size())
      mAxisList.removeAt(position);
  }
  endRemoveRows();
  return true;
}

bool TableModelBinning::setData(const QModelIndex &index, const QVariant &value,
                                int role) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (index.isValid() && role == Qt::EditRole) {
    const int row = index.row();
    Axis &ax = mAxisList[row].mAxis;
    switch (index.column()) {
    // FIXME: actually unable to setup the lower, upper and width
    case 0: {
      qDebug() << "Change the column of axis " << row << " from "
               << mAxisList[row].mColumn << " to " << value;
      mAxisList[row].mColumn = value.toInt();
      break;
    }
    case 1: {
      const double newLowerBound = ax.setLowerBound(value.toDouble());
      qDebug() << "New lower bound of axis " << index.row() << " is changed to "
               << newLowerBound;
      break;
    }
    case 2: {
      const double newUpperBound = ax.setUpperBound(value.toDouble());
      qDebug() << "New upper bound of axis " << index.row() << " is changed to "
               << newUpperBound;
      break;
    }
    case 3: {
      const double newWidth = ax.setWidth(value.toDouble());
      qDebug() << "New width of axis " << index.row() << " is changed to "
               << newWidth;
      break;
    }
    }
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    qDebug() << "Axis " << row << " is : " << ax;
    return true;
  }
  return false;
}

Qt::ItemFlags TableModelBinning::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;
  return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

std::vector<int> TableModelBinning::fromColumns() const {
  std::vector<int> result;
  for (const auto &i : mAxisList) {
    result.push_back(i.mColumn);
  }
  return result;
}

std::vector<Axis> TableModelBinning::targetAxis() const {
  std::vector<Axis> result;
  for (const auto &i : mAxisList) {
    result.push_back(i.mAxis);
  }
  return result;
}

void TableModelBinning::clearAll() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  beginResetModel();
  mAxisList.clear();
  endResetModel();
  emit layoutChanged();
}
