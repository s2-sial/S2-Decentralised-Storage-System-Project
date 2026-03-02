#include "dss_worker.h"

#include <QMetaObject>
#include <stdexcept>

DssWorker::DssWorker(QObject* parent) : QObject(parent) {}

void DssWorker::runProgress(dss::Progress p) {
  emit progress(p.done, p.total, QString::fromStdString(p.message));
}

void DssWorker::putFile(const QString& trackerIp, int trackerPort,
                        const QString& filePath, quint64 chunkSize, int replicas) {
  try {
    dss::ClientConfig cfg;
    cfg.trackerIp = trackerIp.toStdString();
    cfg.trackerPort = trackerPort;
    cfg.chunkSize = chunkSize > 0 ? static_cast<size_t>(chunkSize) : 1024 * 1024;
    cfg.desiredReplicas = replicas > 0 ? replicas : 2;

    dss::DssClient client(cfg);
    std::string manifestPath = client.putFile(filePath.toStdString(), [this](dss::Progress p) {
      runProgress(p);
    });
    emit putFinished(QString::fromStdString(manifestPath));
  } catch (const std::exception& e) {
    emit error(QString::fromUtf8(e.what()));
  }
}

void DssWorker::getFile(const QString& trackerIp, int trackerPort,
                        const QString& manifestPath, const QString& outputPath) {
  try {
    dss::ClientConfig cfg;
    cfg.trackerIp = trackerIp.toStdString();
    cfg.trackerPort = trackerPort;
    cfg.chunkSize = 1024 * 1024;
    cfg.desiredReplicas = 2;

    dss::DssClient client(cfg);
    client.getFile(manifestPath.toStdString(), outputPath.toStdString(), [this](dss::Progress p) {
      runProgress(p);
    });
    emit getFinished(outputPath);
  } catch (const std::exception& e) {
    emit error(QString::fromUtf8(e.what()));
  }
}

void DssWorker::repair(const QString& trackerIp, int trackerPort,
                      int desiredReplicas, int batch) {
  try {
    dss::ClientConfig cfg;
    cfg.trackerIp = trackerIp.toStdString();
    cfg.trackerPort = trackerPort;
    cfg.chunkSize = 1024 * 1024;
    cfg.desiredReplicas = desiredReplicas > 0 ? desiredReplicas : 2;

    dss::DssClient client(cfg);
    int batchVal = batch > 0 ? batch : 50;
    client.repair(batchVal, [this](dss::Progress p) {
      runProgress(p);
    });
    emit repairFinished();
  } catch (const std::exception& e) {
    emit error(QString::fromUtf8(e.what()));
  }
}
