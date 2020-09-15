#include "helper.h"

#include <QDebug>

QString boolToString(bool x) {
  if (x == true) return "True";
  else return "False";
}

double kbT(const double temperature, const QString &unit)
{
  qDebug() << "Calling " << Q_FUNC_INFO;
  double factor = 1.0;
  if (unit.compare("kcal/mol", Qt::CaseInsensitive) == 0) {
#ifdef SI2019
    factor = 0.001985875;
#else
    factor = 0.0019872041;
#endif
  } else if (unit.compare("kj/mol", Qt::CaseInsensitive) == 0) {
#ifdef SI2019
    factor = 0.008314463;
#else
    factor = 0.0083144621;
#endif
  } else {
    qDebug() << Q_FUNC_INFO << ": undefined unit factor, use " << factor;
  }
  const double kbt = temperature * factor;
  return kbt;
}
