/*
  PMFToolBox: A toolbox to analyze and post-process the output of
  potential of mean force calculations.
  Copyright (C) 2020  Haochuan Chen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "helper.h"
#include "common.h"

#include <QDebug>

QString getVersionString() {
  return QString("%1.%2.%3").arg(MAJOR_VERSION).arg(MINOR_VERSION).arg(RELEASE);
}

QString boolToString(bool x) {
  if (x == true)
    return "True";
  else
    return "False";
}

double kbT(const double temperature, const QString &unit) {
  qDebug() << "Calling" << Q_FUNC_INFO;
  double factor = 1.0;
  if (unit.compare("kcal/mol", Qt::CaseInsensitive) == 0) {
    factor = 0.001987204258641;
  } else if (unit.compare("kj/mol", Qt::CaseInsensitive) == 0) {
    factor = 0.008314462618153;
  } else {
    qDebug() << Q_FUNC_INFO << ": undefined unit factor, use " << factor;
  }
  const double kbt = temperature * factor;
  return kbt;
}
