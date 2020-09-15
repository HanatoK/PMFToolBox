#ifndef HELPER_H
#define HELPER_H

#include <limits>
#include <utility>
#include <cmath>
#include <QString>
#include <QStringList>
#include <QRegExp>

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

template <typename T>
T stringToNumber(const QString& str, bool* ok = nullptr) {
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
QVector<T> splitStringToNumbers(const QString& str) {
  QStringList tmpFields = str.split(QRegExp("[(),\\s]+"), Qt::SkipEmptyParts);
  QVector<T> result;
  for (auto it = tmpFields.begin(); it != tmpFields.end(); ++it) {
    result.append(stringToNumber<T>(*it));
  }
  return result;
}

QString boolToString(bool x);

double kbT(const double temperature, const QString& unit);

#endif // HELPER_H
