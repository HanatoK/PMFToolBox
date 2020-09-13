#include "listmodeltrajectory.h"

#include <QDebug>

ListModelTrajectory::ListModelTrajectory(QObject *parent)
  : QAbstractListModel(parent)
{
}

QVariant ListModelTrajectory::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
      switch (section) {
      case 0: return QString{"Trajectory file(s)"}; break;
      }
  }
  if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
      return section;
  }
  return QVariant();
}

int ListModelTrajectory::rowCount(const QModelIndex &parent) const
{
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid())
    return 0;

  return mTrajectoryFileNameList.size();
}

QVariant ListModelTrajectory::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    return mTrajectoryFileNameList[index.row()];
  }
  return QVariant();
}

bool ListModelTrajectory::insertRows(int row, int count, const QModelIndex &parent)
{
  beginInsertRows(parent, row, row + count - 1);
  row = qBound(0, row, row);
  for (int i = 0; i < count; ++i) {
    mTrajectoryFileNameList.insert(row, "");
  }
  endInsertRows();
  return true;
}

bool ListModelTrajectory::removeRows(int row, int count, const QModelIndex &parent)
{
  beginRemoveRows(parent, row, row + count - 1);
  for (int i = 0; i < count; ++i) {
    mTrajectoryFileNameList.removeAt(row);
  }
  endRemoveRows();
  return true;
}

void ListModelTrajectory::addItem(const QString &name, const QModelIndex &currentIndex)
{
  qDebug() << Q_FUNC_INFO;
  insertRows(currentIndex.row(), 1);
  int row = qBound(0, currentIndex.row(), currentIndex.row());
  mTrajectoryFileNameList[row] = name;
  emit layoutChanged();
  dumpList();
}

void ListModelTrajectory::removeItem(const QModelIndex &currentIndex)
{
  removeRows(currentIndex.row(), 1);
  emit layoutChanged();
}

QStringList ListModelTrajectory::trajectoryFileNameList() const
{
  return mTrajectoryFileNameList;
}

// for debug
void ListModelTrajectory::dumpList() const
{
  qDebug() << "Current file name list:";
  qDebug() << mTrajectoryFileNameList;
}
