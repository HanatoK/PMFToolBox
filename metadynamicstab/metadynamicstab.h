#ifndef METADYNAMICSTAB_H
#define METADYNAMICSTAB_H

#include "base/metadynamics.h"
#include "metadynamicstab/tablemodelaxes.h"

#include <QWidget>

namespace Ui {
class MetadynamicsTab;
}

class MetadynamicsTab : public QWidget
{
  Q_OBJECT

public:
  explicit MetadynamicsTab(QWidget *parent = nullptr);
  ~MetadynamicsTab();
public slots:
  void loadTrajectory();
  void saveFile();
  void runSumHills();
private:
  Ui::MetadynamicsTab *ui;
  SumHillsThread mWorkerThread;
  Metadynamics mMetaD;
  TableModelAxes *mTableModel;
};

#endif // METADYNAMICSTAB_H
