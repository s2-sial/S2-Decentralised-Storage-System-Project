#pragma once

#include <QObject>
#include <QString>

#include "core/dss/dss_client.h"

class DssWorker : public QObject {
  Q_OBJECT
public:
  explicit DssWorker(QObject* parent = nullptr);

public slots:
  void putFile(const QString& trackerIp, int trackerPort,
               const QString& filePath, quint64 chunkSize, int replicas);
  void getFile(const QString& trackerIp, int trackerPort,
               const QString& manifestPath, const QString& outputPath);
  void repair(const QString& trackerIp, int trackerPort, int desiredReplicas, int batch);

signals:
  void progress(int done, int total, const QString& message);
  void putFinished(const QString& manifestPath);
  void getFinished(const QString& outputPath);
  void repairFinished();
  void error(const QString& message);

private:
  void runProgress(dss::Progress p);
};
