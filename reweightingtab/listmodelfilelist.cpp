#include "listmodelfilelist.h"

#include <QDebug>

ListModelFileList::ListModelFileList(QObject *parent)
  : QAbstractListModel(parent)
{
}

QVariant ListModelFileList::headerData(int section, Qt::Orientation orientation, int role) const
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

int ListModelFileList::rowCount(const QModelIndex &parent) const
{
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid())
    return 0;

  return mFileNameList.size();
}

QVariant ListModelFileList::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role == Qt::DisplayRole) {
    return mFileNameList[index.row()];
  }
  return QVariant();
}

bool ListModelFileList::insertRows(int row, int count, const QModelIndex &parent)
{
  row = qBound(0, row, row);
  beginInsertRows(parent, row, row + count - 1);
  for (int i = 0; i < count; ++i) {
    mFileNameList.insert(row, "");
  }
  endInsertRows();
  return true;
}

bool ListModelFileList::removeRows(int row, int count, const QModelIndex &parent)
{
  beginRemoveRows(parent, row, row + count - 1);
  for (int i = 0; i < count; ++i) {
    if (row >= 0 && row < mFileNameList.size())
      mFileNameList.removeAt(row);
  }
  endRemoveRows();
  return true;
}

void ListModelFileList::addItem(const QString &name, const QModelIndex &currentIndex)
{
  qDebug() << Q_FUNC_INFO;
  insertRows(currentIndex.row(), 1);
  int row = qBound(0, currentIndex.row(), currentIndex.row());
  mFileNameList[row] = name;
  emit layoutChanged();
  dumpList();
}

void ListModelFileList::removeItem(const QModelIndex &currentIndex)
{
  removeRows(currentIndex.row(), 1);
  emit layoutChanged();
}

QStringList ListModelFileList::trajectoryFileNameList() const
{
  return mFileNameList;
}

// for debug
void ListModelFileList::dumpList() const
{
  qDebug() << "Current file name list:";
  qDebug() << mFileNameList;
}
