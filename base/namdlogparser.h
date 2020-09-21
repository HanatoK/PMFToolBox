#ifndef NAMDLOGPARSER_H
#define NAMDLOGPARSER_H

#include <QMap>
#include <QVector>
#include <QVector3D>
#include <QTextStream>
#include <cstring>

using ForceType = QVector3D;

class NAMDLogParser
{
public:
  NAMDLogParser();
  void clearData();
  void readFromStream(QTextStream& ifs);
private:
  QStringList mEnergyTitle;
  QMap<QString, QVector<double>> mEnergyData;
  QMap<QString, QVector<ForceType>> mPairData;
};

#endif // NAMDLOGPARSER_H
