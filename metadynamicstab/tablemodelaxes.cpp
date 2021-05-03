#include "tablemodelaxes.h"

TableModelAxes::TableModelAxes(QObject *parent) : QAbstractTableModel(parent) {}

QVariant TableModelAxes::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return QString{"Lower bound"};
      break;
    case 1:
      return QString{"Upper bound"};
      break;
    case 2:
      return QString{"Width"};
      break;
    case 3:
      return QString{"Periodic"};
      break;
    }
  }
  if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
    return section;
  }
  return QVariant();
}

bool TableModelAxes::setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant &value, int role) {
  if (value != headerData(section, orientation, role)) {
    // FIXME: Implement me!
    emit headerDataChanged(orientation, section, section);
    return true;
  }
  return false;
}

int TableModelAxes::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return mAxisList.size();
}

int TableModelAxes::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return 4;
}

QVariant TableModelAxes::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    const int row_index = index.row();
    const int column_index = index.column();
    if (row_index >= 0 && column_index >= 0) {
      switch (column_index) {
      case 0:
        return mAxisList[row_index].mAxis.lowerBound();
        break;
      case 1:
        return mAxisList[row_index].mAxis.upperBound();
        break;
      case 2:
        return mAxisList[row_index].mAxis.width();
        break;
      case 3:
        if (mAxisList[row_index].mAxis.periodic()) {
          return QString("true");
        } else {
          return QString("false");
        }
        break;
      }
    }
  }
  return QVariant();
}

bool TableModelAxes::setData(const QModelIndex &index, const QVariant &value,
                             int role) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  if (data(index, role) != value) {
    const int row = index.row();
    Axis &ax = mAxisList[row].mAxis;
    switch (index.column()) {
    case 0: {
      const double newLowerBound = ax.setLowerBound(value.toDouble());
      qDebug() << "New lower bound of axis " << index.row() << " is changed to "
               << newLowerBound;
      break;
    }
    case 1: {
      const double newUpperBound = ax.setUpperBound(value.toDouble());
      qDebug() << "New upper bound of axis " << index.row() << " is changed to "
               << newUpperBound;
      break;
    }
    case 2: {
      const double newWidth = ax.setWidth(value.toDouble());
      qDebug() << "New width of axis " << index.row() << " is changed to "
               << newWidth;
      break;
    }
    case 3: {
      const QString newPbc = value.toString();
      if (newPbc == "true") {
        ax.setPeriodicity(true, ax.lowerBound(), ax.upperBound());
      } else if (newPbc == "false") {
        ax.setPeriodicity(false, ax.lowerBound(), ax.upperBound());
      } else {
        return false;
      }
      break;
    }
    }
    emit dataChanged(index, index, QVector<int>() << role);
    qDebug() << "Axis " << row << " is : " << ax;
    return true;
  }
  return false;
}

Qt::ItemFlags TableModelAxes::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;
  return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

bool TableModelAxes::insertRows(int position, int rows,
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

bool TableModelAxes::removeRows(int position, int rows,
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

std::vector<Axis> TableModelAxes::targetAxis() const
{
  std::vector<Axis> result;
  for (const auto &i : mAxisList) {
    result.push_back(i.mAxis);
  }
  return result;
}
