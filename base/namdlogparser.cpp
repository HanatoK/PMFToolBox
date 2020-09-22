#include "namdlogparser.h"

NAMDLog::NAMDLog()
{

}

void NAMDLog::clearData()
{
  mEnergyTitle.clear();
  mEnergyData.clear();
  mPairData.clear();
}

void NAMDLog::readFromStream(QTextStream &ifs)
{
  QString line;
  bool firsttime = true;
  while (!ifs.atEnd()) {
    ifs.readLineInto(&line);
    if (firsttime) {
      if (line.startsWith("ETITLE:")) {
        firsttime = false;
        QStringList titles = line.split(QRegExp("(ETITLE:|\\s+)"), Qt::SkipEmptyParts);
        for (auto it = titles.begin(); it != titles.end(); ++it) {
          // +1 to skip the "ETITLE" field
          const auto keyFound = mEnergyData.find(*it);
          if (keyFound == mEnergyData.end()) {
            mEnergyTitle.append(*it);
            mEnergyData[*it] = QVector<double>();
          }
        }
      }
    }
    if (line.startsWith("ENERGY:")) {
      QStringList fields = line.split(QRegExp("[A-Z:\\s]+"), Qt::SkipEmptyParts);
      for (int i = 0; i < mEnergyTitle.size(); ++i) {
        mEnergyData[mEnergyTitle[i]].append(fields[i].toDouble());
      }
    }
    if (line.startsWith("PAIR INTERACTION:")) {
      QStringList fields = line.split(QRegExp("[A-Z:_\\s]+"), Qt::SkipEmptyParts);
      if (fields.size() == 7) {
        mPairData["VDW_FORCE"].append(QVector3D(fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble()));
        mPairData["ELECT_FORCE"].append(QVector3D(fields[4].toDouble(), fields[5].toDouble(), fields[6].toDouble()));
      }
    }
  }
}

QVector<double> NAMDLog::getStep() const
{
  return getEnergyData("TS");
}

QVector<double> NAMDLog::getVdW() const
{
  return getEnergyData("VDW");
}

QVector<double> NAMDLog::getElectrostatic() const
{
  return getEnergyData("ELECT");
}

QVector<double> NAMDLog::getEnergyData(const QString &title, bool *ok) const
{
  const auto keyFound = mEnergyData.find(title);
  if (keyFound == mEnergyData.end()) {
    if (ok != nullptr) *ok = false;
    return QVector<double>();
  } else {
    if (ok != nullptr) *ok = true;
    return mEnergyData.value(title);
  }
}

QVector<ForceType> NAMDLog::getVdWForce() const
{
  return mPairData.value("VDW_FORCE");
}

QVector<ForceType> NAMDLog::getElectrostaticForce() const
{
  return mPairData.value("ELECT_FORCE");
}
