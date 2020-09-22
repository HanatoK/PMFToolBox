#ifndef NAMDLOGPARSER_H
#define NAMDLOGPARSER_H

#include <QMap>
#include <QVector>
#include <QVector3D>
#include <QTextStream>
#include <cstring>

using ForceType = QVector3D;

class NAMDLog
{
public:
  NAMDLog();
  void clearData();
  void readFromStream(QTextStream& ifs);
  QVector<double> getStep() const;
  QVector<double> getVdW() const;
  QVector<double> getElectrostatic() const;
  QVector<double> getEnergyData(const QString& title, bool* ok = nullptr) const;
  QVector<ForceType> getVdWForce() const;
  QVector<ForceType> getElectrostaticForce() const;
private:
  QStringList mEnergyTitle;
  QMap<QString, QVector<double>> mEnergyData;
  QMap<QString, QVector<ForceType>> mPairData;
};

#endif // NAMDLOGPARSER_H
