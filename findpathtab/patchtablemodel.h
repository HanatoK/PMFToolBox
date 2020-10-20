#ifndef PATCHTABLEMODEL_H
#define PATCHTABLEMODEL_H

#include "base/histogram.h"

#include <QAbstractTableModel>
#include <vector>

class PatchTableModel : public QAbstractTableModel
{
  Q_OBJECT
public:


public:
  explicit PatchTableModel(QObject *parent = nullptr);

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  size_t dimension() const;
  void setDimension(int dimension);

  QList<GridDataPatch> patchList() const;

private:
  size_t mDimension;
  QList<GridDataPatch> mPatchList;
};

#endif // PATCHTABLEMODEL_H
