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
  void intermediate(qint64 step, HistogramScalar<double> PMF, HistogramVector<double> gradients);
  void done(HistogramScalar<double> PMF, HistogramVector<double> gradients);
  void runSumHills();
  void toggleWellTempered(bool enableWellTempered);
  void addAxis();
  void removeAxis();
  void progress(qint64 percent);
private:
  Ui::MetadynamicsTab *ui;
  SumHillsThread mWorkerThread;
  Metadynamics mMetaD;
  TableModelAxes *mTableModel;
};

#endif // METADYNAMICSTAB_H
