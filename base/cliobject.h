#ifndef CLIOBJECT_H
#define CLIOBJECT_H

#include <QObject>
#include <QString>
#include <QJsonDocument>

class CLIObject : public QObject
{
  Q_OBJECT
public:
  explicit CLIObject(QObject *parent = nullptr);
  virtual bool readJSON(const QString &jsonFilename);
  virtual void start() = 0;
  virtual ~CLIObject();
signals:
  void allDone();
protected:
  QJsonDocument mLoadDoc;
};

#endif // CLIOBJECT_H
