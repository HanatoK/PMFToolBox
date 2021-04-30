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

#ifndef HELPER_H
#define HELPER_H

#include <QDebug>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <cmath>
#include <deque>
#include <iostream>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

QString getVersionString();

template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
almost_equal(T x, T y, int ulp = 2) {
  // the machine epsilon has to be scaled to the magnitude of the values used
  // and multiplied by the desired precision in ULPs (units in the last place)
  return std::abs(x - y) <
             std::numeric_limits<T>::epsilon() * std::abs(x + y) * ulp
         // unless the result is subnormal
         || std::abs(x - y) < std::numeric_limits<T>::min();
}

template <typename T> T stringToNumber(const QString &str, bool *ok = nullptr) {
  // check signed integer type
  if constexpr (std::is_same<T, long long int>::value) {
    return str.toLongLong(ok);
  } else if constexpr (std::is_same<T, long int>::value) {
    return str.toLong(ok);
  } else if constexpr (std::is_same<T, int>::value) {
    return str.toInt(ok);
  }
  // check signed floating type
  else if constexpr (std::is_same<T, float>::value) {
    return str.toFloat(ok);
  } else if constexpr (std::is_same<T, double>::value) {
    return str.toDouble(ok);
  }
  // check unsigned integer type
  else if constexpr (std::is_same<T, unsigned int>::value) {
    return str.toUInt(ok);
  } else if constexpr (std::is_same<T, unsigned long>::value) {
    return str.toULong(ok);
  } else if constexpr (std::is_same<T, unsigned long long>::value) {
    return str.toULongLong(ok);
  }
  // try direct conversion
  else {
    return T(str);
  }
}

template <typename T>
T stringToNumber(const QStringRef &str, bool *ok = nullptr) {
  // check signed integer type
  if constexpr (std::is_same<T, long long int>::value) {
    return str.toLongLong(ok);
  } else if constexpr (std::is_same<T, long int>::value) {
    return str.toLong(ok);
  } else if constexpr (std::is_same<T, int>::value) {
    return str.toInt(ok);
  }
  // check signed floating type
  else if constexpr (std::is_same<T, float>::value) {
    return str.toFloat(ok);
  } else if constexpr (std::is_same<T, double>::value) {
    return str.toDouble(ok);
  }
  // check unsigned integer type
  else if constexpr (std::is_same<T, unsigned int>::value) {
    return str.toUInt(ok);
  } else if constexpr (std::is_same<T, unsigned long>::value) {
    return str.toULong(ok);
  } else if constexpr (std::is_same<T, unsigned long long>::value) {
    return str.toULongLong(ok);
  }
  // try direct conversion
  else {
    return T(str);
  }
}

template <typename T> std::vector<T> splitStringToNumbers(const QString &str) {
  QVector<QStringRef> tmpFields =
      str.splitRef(QRegularExpression("[(),\\s]+"), Qt::SkipEmptyParts);
  std::vector<T> result;
  for (auto it = tmpFields.begin(); it != tmpFields.end(); ++it) {
    result.push_back(stringToNumber<T>(*it));
  }
  return result;
}

template <typename T>
void print_deque(const std::deque<T> &queue, const char *head_line) {
  using std::cout;
  using std::endl;
  cout << head_line << endl;
  for (const auto &i : queue) {
    cout << i << ' ';
  }
  cout << endl;
}

template <typename T>
void debug_priority_queue(
    std::priority_queue<std::pair<T, size_t>, std::vector<std::pair<T, size_t>>,
                        std::greater<std::pair<T, size_t>>>
        pq,
    const char *str) {
  qDebug() << str;
  while (!pq.empty()) {
    qDebug() << "(" << pq.top().first << ", " << pq.top().second << ")";
    pq.pop();
  }
}

template <typename T>
void print_vector(const std::vector<T> &v, const char *head_line) {
  using std::cout;
  using std::endl;
  cout << head_line << endl;
  for (const auto &i : v) {
    cout << i << ' ';
  }
  cout << endl;
}

QString boolToString(bool x);

double kbT(const double temperature, const QString &unit);

template <typename T, typename Alloc>
QDebug operator<<(QDebug dbg, const std::deque<T, Alloc> &data) {
  QString str{"("};
  for (auto it = data.begin(); it != data.end(); ++it) {
    str.append(QString::number(*it));
    if (it != data.end() - 1) {
      str.append(", ");
    }
  }
  str.append(")");
  dbg << str;
  return dbg;
}

#endif // HELPER_H
