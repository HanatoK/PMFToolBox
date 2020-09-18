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
  void addItem(const QString& name, const QModelIndex &currentIndex);
  void removeItem(const QModelIndex &currentIndex);
  QStringList trajectoryFileNameList() const;
  void dumpList() const;
private:
  QStringList mFileNameList;
};

#endif // LISTMODELTRAJECTORY_H
