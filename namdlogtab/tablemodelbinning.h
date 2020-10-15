#ifndef TABLEMODELBINNING_H
#define TABLEMODELBINNING_H

#include "base/histogram.h"

#include <QAbstractTableModel>

class TableModelBinning : public QAbstractTableModel
{
  Q_OBJECT

public:
  explicit TableModelBinning(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  std::vector<int> fromColumns() const;
  std::vector<Axis> targetAxis() const;
  void clearAll();

private:
  QList<AxisView> mAxisList;
};

#endif // TABLEMODELBINNING_H
