#pragma once

#include <cstdint>
#include <memory>
#include <QMainWindow>
#include <QThread>

class DssWorker;
class QProgressBar;
class QPlainTextEdit;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class QLabel;
namespace dss::peer { class PeerService; }

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

private slots:
  void onPutClicked();
  void onGetClicked();
  void onRepairClicked();
  void onProgress(int done, int total, const QString& message);
  void onPutFinished(const QString& manifestPath);
  void onGetFinished(const QString& outputPath);
  void onRepairFinished();
  void onError(const QString& message);
  void log(const QString& text);
  void onDownloadButtonClicked();
  void refreshPeerMonitor();
  void onApplyNetworkSettingsClicked();

private:
  void setupUi();
  QWidget* makePutTab();
  QWidget* makeGetTab();
  QWidget* makeRepairTab();
  QWidget* makeNetworkTab();
  QString trackerIp() const;
  int trackerPort() const;
  void setBusy(bool busy);
  void loadOrPromptPeerSettings();
  void loadBootstrapSeedsFromSettings();
  void startEmbeddedPeerIfEnabled();
  void stopEmbeddedPeer();
  void discoverPeersFromBootstrap();

  QThread workerThread_;
  DssWorker* worker_{};
  QProgressBar* progressBar_{};
  QPlainTextEdit* logEdit_{};

  QLineEdit* trackerIpEdit_{};
  QSpinBox* trackerPortSpin_{};  // unused in DHT mode (kept for compatibility)
  QLabel* bootstrapStatusLabel_{};
  QString discoveredPeersCsv_;
  QStringList bootstrapNodes_;
  bool peerEnabled_{true};
  std::uint64_t peerMaxBytes_{0};
  int peerPort_{9101};
  QString peerStorageDir_;
  std::unique_ptr<dss::peer::PeerService> embeddedPeerService_;

  // Upload / files UI
  QTableWidget* filesTable_{};

  QLineEdit* putFileEdit_{};
  QLineEdit* putChunkEdit_{};
  QSpinBox* putReplicasSpin_{};

  QLineEdit* getManifestEdit_{};
  QLineEdit* getOutputEdit_{};

  QSpinBox* repairReplicasSpin_{};
  QSpinBox* repairBatchSpin_{};

  // Peer monitor UI
  QTableWidget* peersTable_{};
  QLabel* networkMapLabel_{};
  QLineEdit* bootstrapSeedsEdit_{};
  QLineEdit* advertiseIpEdit_{};

  QString peerAdvertiseIp_{QStringLiteral("127.0.0.1")};
};
