#ifndef NAMDLOGTAB_H
#define NAMDLOGTAB_H

#include "base/namdlogparser.h"

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

private:
  Ui::NAMDLogTab *ui;
  NAMDLogReaderThread mLogReaderThread;
  NAMDLog mLog;
};

#endif // NAMDLOGTAB_H
