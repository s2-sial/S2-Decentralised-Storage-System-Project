#pragma once

#include <QObject>
#include <QString>

#include "core/dss/dss_client.h"

class DssWorker : public QObject {
  Q_OBJECT
public:
  explicit DssWorker(QObject* parent = nullptr);

public slots:
  void putFile(const QString& peers,
               const QString& filePath,
               const QString& localManifestDir,
               quint64 chunkSize,
               int replicas,
               const QString& rsaPublicKeyPem);
  void getFile(const QString& peers,
               const QString& manifestPath,
               const QString& outputPath,
               const QString& rsaPrivateKeyPem);
  void repair(const QString& peers, int desiredReplicas, int batch);

signals:
  void progress(int done, int total, const QString& message);
  void putFinished(const QString& shareId, const QString& localManifestPath);
  void getFinished(const QString& outputPath);
  void repairFinished();
  void error(const QString& message);

private:
  void runProgress(dss::Progress p);
};
