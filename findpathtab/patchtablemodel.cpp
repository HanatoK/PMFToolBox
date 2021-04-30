#include "patchtablemodel.h"

PatchTableModel::PatchTableModel(QObject *parent)
    : QAbstractTableModel(parent), mDimension(0) {}

QVariant PatchTableModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    if (mDimension > 0) {
      // horizontal header
      if (section == 0)
        return QString("Center");
      else if (section == int(mDimension + 1))
        return QString("Potential");
      else {
        return QString("Axis ") + QString::number(section);
      }
    }
  }
  if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
    return section;
  }
  return QVariant();
}

int PatchTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return mPatchList.size();
}

int PatchTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return mDimension + 2;
}

QVariant PatchTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    const int row_index = index.row();
    const int column_index = index.column();
    if (mDimension == 0 || mPatchList.size() == 0)
      return QVariant();
    if (row_index >= 0 && column_index >= 0) {
      if (column_index == 0) {
        QString text("(");
        for (std::size_t i = 0; i < mPatchList[row_index].mCenter.size(); ++i) {
          text += QString::number(mPatchList[row_index].mCenter[i], 'g', 2);
          if (i != mPatchList[row_index].mCenter.size() - 1) {
            text += QString(", ");
          }
        }
        text += QString(")");
        return text;
      } else if (column_index == int(mDimension + 1))
        return mPatchList[row_index].mValue;
      else {
        return mPatchList[row_index].mLength[column_index - 1];
      }
    }
  }
  return QVariant();
}

bool PatchTableModel::insertRows(int position, int rows,
                                 const QModelIndex &index) {
  Q_UNUSED(index);
  beginInsertRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    mPatchList.insert(position, GridDataPatch());
  }

  endInsertRows();
  return true;
}

bool PatchTableModel::removeRows(int position, int rows,
                                 const QModelIndex &index) {
  Q_UNUSED(index);
  beginRemoveRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    mPatchList.removeAt(position);
  }

  endRemoveRows();
  return true;
}

bool PatchTableModel::setData(const QModelIndex &index, const QVariant &value,
                              int role) {
  if (index.isValid() && role == Qt::EditRole) {
    const int row = index.row();
    const int col = index.column();
    auto patch = mPatchList.value(row);
    if (col == 0) {
      std::vector<double> fields =
          splitStringToNumbers<double>(value.toString());
      if (fields.size() != mDimension)
        return false;
      patch.mCenter = fields;
    } else if (col == int(mDimension + 1)) {
      patch.mValue = value.toDouble();
    } else if (col > 0 && col <= int(mDimension)) {
      if (patch.mLength.size() != mDimension)
        patch.mLength.resize(mDimension);
      patch.mLength[col - 1] = value.toDouble();
    } else {
      return false;
    }
    mPatchList.replace(row, patch);
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
  }
  return false;
}

Qt::ItemFlags PatchTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

size_t PatchTableModel::dimension() const { return mDimension; }

void PatchTableModel::setDimension(int dimension) {
  mDimension = dimension;
  emit layoutChanged();
}

QList<GridDataPatch> PatchTableModel::patchList() const { return mPatchList; }
