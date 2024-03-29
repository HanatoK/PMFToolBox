#ifndef METADYNAMICSTAB_H
#define METADYNAMICSTAB_H

#include "base/metadynamics.h"
#include "base/cliobject.h"
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
  TableModelAxes *mTableModel;
};

class MetadynamicsCLI : public CLIObject {
  Q_OBJECT
public:
  explicit MetadynamicsCLI(QObject* parent);
  virtual bool readJSON(const QString &jsonFilename) override;
  virtual void start() override;
  ~MetadynamicsCLI();
public slots:
  void progress(int percent);
  void error(QString msg);
  void intermediate(qint64 step, HistogramScalar<double> PMF, HistogramVector<double> gradients);
  void done(HistogramScalar<double> PMF, HistogramVector<double> gradients);
private:
  QString mTrajectoryFilename;
  QString mOutputPrefix;
  std::vector<Axis> mAxes;
  qint64 mStride;
  bool mIsWellTempered;
  double mDeltaT;
  double mTemperature;
  SumHillsThread mWorkerThread;
};

#endif // METADYNAMICSTAB_H
