#ifndef PROJECTPMF_H
#define PROJECTPMF_H

#include "base/plot.h"

#include <QWidget>

namespace Ui {
class ProjectPMFTab;
}

// the computation here is not intensive so we do not need another thread
class ProjectPMFTab : public QWidget
{
  Q_OBJECT

public slots:
  void loadPMF();
  void saveFile();
  void projectPMF();
  void plotOriginPMF();
  void plotProjectedPMF();

public:
  explicit ProjectPMFTab(QWidget *parent = nullptr);
  ~ProjectPMFTab();

private:
  Ui::ProjectPMFTab *ui;
  HistogramPMF mOriginPMF;
  HistogramPMF mProjectedPMF;
};

#endif // PROJECTPMF_H
