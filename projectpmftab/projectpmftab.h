#ifndef PROJECTPMF_H
#define PROJECTPMF_H

#include <QWidget>

namespace Ui {
class ProjectPMFTab;
}

class ProjectPMFTab : public QWidget
{
  Q_OBJECT

public:
  explicit ProjectPMFTab(QWidget *parent = nullptr);
  ~ProjectPMFTab();

private:
  Ui::ProjectPMFTab *ui;
};

#endif // PROJECTPMF_H
