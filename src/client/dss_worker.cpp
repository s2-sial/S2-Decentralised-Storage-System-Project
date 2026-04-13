#include "dss_worker.h"

#include "core/net/peer_endpoint_parse.h"

#include <QMetaObject>
#include <stdexcept>
#include <sstream>

DssWorker::DssWorker(QObject* parent) : QObject(parent) {}

// Called from within blocking putFile/getFile on the worker thread.
// emit here is safe: Qt marshals the arguments across the thread
// boundary and delivers to onProgress() on the main thread.
void DssWorker::runProgress(dss::Progress p) {
  emit progress(p.done, p.total, QString::fromStdString(p.message));
}

void DssWorker::putFile(const QString& peers,
                        const QString& filePath,
                        const QString& localManifestDir,
                        quint64 chunkSize,
                        int replicas,
                        const QString& rsaPublicKeyPem) {
  try {
    dss::ClientConfig cfg;
    cfg.localManifestDir = localManifestDir.toStdString();
    const std::string peersStr = peers.toStdString();
    std::stringstream ss(peersStr);
    std::string item;
    while (std::getline(ss, item, ',')) {
      if (item.empty()) continue;
      try {
        cfg.peers.push_back(dss::net::parse_peer_entry(std::move(item)));
      } catch (const std::exception&) {
        continue;
      }
    }
    if (cfg.peers.empty()) {
      throw std::runtime_error("No valid peers provided");
    }
    cfg.chunkSize = chunkSize > 0 ? static_cast<size_t>(chunkSize) : 1024 * 1024;
    cfg.desiredReplicas = replicas > 0 ? replicas : 2;
    const QString pub = rsaPublicKeyPem.trimmed();
    if (!pub.isEmpty()) {
      cfg.rsaPublicKeyPath = pub.toStdString();
    }

    dss::DssClient client(cfg);
    dss::PutFileResult result = client.putFile(filePath.toStdString(), [this](dss::Progress p) {
      runProgress(p);
    });
    emit putFinished(QString::fromStdString(result.shareUri),
                     QString::fromStdString(result.localManifestPath));
  } catch (const std::exception& e) {
    emit error(QString::fromUtf8(e.what()));
  }
}

void DssWorker::getFile(const QString& peers,
                        const QString& manifestPath,
                        const QString& outputPath,
                        const QString& rsaPrivateKeyPem) {
  try {
    dss::ClientConfig cfg;
    const std::string peersStr = peers.toStdString();
    std::stringstream ss(peersStr);
    std::string item;
    while (std::getline(ss, item, ',')) {
      if (item.empty()) continue;
      try {
        cfg.peers.push_back(dss::net::parse_peer_entry(std::move(item)));
      } catch (const std::exception&) {
        continue;
      }
    }
    if (cfg.peers.empty()) {
      throw std::runtime_error("No valid peers provided");
    }
    cfg.chunkSize = 1024 * 1024;
    cfg.desiredReplicas = 2;
    const QString prv = rsaPrivateKeyPem.trimmed();
    if (!prv.isEmpty()) {
      cfg.rsaPrivateKeyPath = prv.toStdString();
    }

    dss::DssClient client(cfg);
    client.getFile(manifestPath.toStdString(), outputPath.toStdString(), [this](dss::Progress p) {
      runProgress(p);
    });
    emit getFinished(outputPath);
  } catch (const std::exception& e) {
    emit error(QString::fromUtf8(e.what()));
  }
}

void DssWorker::repair(const QString& trackerIp,
                      int desiredReplicas, int batch) {
  try {
    (void)trackerIp;
    (void)desiredReplicas;
    (void)batch;
    throw std::runtime_error("Repair is not supported in DHT mode");
  } catch (const std::exception& e) {
    emit error(QString::fromUtf8(e.what()));
  }
}
