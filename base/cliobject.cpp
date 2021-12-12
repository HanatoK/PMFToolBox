#include "cliobject.h"

#include <QFile>
#include <QJsonParseError>

CLIObject::CLIObject(QObject *parent)
  : QObject{parent}
{

}

bool CLIObject::readJSON(const QString& jsonFilename)
{
  qDebug() << "Reading" << jsonFilename;
  QFile loadFile(jsonFilename);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning() << QString("Could not open json file") + jsonFilename;
    return false;
  }
  const QByteArray jsonData = loadFile.readAll();
  QJsonParseError jsonParseError;
  mLoadDoc = QJsonDocument::fromJson(jsonData, &jsonParseError);
  if (mLoadDoc.isNull()) {
    qWarning() << QString("Invalid json file:") + jsonFilename;
    qWarning() << "Json parse error:" << jsonParseError.errorString();
    return false;
  }
  return true;
}

CLIObject::~CLIObject()
{

}
