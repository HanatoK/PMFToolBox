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

#include "tablemodelreweightingaxis.h"

TableModelReweightingAxis::TableModelReweightingAxis(QObject *parent)
    : QAbstractTableModel(parent) {}

QVariant TableModelReweightingAxis::headerData(int section,
                                               Qt::Orientation orientation,
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
    case 4:
      return QString{"RW from"};
      break;
    case 5:
      return QString{"RW to"};
      break;
    }
  }
  if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
    return section;
  }
  return QVariant();
}

int TableModelReweightingAxis::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return mAxisList.size();
}

int TableModelReweightingAxis::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  // column 0: column number in trajectory
  // column 1: lower bound
  // column 2: upper bound
  // column 3: width
  // column 4: to be reweighting? (axis in PMF)
  // column 5: reweighting to?
  return 6;
}

QVariant TableModelReweightingAxis::data(const QModelIndex &index,
                                         int role) const {
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
      case 4:
        return boolToString(mAxisList[row_index].mInPMF);
      case 5:
        return boolToString(mAxisList[row_index].mReweightingTo);
      }
    }
  }
  return QVariant();
}

bool TableModelReweightingAxis::insertRows(int position, int rows,
                                           const QModelIndex &index) {
  Q_UNUSED(index);
  beginInsertRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    mAxisList.insert(position, AxisView());
  }

  endInsertRows();
  return true;
}

bool TableModelReweightingAxis::removeRows(int position, int rows,
                                           const QModelIndex &index) {
  Q_UNUSED(index);
  beginRemoveRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    mAxisList.removeAt(position);
  }

  endRemoveRows();
  return true;
}

bool TableModelReweightingAxis::setData(const QModelIndex &index,
                                        const QVariant &value, int role) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (index.isValid() && role == Qt::EditRole) {
    const int row = index.row();
    Axis &ax = mAxisList[row].mAxis;
    switch (index.column()) {
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
    return true;
  }
  return false;
}

Qt::ItemFlags TableModelReweightingAxis::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  const int row_index = index.row();
  const int column_index = index.column();
  const bool is_from_pmf = mAxisList[index.row()].mInPMF;
  if (row_index < 0 || column_index < 0) {
    return QAbstractTableModel::flags(index);
  }
  if (column_index >= 1 && column_index <= 3 && !is_from_pmf) {
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
  } else {
    return QAbstractTableModel::flags(index);
  }
}

void TableModelReweightingAxis::addItem(const Axis &ax, int col, bool inPMF,
                                        bool reweightingTo) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  AxisView tmp;
  tmp.mAxis = ax;
  tmp.mInPMF = inPMF;
  tmp.mReweightingTo = reweightingTo;
  tmp.mColumn = col;
  mAxisList.append(tmp);
  emit layoutChanged();
}

std::vector<int> TableModelReweightingAxis::fromColumns() const {
  std::vector<int> result;
  for (auto it = mAxisList.begin(); it != mAxisList.end(); ++it) {
    if (it->mInPMF)
      result.push_back(it->mColumn);
  }
  return result;
}

std::vector<Axis> TableModelReweightingAxis::targetAxis() const {
  std::vector<Axis> result;
  for (auto it = mAxisList.begin(); it != mAxisList.end(); ++it) {
    if (it->mReweightingTo)
      result.push_back(it->mAxis);
  }
  return result;
}

std::vector<int> TableModelReweightingAxis::toColumns() const {
  std::vector<int> result;
  for (auto it = mAxisList.begin(); it != mAxisList.end(); ++it) {
    if (it->mReweightingTo)
      result.push_back(it->mColumn);
  }
  return result;
}

void TableModelReweightingAxis::clearAll() {
  qDebug() << "Calling" << Q_FUNC_INFO;
  beginResetModel();
  mAxisList.clear();
  endResetModel();
  emit layoutChanged();
}
