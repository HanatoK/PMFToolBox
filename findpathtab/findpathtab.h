#ifndef FINDPATHTAB_H
#define FINDPATHTAB_H

#include <QWidget>

namespace Ui {
class FindPathTab;
}

class FindPathTab : public QWidget
{
  Q_OBJECT

public:
  explicit FindPathTab(QWidget *parent = nullptr);
  ~FindPathTab();

private:
  Ui::FindPathTab *ui;
};

#endif // FINDPATHTAB_H
