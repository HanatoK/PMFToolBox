#ifndef NAMDLOGTAB_H
#define NAMDLOGTAB_H

#include "base/namdlogparser.h"

#include <QWidget>

namespace Ui {
class NAMDLogTab;
}

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

private:
  Ui::NAMDLogTab *ui;
  NAMDLogReaderThread mLogReaderThread;
  NAMDLog mLog;
};

#endif // NAMDLOGTAB_H
