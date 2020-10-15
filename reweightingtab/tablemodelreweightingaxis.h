#ifndef REWEIGHTINGAXISTABLEMODEL_H
#define REWEIGHTINGAXISTABLEMODEL_H

#include "base/histogram.h"

#include <QAbstractTableModel>
#include <QObject>

class TableModelReweightingAxis : public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit TableModelReweightingAxis(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  void addItem(const Axis& ax, int col, bool inPMF, bool reweightingTo);
  std::vector<int> fromColumns() const;
  std::vector<int> toColumns() const;
  std::vector<Axis> targetAxis() const;
  void clearAll();
private:
  QList<AxisView> mAxisList;
};

#endif // REWEIGHTINGAXISTABLEMODEL_H
