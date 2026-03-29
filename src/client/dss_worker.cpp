#include "dss_worker.h"

#include <QMetaObject>
#include <stdexcept>

DssWorker::DssWorker(QObject* parent) : QObject(parent) {}

void DssWorker::runProgress(dss::Progress p) {
  emit progress(p.done, p.total, QString::fromStdString(p.message));
}

void DssWorker::putFile(const QString& peers,
                        const QString& filePath, quint64 chunkSize, int replicas) {
  try {
    dss::ClientConfig cfg;
    const std::string peersStr = peers.toStdString();
    std::stringstream ss(peersStr);
    std::string item;
    while (std::getline(ss, item, ',')) {
      auto trim = [](std::string s) {
        const char* ws = " \t\r\n";
        auto b = s.find_first_not_of(ws);
        if (b == std::string::npos) return std::string();
        auto e = s.find_last_not_of(ws);
        return s.substr(b, e - b + 1);
      };
      item = trim(item);
      if (item.empty()) continue;
      auto pos = item.find(':');
      if (pos == std::string::npos) continue;
      dss::PeerEndpoint ep;
      std::string ipPart = trim(item.substr(0, pos));
      std::string portPart = trim(item.substr(pos + 1));
      if (ipPart.empty() || portPart.empty()) continue;
      ep.ip = ipPart;
      ep.port = std::stoi(portPart);
      cfg.peers.push_back(ep);
    }
    if (cfg.peers.empty()) {
      throw std::runtime_error("No valid peers provided");
    }
    cfg.chunkSize = chunkSize > 0 ? static_cast<size_t>(chunkSize) : 1024 * 1024;
    cfg.desiredReplicas = replicas > 0 ? replicas : 2;

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
                        const QString& manifestPath, const QString& outputPath) {
  try {
    dss::ClientConfig cfg;
    const std::string peersStr = peers.toStdString();
    std::stringstream ss(peersStr);
    std::string item;
    while (std::getline(ss, item, ',')) {
      auto trim = [](std::string s) {
        const char* ws = " \t\r\n";
        auto b = s.find_first_not_of(ws);
        if (b == std::string::npos) return std::string();
        auto e = s.find_last_not_of(ws);
        return s.substr(b, e - b + 1);
      };
      item = trim(item);
      if (item.empty()) continue;
      auto pos = item.find(':');
      if (pos == std::string::npos) continue;
      dss::PeerEndpoint ep;
      std::string ipPart = trim(item.substr(0, pos));
      std::string portPart = trim(item.substr(pos + 1));
      if (ipPart.empty() || portPart.empty()) continue;
      ep.ip = ipPart;
      ep.port = std::stoi(portPart);
      cfg.peers.push_back(ep);
    }
    if (cfg.peers.empty()) {
      throw std::runtime_error("No valid peers provided");
    }
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
