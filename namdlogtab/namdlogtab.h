#ifndef NAMDLOGTAB_H
#define NAMDLOGTAB_H

#include "base/namdlogparser.h"
#include "namdlogtab/tablemodelbinning.h"

#include <QWidget>
#include <QDialog>
#include <QCheckBox>
#include <QList>

namespace Ui {
class NAMDLogTab;
}

class selectEnergyTermDialog : public QDialog {
  Q_OBJECT
public:
  selectEnergyTermDialog(const QStringList& title, QWidget *parent = nullptr);
  QStringList selectedTitle() const;

private:
  QStringList mAvailableTitle;
  QList<QCheckBox*> mCheckList;
};

class NAMDLogTab : public QWidget
{
  Q_OBJECT

public:
  explicit NAMDLogTab(QWidget *parent = nullptr);
  ~NAMDLogTab();

public slots:
  void loadNAMDLog();
  void loadNAMDLogDone(NAMDLog log);
  void logReadingProgress(int x);
  void openTrajectory();
  void saveFile();
  void runBinning();
  void binningProgress(QString status, int x);
  void addAxis();
  void removeAxis();
  void binningDone(std::vector<HistogramScalar<double> > data);

private:
  Ui::NAMDLogTab *ui;
  TableModelBinning *mTableModel;
  NAMDLogReaderThread mLogReaderThread;
  NAMDLog mLog;
  BinNAMDLogThread mBinningThread;
  QStringList mSeletedTitle;
  std::vector<HistogramScalar<double>> mHistogram;
};

#endif // NAMDLOGTAB_H
