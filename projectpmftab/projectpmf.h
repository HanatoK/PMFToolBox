#ifndef PROJECTPMF_H
#define PROJECTPMF_H

#include <QWidget>

namespace Ui {
class ProjectPMF;
}

class ProjectPMF : public QWidget
{
  Q_OBJECT

public:
  explicit ProjectPMF(QWidget *parent = nullptr);
  ~ProjectPMF();

private:
  Ui::ProjectPMF *ui;
};

#endif // PROJECTPMF_H
