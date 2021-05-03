#ifndef TABLEMODELAXES_H
#define TABLEMODELAXES_H

#include "base/histogram.h"

#include <QAbstractTableModel>

class TableModelAxes : public QAbstractTableModel {
  Q_OBJECT

public:
  explicit TableModelAxes(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  bool setHeaderData(int section, Qt::Orientation orientation,
                     const QVariant &value, int role = Qt::EditRole) override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool insertRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) override;
  bool removeRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) override;

private:
  QList<AxisView> mAxisList;
};

#endif // TABLEMODELAXES_H
