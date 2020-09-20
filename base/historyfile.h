#ifndef HISOTRYFILE_H
#define HISOTRYFILE_H

#include "base/histogram.h"

#include <QObject>
#include <QThread>
#include <QMutex>

class HistoryReaderThread : public QThread {
  Q_OBJECT
public:
  HistoryReaderThread(QObject *parent = nullptr);
  ~HistoryReaderThread();
  void readFromFile(const QStringList& fileNameList);
signals:
  void done(HistogramPMFHistory PMFHistory);
  void error(QString err);
  void progress(int fileRead, int percent);
protected:
  void run() override;
private:
  bool readFromStream(QTextStream& ifs, HistogramPMFHistory& PMFHistory, int fileIndex, int fileSize);
  QMutex mutex;
  QStringList mHistoryFileName;
  static const int refreshPeriod = 5;
};

#endif // HISOTRYFILE_H
