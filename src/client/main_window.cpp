#include "main_window.h"
#include "dss_worker.h"

#include <QApplication>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWidget>
#include <QFile>

#include "core/manifest/manifest.h"
#include "core/dht/kademlia_id.h"
#include "core/peer/peer_service.h"

#include <filesystem>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  //Move worker to its own OS thread - all its slots execute there
  worker_ = new DssWorker;
  worker_->moveToThread(&workerThread_);
  connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

  //Qt::QueuedConnection: signal emission on worker thread is queued as
  //an event; the slot fires on the receiver's thread (main/GUI thread).
  //This is the only safe way to update widgets from a background thread.
  connect(worker_, &DssWorker::progress, this, &MainWindow::onProgress, Qt::QueuedConnection);
  connect(worker_, &DssWorker::putFinished, this, &MainWindow::onPutFinished, Qt::QueuedConnection);
  connect(worker_, &DssWorker::getFinished, this, &MainWindow::onGetFinished, Qt::QueuedConnection);
  connect(worker_, &DssWorker::repairFinished, this, &MainWindow::onRepairFinished, Qt::QueuedConnection);
  connect(worker_, &DssWorker::error, this, &MainWindow::onError, Qt::QueuedConnection);

  workerThread_.start();
  loadOrPromptPeerSettings();
  setupUi();
  if (bootstrapSeedsEdit_) {
    bootstrapSeedsEdit_->setText(bootstrapNodes_.join(','));
  }
  if (advertiseIpEdit_) {
    advertiseIpEdit_->setText(peerAdvertiseIp_);
  }
  startEmbeddedPeerIfEnabled();
  discoverPeersFromBootstrap();
  setWindowTitle(tr("Decent Store"));
  resize(520, 420);
}

MainWindow::~MainWindow() {
  stopEmbeddedPeer();
  workerThread_.quit();
  workerThread_.wait();
}

void MainWindow::setupUi() {
  QWidget* central = new QWidget(this);
  QVBoxLayout* mainLayout = new QVBoxLayout(central);

  QGroupBox* connGroup = new QGroupBox(tr("Network Join (Bootstrap)"));
  QFormLayout* connLayout = new QFormLayout(connGroup);
  trackerIpEdit_ = new QLineEdit(this);
  trackerIpEdit_->setReadOnly(true);
  trackerIpEdit_->setPlaceholderText(tr("Discovered peers will appear here"));
  connLayout->addRow(tr("Discovered peers:"), trackerIpEdit_);
  bootstrapStatusLabel_ = new QLabel(tr("Joining network via bootstrap nodes..."), this);
  connLayout->addRow(tr("Status:"), bootstrapStatusLabel_);
  trackerPortSpin_ = nullptr;
  mainLayout->addWidget(connGroup);

  QTabWidget* tabs = new QTabWidget(this);
  tabs->addTab(makePutTab(), tr("Put"));
  tabs->addTab(makeGetTab(), tr("Get"));
  tabs->addTab(makeRepairTab(), tr("Repair"));
  tabs->addTab(makeSettingsTab(), tr("Settings"));
  tabs->addTab(makeNetworkTab(), tr("Network"));
  mainLayout->addWidget(tabs);

  syncPeerSettingsToUi();

  progressBar_ = new QProgressBar(this);
  progressBar_->setRange(0, 0);
  progressBar_->setTextVisible(true);
  progressBar_->setVisible(false);
  mainLayout->addWidget(progressBar_);

  logEdit_ = new QPlainTextEdit(this);
  logEdit_->setReadOnly(true);
  logEdit_->setMaximumHeight(120);
  mainLayout->addWidget(logEdit_);

  setCentralWidget(central);
}

QWidget* MainWindow::makePutTab() {
  QWidget* w = new QWidget(this);
  QVBoxLayout* vbox = new QVBoxLayout(w);

  QGroupBox* uploadGroup = new QGroupBox(tr("Upload File"), w);
  QFormLayout* form = new QFormLayout(uploadGroup);
  putFileEdit_ = new QLineEdit(this);
  putFileEdit_->setPlaceholderText(tr("Path to file to upload"));
  auto* putFileRow = new QWidget(this);
  auto* putFileLayout = new QHBoxLayout(putFileRow);
  putFileLayout->setContentsMargins(0, 0, 0, 0);
  putFileLayout->addWidget(putFileEdit_);
  auto* putBrowseBtn = new QPushButton(tr("Browse…"), this);
  putFileLayout->addWidget(putBrowseBtn);
  form->addRow(tr("File:"), putFileRow);
  connect(putBrowseBtn, &QPushButton::clicked, this, [this]() {
    const QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Select file to upload"));
    if (!path.isEmpty()) {
      putFileEdit_->setText(path);
    }
  });
  putChunkEdit_ = new QLineEdit(this);
  putChunkEdit_->setPlaceholderText(tr("Default 1048576 (1 MiB)"));
  form->addRow(tr("Chunk size (bytes):"), putChunkEdit_);
  putReplicasSpin_ = new QSpinBox(this);
  putReplicasSpin_->setRange(1, 32);
  putReplicasSpin_->setValue(2);
  form->addRow(tr("Replicas:"), putReplicasSpin_);
  putRsaPublicEdit_ = new QLineEdit(this);
  putRsaPublicEdit_->setPlaceholderText(tr("Optional — RSA public key (.pem) for AES-256-GCM + hybrid wrap"));
  auto* rsaPubRow = new QWidget(this);
  auto* rsaPubLayout = new QHBoxLayout(rsaPubRow);
  rsaPubLayout->setContentsMargins(0, 0, 0, 0);
  rsaPubLayout->addWidget(putRsaPublicEdit_);
  auto* rsaPubBrowse = new QPushButton(tr("Browse…"), this);
  rsaPubLayout->addWidget(rsaPubBrowse);
  form->addRow(tr("RSA public key:"), rsaPubRow);
  connect(rsaPubBrowse, &QPushButton::clicked, this, [this]() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Select RSA public key"),
        QString(),
        tr("PEM files (*.pem);;All files (*.*)"));
    if (!path.isEmpty()) {
      putRsaPublicEdit_->setText(path);
    }
  });
  QPushButton* btn = new QPushButton(tr("Put file"), this);
  connect(btn, &QPushButton::clicked, this, &MainWindow::onPutClicked);
  form->addRow(btn);

  vbox->addWidget(uploadGroup);

  // Stored files table
  QGroupBox* filesGroup = new QGroupBox(tr("Stored Files"), w);
  QVBoxLayout* filesLayout = new QVBoxLayout(filesGroup);
  filesTable_ = new QTableWidget(filesGroup);
  filesTable_->setColumnCount(4);
  QStringList headers;
  headers << tr("Share ID") << tr("Name") << tr("Size (bytes)") << tr("Download");
  filesTable_->setHorizontalHeaderLabels(headers);
  filesTable_->horizontalHeader()->setStretchLastSection(true);
  filesTable_->setSelectionMode(QAbstractItemView::NoSelection);
  filesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  filesLayout->addWidget(filesTable_);
  vbox->addWidget(filesGroup);

  vbox->addStretch(1);
  return w;
}

QWidget* MainWindow::makeGetTab() {
  QWidget* w = new QWidget(this);
  QFormLayout* form = new QFormLayout(w);
  getManifestEdit_ = new QLineEdit(this);
  getManifestEdit_->setPlaceholderText(
      tr("dss://file/<sha256> or path to .manifest.txt"));
  auto* manifestRow = new QWidget(this);
  auto* manifestLayout = new QHBoxLayout(manifestRow);
  manifestLayout->setContentsMargins(0, 0, 0, 0);
  manifestLayout->addWidget(getManifestEdit_);
  auto* manifestBrowseBtn = new QPushButton(tr("Browse…"), this);
  manifestLayout->addWidget(manifestBrowseBtn);
  form->addRow(tr("Manifest:"), manifestRow);
  connect(manifestBrowseBtn, &QPushButton::clicked, this, [this]() {
    const QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Select manifest"),
                                                      QString(),
                                                      tr("Manifest files (*.manifest.txt);;All files (*.*)"));
    if (!path.isEmpty()) {
      getManifestEdit_->setText(path);
    }
  });
  getOutputEdit_ = new QLineEdit(this);
  getOutputEdit_->setPlaceholderText(tr("Output file path"));
  auto* outputRow = new QWidget(this);
  auto* outputLayout = new QHBoxLayout(outputRow);
  outputLayout->setContentsMargins(0, 0, 0, 0);
  outputLayout->addWidget(getOutputEdit_);
  auto* outputBrowseBtn = new QPushButton(tr("Browse…"), this);
  outputLayout->addWidget(outputBrowseBtn);
  form->addRow(tr("Output file:"), outputRow);
  connect(outputBrowseBtn, &QPushButton::clicked, this, [this]() {
    const QString path = QFileDialog::getSaveFileName(this,
                                                      tr("Select output file"));
    if (!path.isEmpty()) {
      getOutputEdit_->setText(path);
    }
  });
  getRsaPrivateEdit_ = new QLineEdit(this);
  getRsaPrivateEdit_->setPlaceholderText(tr("Optional — RSA private key (.pem) if upload was encrypted"));
  auto* rsaPrvRow = new QWidget(this);
  auto* rsaPrvLayout = new QHBoxLayout(rsaPrvRow);
  rsaPrvLayout->setContentsMargins(0, 0, 0, 0);
  rsaPrvLayout->addWidget(getRsaPrivateEdit_);
  auto* rsaPrvBrowse = new QPushButton(tr("Browse…"), this);
  rsaPrvLayout->addWidget(rsaPrvBrowse);
  form->addRow(tr("RSA private key:"), rsaPrvRow);
  connect(rsaPrvBrowse, &QPushButton::clicked, this, [this]() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Select RSA private key"),
        QString(),
        tr("PEM files (*.pem);;All files (*.*)"));
    if (!path.isEmpty()) {
      getRsaPrivateEdit_->setText(path);
    }
  });
  QPushButton* btn = new QPushButton(tr("Get file"), this);
  connect(btn, &QPushButton::clicked, this, &MainWindow::onGetClicked);
  form->addRow(btn);
  return w;
}

QWidget* MainWindow::makeRepairTab() {
  QWidget* w = new QWidget(this);
  QVBoxLayout* outer = new QVBoxLayout(w);
  QLabel* info = new QLabel(
      tr("Tracker-based automatic repair is not available in DHT-only mode. "
         "This single-app build uses peer bootstrap and embedded storage only. "
         "Upload with enough replicas to tolerate peer loss."),
      w);
  info->setWordWrap(true);
  outer->addWidget(info);

  QFormLayout* form = new QFormLayout;
  repairReplicasSpin_ = new QSpinBox(this);
  repairReplicasSpin_->setRange(1, 32);
  repairReplicasSpin_->setValue(2);
  repairReplicasSpin_->setEnabled(false);
  form->addRow(tr("Desired replicas:"), repairReplicasSpin_);
  repairBatchSpin_ = new QSpinBox(this);
  repairBatchSpin_->setRange(1, 1000);
  repairBatchSpin_->setValue(50);
  repairBatchSpin_->setEnabled(false);
  form->addRow(tr("Batch size:"), repairBatchSpin_);
  QPushButton* btn = new QPushButton(tr("Repair"), this);
  btn->setEnabled(false);
  btn->setToolTip(tr("Not available without a tracker"));
  connect(btn, &QPushButton::clicked, this, &MainWindow::onRepairClicked);
  form->addRow(btn);
  outer->addLayout(form);
  outer->addStretch(1);
  return w;
}

QWidget* MainWindow::makeSettingsTab() {
  QWidget* w = new QWidget(this);
  QVBoxLayout* outer = new QVBoxLayout(w);

  QGroupBox* peerGroup = new QGroupBox(tr("Local peer (embedded)"), w);
  QFormLayout* form = new QFormLayout(peerGroup);

  settingsPeerEnabledCheck_ = new QCheckBox(tr("Enable peer contribution (storage and network listener)"), w);
  form->addRow(settingsPeerEnabledCheck_);

  settingsPeerPortSpin_ = new QSpinBox(w);
  settingsPeerPortSpin_->setRange(1024, 65535);
  form->addRow(tr("Peer port:"), settingsPeerPortSpin_);

  settingsMaxGiBSpin_ = new QSpinBox(w);
  settingsMaxGiBSpin_->setRange(1, 1024);
  form->addRow(tr("Max storage contribution (GiB):"), settingsMaxGiBSpin_);

  settingsStorageDirEdit_ = new QLineEdit(w);
  settingsStorageDirEdit_->setPlaceholderText(tr("Directory for chunk and manifest storage"));
  auto* storageRow = new QWidget(w);
  auto* storageLayout = new QHBoxLayout(storageRow);
  storageLayout->setContentsMargins(0, 0, 0, 0);
  storageLayout->addWidget(settingsStorageDirEdit_);
  auto* storageBrowse = new QPushButton(tr("Browse…"), w);
  storageLayout->addWidget(storageBrowse);
  form->addRow(tr("Storage directory:"), storageRow);
  connect(storageBrowse, &QPushButton::clicked, this, [this]() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select peer storage directory"), settingsStorageDirEdit_->text());
    if (!dir.isEmpty()) {
      settingsStorageDirEdit_->setText(dir);
    }
  });

  QLabel* hint = new QLabel(
      tr("Click Apply to save. Changing port, storage cap, or directory restarts the embedded peer."),
      w);
  hint->setWordWrap(true);
  form->addRow(hint);

  auto* applyBtn = new QPushButton(tr("Apply peer settings"), w);
  connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onApplySettingsClicked);
  form->addRow(applyBtn);

  outer->addWidget(peerGroup);
  outer->addStretch(1);
  return w;
}

void MainWindow::syncPeerSettingsToUi() {
  if (!settingsPeerEnabledCheck_ || !settingsPeerPortSpin_ || !settingsMaxGiBSpin_ ||
      !settingsStorageDirEdit_) {
    return;
  }
  settingsPeerEnabledCheck_->setChecked(peerEnabled_);
  settingsPeerPortSpin_->setValue(peerPort_);
  std::uint64_t gib = peerMaxBytes_ / (1024ull * 1024 * 1024);
  if (gib < 1) {
    gib = 1;
  }
  if (gib > 1024) {
    gib = 1024;
  }
  settingsMaxGiBSpin_->setValue(static_cast<int>(gib));
  settingsStorageDirEdit_->setText(peerStorageDir_);
}

void MainWindow::onApplySettingsClicked() {
  if (!settingsPeerEnabledCheck_) {
    return;
  }

  const QString dir = settingsStorageDirEdit_->text().trimmed();
  if (dir.isEmpty()) {
    QMessageBox::warning(this, tr("Settings"), tr("Storage directory cannot be empty."));
    return;
  }

  peerEnabled_ = settingsPeerEnabledCheck_->isChecked();
  peerPort_ = settingsPeerPortSpin_->value();
  const int gib = settingsMaxGiBSpin_->value();
  peerMaxBytes_ = static_cast<std::uint64_t>(gib) * 1024ull * 1024ull * 1024ull;
  peerStorageDir_ = dir;

  QSettings settings(QStringLiteral("decent_store"), QStringLiteral("decent_store"));
  settings.setValue(QStringLiteral("peer/enabled"), peerEnabled_);
  settings.setValue(QStringLiteral("peer/port"), peerPort_);
  settings.setValue(QStringLiteral("peer/max_bytes"), static_cast<qulonglong>(peerMaxBytes_));
  settings.setValue(QStringLiteral("peer/storage_dir"), peerStorageDir_);

  const bool hadRunning = embeddedPeerService_ && embeddedPeerService_->isRunning();
  if (hadRunning) {
    stopEmbeddedPeer();
  }
  startEmbeddedPeerIfEnabled();
  discoverPeersFromBootstrap();
  log(tr("Peer settings saved (port %1, max %2 GiB).")
          .arg(peerPort_)
          .arg(gib));
  QMessageBox::information(this, tr("Settings"),
                           tr("Peer settings saved. The embedded peer was restarted if it was "
                              "running or is enabled."));
}

QWidget* MainWindow::makeNetworkTab() {
  QWidget* w = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(w);

  QGroupBox* routingGroup = new QGroupBox(tr("Routing configuration"), w);
  QFormLayout* routingForm = new QFormLayout(routingGroup);
  bootstrapSeedsEdit_ = new QLineEdit(w);
  bootstrapSeedsEdit_->setPlaceholderText(
      tr("Comma-separated host:port seeds, e.g. 127.0.0.1:9101,192.168.1.10:9101"));
  routingForm->addRow(tr("Bootstrap seeds:"), bootstrapSeedsEdit_);
  advertiseIpEdit_ = new QLineEdit(w);
  advertiseIpEdit_->setPlaceholderText(tr("Address other peers use to reach this node (e.g. LAN IP)"));
  routingForm->addRow(tr("Advertise address:"), advertiseIpEdit_);
  auto* applyRow = new QWidget(w);
  auto* applyLayout = new QHBoxLayout(applyRow);
  applyLayout->setContentsMargins(0, 0, 0, 0);
  QPushButton* applyBtn = new QPushButton(tr("Apply network settings"), w);
  connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyNetworkSettingsClicked);
  applyLayout->addWidget(applyBtn);
  applyLayout->addStretch(1);
  routingForm->addRow(applyRow);
  layout->addWidget(routingGroup);

  QHBoxLayout* topRow = new QHBoxLayout;
  QPushButton* refreshBtn = new QPushButton(tr("Refresh peers"), w);
  connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshPeerMonitor);
  topRow->addWidget(refreshBtn);
  topRow->addStretch(1);
  layout->addLayout(topRow);

  peersTable_ = new QTableWidget(w);
  peersTable_->setColumnCount(3);
  QStringList headers;
  headers << tr("Peer") << tr("Status") << tr("Node ID (prefix)");
  peersTable_->setHorizontalHeaderLabels(headers);
  peersTable_->horizontalHeader()->setStretchLastSection(true);
  peersTable_->setSelectionMode(QAbstractItemView::NoSelection);
  peersTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  layout->addWidget(peersTable_);

  networkMapLabel_ = new QLabel(w);
  networkMapLabel_->setWordWrap(true);
  layout->addWidget(networkMapLabel_);

  layout->addStretch(1);
  return w;
}

QString MainWindow::trackerIp() const {
  return discoveredPeersCsv_.trimmed();
}

int MainWindow::trackerPort() const {
  return 0;
}

void MainWindow::setBusy(bool busy) {
  progressBar_->setVisible(busy);
  if (busy) {
    progressBar_->setRange(0, 0);
  } else {
    progressBar_->setRange(0, 100);
    progressBar_->setValue(100);
  }
}

void MainWindow::log(const QString& text) {
  logEdit_->appendPlainText(text);
}

void MainWindow::onProgress(int done, int total, const QString& message) {
  if (total > 0) {
    progressBar_->setRange(0, total);
    progressBar_->setValue(done);
    progressBar_->setFormat(tr("%1 / %2 — %3").arg(done).arg(total).arg(message));
  }
}

void MainWindow::onPutClicked() {
  if (trackerIp().isEmpty()) {
    QMessageBox::warning(this, tr("Put"), tr("No peers discovered. Start at least one bootstrap node."));
    return;
  }
  QString path = putFileEdit_->text().trimmed();
  if (path.isEmpty()) {
    QMessageBox::warning(this, tr("Put"), tr("Enter file path."));
    return;
  }
  quint64 chunkSize = 1024 * 1024;
  QString chunkStr = putChunkEdit_->text().trimmed();
  if (!chunkStr.isEmpty()) {
    bool ok;
    chunkSize = chunkStr.toULongLong(&ok);
    if (!ok || chunkSize == 0) chunkSize = 1024 * 1024;
  }
  setBusy(true);
  log(tr("[Put] Started: %1").arg(path));
  const QString rsaPub = putRsaPublicEdit_ ? putRsaPublicEdit_->text().trimmed() : QString();
  if (!rsaPub.isEmpty()) {
    log(tr("[Put] Hybrid encryption enabled (RSA public key)."));
  }
  //QMetaObject::invokeMethod with Qt::QueuedConnection posts a
  //cross-thread call safely without blocking the GUI thread.
  QMetaObject::invokeMethod(worker_, "putFile", Qt::QueuedConnection,
                            Q_ARG(QString, trackerIp()),
                            Q_ARG(QString, path),
                            Q_ARG(quint64, chunkSize),
                            Q_ARG(int, putReplicasSpin_->value()),
                            Q_ARG(QString, rsaPub));
}

void MainWindow::onPutFinished(const QString& shareId, const QString& localManifestPath) {
  setBusy(false);
  log(tr("[Put] Done. Share ID: %1").arg(shareId));
  if (!localManifestPath.isEmpty()) {
    log(tr("[Put] Local manifest: %1").arg(localManifestPath));
  }
  QMessageBox::information(
      this, tr("Put"),
      tr("Share ID (use for Get):\n%1\n\nLocal manifest copy:\n%2").arg(shareId, localManifestPath));

  if (!filesTable_) return;
  const int row = filesTable_->rowCount();
  filesTable_->insertRow(row);

  auto* shareItem = new QTableWidgetItem(shareId);
  filesTable_->setItem(row, 0, shareItem);

  QString nameText;
  QString sizeText;
  if (QFile::exists(localManifestPath)) {
    try {
      dss::Manifest m = dss::readManifest(localManifestPath.toStdString());
      nameText = QString::fromStdString(m.originalName);
      sizeText = QString::number(static_cast<qulonglong>(m.originalSize));
    } catch (const std::exception& e) {
      log(tr("Failed to read local manifest for table: %1").arg(QString::fromUtf8(e.what())));
      nameText = tr("(unknown)");
      sizeText = QStringLiteral("-");
    }
  } else {
    nameText = tr("(unknown)");
    sizeText = QStringLiteral("-");
  }
  filesTable_->setItem(row, 1, new QTableWidgetItem(nameText));
  filesTable_->setItem(row, 2, new QTableWidgetItem(sizeText));

  auto* btn = new QPushButton(tr("Download"), filesTable_);
  btn->setProperty("shareId", shareId);
  filesTable_->setCellWidget(row, 3, btn);
  connect(btn, &QPushButton::clicked, this, &MainWindow::onDownloadButtonClicked);
}

void MainWindow::onGetClicked() {
  if (trackerIp().isEmpty()) {
    QMessageBox::warning(this, tr("Get"), tr("No peers discovered. Start at least one bootstrap node."));
    return;
  }
  QString manifest = getManifestEdit_->text().trimmed();
  QString output = getOutputEdit_->text().trimmed();
  if (manifest.isEmpty() || output.isEmpty()) {
    QMessageBox::warning(this, tr("Get"), tr("Enter share ID (or manifest path) and output path."));
    return;
  }
  setBusy(true);
  log(tr("[Get] Started: %1 → %2").arg(manifest, output));
  const QString rsaPrv = getRsaPrivateEdit_ ? getRsaPrivateEdit_->text().trimmed() : QString();
  QMetaObject::invokeMethod(worker_, "getFile", Qt::QueuedConnection,
                            Q_ARG(QString, trackerIp()),
                            Q_ARG(QString, manifest),
                            Q_ARG(QString, output),
                            Q_ARG(QString, rsaPrv));
}

void MainWindow::onGetFinished(const QString& outputPath) {
  setBusy(false);
  log(tr("[Get] Done: %1").arg(outputPath));
  QMessageBox::information(this, tr("Get"), tr("Download complete:\n%1").arg(outputPath));
}

void MainWindow::onRepairClicked() {
  if (trackerIp().isEmpty()) {
    QMessageBox::warning(this, tr("Repair"), tr("No peers discovered. Start at least one bootstrap node."));
    return;
  }
  setBusy(true);
  log(tr("[Repair] Started."));
  QMetaObject::invokeMethod(worker_, "repair", Qt::QueuedConnection,
                            Q_ARG(QString, trackerIp()),
                            Q_ARG(int, repairReplicasSpin_->value()),
                            Q_ARG(int, repairBatchSpin_->value()));
}

void MainWindow::onRepairFinished() {
  setBusy(false);
  log(tr("[Repair] Done."));
  QMessageBox::information(this, tr("Repair"), tr("Repair finished."));
}

void MainWindow::onError(const QString& message) {
  setBusy(false);
  log(tr("Error: %1").arg(message));
  QMessageBox::critical(this, tr("Error"), message);
}

void MainWindow::onDownloadButtonClicked() {
  auto* btn = qobject_cast<QPushButton*>(sender());
  if (!btn) return;
  const QString shareId = btn->property("shareId").toString();
  if (shareId.isEmpty()) return;

  if (trackerIp().isEmpty()) {
    QMessageBox::warning(this, tr("Get"), tr("No peers discovered. Start at least one bootstrap node."));
    return;
  }

  QString suggested = shareId;
  if (suggested.startsWith(QStringLiteral("dss://file/"))) {
    suggested = suggested.mid(QStringLiteral("dss://file/").size());
  }
  if (suggested.endsWith(".manifest.txt")) {
    suggested.chop(QString(".manifest.txt").size());
  }
  const QString output = QFileDialog::getSaveFileName(this,
                                                      tr("Select output file"),
                                                      suggested);
  if (output.isEmpty()) return;

  setBusy(true);
  log(tr("[Get] Started: %1 → %2").arg(shareId, output));
  const QString rsaPrv = getRsaPrivateEdit_ ? getRsaPrivateEdit_->text().trimmed() : QString();
  QMetaObject::invokeMethod(worker_, "getFile", Qt::QueuedConnection,
                            Q_ARG(QString, trackerIp()),
                            Q_ARG(QString, shareId),
                            Q_ARG(QString, output),
                            Q_ARG(QString, rsaPrv));
}

void MainWindow::refreshPeerMonitor() {
  if (!peersTable_) return;

  discoverPeersFromBootstrap();

  peersTable_->setRowCount(0);
  QString peersText = trackerIp();
  const auto parts = peersText.split(',', Qt::SkipEmptyParts);

  QString mapText;

  int row = 0;
  for (const QString& part : parts) {
    QString peerStr = part.trimmed();
    if (peerStr.isEmpty()) continue;
    const int colon = peerStr.indexOf(':');
    if (colon <= 0 || colon == peerStr.size() - 1) continue;
    const QString ip = peerStr.left(colon);
    const int port = peerStr.mid(colon + 1).toInt();

    // connectivity check
    QTcpSocket sock;
    sock.connectToHost(ip, static_cast<quint16>(port));
    bool ok = sock.waitForConnected(300);
    sock.abort();

    QString status = ok ? tr("Online") : tr("Offline");

    peersTable_->insertRow(row);
    peersTable_->setItem(row, 0, new QTableWidgetItem(peerStr));
    peersTable_->setItem(row, 1, new QTableWidgetItem(status));

    // Node ID prefix via DHT id helper
    dss::PeerEndpoint ep;
    ep.ip = ip.toStdString();
    ep.port = port;
    auto id = dss::dht::makeNodeId(ep);
    QString idHex = QString::fromStdString(dss::dht::toHex(id)).left(8);
    peersTable_->setItem(row, 2, new QTableWidgetItem(idHex));

    mapText += QString("• %1 — ID %2 (%3)\n").arg(peerStr, idHex, status);
    ++row;
  }

  if (networkMapLabel_) {
    if (mapText.isEmpty()) {
      networkMapLabel_->setText(tr("No peers configured."));
    } else {
      networkMapLabel_->setText(mapText.trimmed());
    }
  }
}

void MainWindow::discoverPeersFromBootstrap() {
  QStringList merged;

  for (const QString& peerStrRaw : bootstrapNodes_) {
    const QString peerStr = peerStrRaw.trimmed();
    if (peerStr.isEmpty()) continue;
    const int colon = peerStr.indexOf(':');
    if (colon <= 0 || colon == peerStr.size() - 1) continue;
    const QString ip = peerStr.left(colon);
    const int port = peerStr.mid(colon + 1).toInt();
    if (port <= 0) continue;

    QTcpSocket sock;
    sock.connectToHost(ip, static_cast<quint16>(port));
    const bool ok = sock.waitForConnected(400);
    sock.abort();
    if (ok) merged.push_back(peerStr);
  }

  if (peerEnabled_ && embeddedPeerService_ && embeddedPeerService_->isRunning()) {
    const QString local = QStringLiteral("%1:%2").arg(peerAdvertiseIp_, QString::number(peerPort_));
    if (!merged.contains(local)) merged.push_back(local);
  }

  merged.removeDuplicates();
  discoveredPeersCsv_ = merged.join(',');
  if (trackerIpEdit_) {
    trackerIpEdit_->setText(discoveredPeersCsv_);
  }
  if (bootstrapStatusLabel_) {
    if (!peerEnabled_) {
      if (merged.isEmpty()) {
        bootstrapStatusLabel_->setText(
            tr("Client-only: no bootstrap peers reachable. Edit network/bootstrap_seeds in settings "
               "or start remote peers."));
      } else {
        bootstrapStatusLabel_->setText(
            tr("Client-only: routing via %1 bootstrap peer(s).").arg(merged.size()));
      }
    } else if (merged.isEmpty()) {
      bootstrapStatusLabel_->setText(
          tr("No reachable peers. Embedded peer on port %1 (max storage %2 MiB) — add bootstrap "
             "seeds if you need remote routing.")
              .arg(peerPort_)
              .arg(peerMaxBytes_ / (1024 * 1024)));
    } else {
      const bool hasLocal =
          embeddedPeerService_ && embeddedPeerService_->isRunning();
      bootstrapStatusLabel_->setText(
          tr("Routing: %1 peer(s) from bootstrap%2. Local contribution: port %3, max %4 MiB.")
              .arg(merged.size())
              .arg(hasLocal ? tr(" + local") : QString())
              .arg(peerPort_)
              .arg(peerMaxBytes_ / (1024 * 1024)));
    }
  }
}

void MainWindow::startEmbeddedPeerIfEnabled() {
  if (!peerEnabled_) return;
  if (embeddedPeerService_ && embeddedPeerService_->isRunning()) return;
  if (peerStorageDir_.isEmpty()) return;

  std::error_code ec;
  std::filesystem::create_directories(peerStorageDir_.toStdString(), ec);

  try {
    embeddedPeerService_ = std::make_unique<dss::peer::PeerService>(
        peerAdvertiseIp_.toStdString(),
        peerPort_,
        peerStorageDir_.toStdString(),
        peerMaxBytes_);
    embeddedPeerService_->start();
  } catch (const std::exception& e) {
    log(tr("Failed to start embedded peer: %1").arg(QString::fromUtf8(e.what())));
    embeddedPeerService_.reset();
  }
}

void MainWindow::stopEmbeddedPeer() {
  if (!embeddedPeerService_) return;
  embeddedPeerService_->stop();
  embeddedPeerService_.reset();
}

void MainWindow::loadOrPromptPeerSettings() {
  QSettings settings(QStringLiteral("decent_store"), QStringLiteral("decent_store"));

  const bool initialized = settings.value(QStringLiteral("peer/settings_initialized"), false).toBool();
  if (!initialized) {
    const auto answer = QMessageBox::question(
        this,
        tr("Peer Contribution"),
        tr("Enable peer contribution mode?\n"
           "When enabled, this app can contribute storage and participate in the network."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    peerEnabled_ = (answer == QMessageBox::Yes);

    int gib = QInputDialog::getInt(
        this,
        tr("Storage Contribution"),
        tr("Max local storage contribution (GiB):"),
        5, 1, 1024, 1);
    peerMaxBytes_ = static_cast<std::uint64_t>(gib) * 1024ull * 1024ull * 1024ull;

    int port = QInputDialog::getInt(
        this,
        tr("Peer Port"),
        tr("Local peer port:"),
        9101, 1024, 65535, 1);
    peerPort_ = port;

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty()) appData = QStringLiteral(".");
    peerStorageDir_ = appData + QStringLiteral("/peer-storage");

    settings.setValue(QStringLiteral("peer/settings_initialized"), true);
    settings.setValue(QStringLiteral("peer/enabled"), peerEnabled_);
    settings.setValue(QStringLiteral("peer/max_bytes"), static_cast<qulonglong>(peerMaxBytes_));
    settings.setValue(QStringLiteral("peer/port"), peerPort_);
    settings.setValue(QStringLiteral("peer/storage_dir"), peerStorageDir_);
    if (!settings.contains(QStringLiteral("network/bootstrap_seeds"))) {
      settings.setValue(
          QStringLiteral("network/bootstrap_seeds"),
          QStringLiteral("127.0.0.1:9101,127.0.0.1:9102,127.0.0.1:9103"));
    }
    if (!settings.contains(QStringLiteral("network/advertise_ip"))) {
      settings.setValue(QStringLiteral("network/advertise_ip"), QStringLiteral("127.0.0.1"));
    }
  } else {
    peerEnabled_ = settings.value(QStringLiteral("peer/enabled"), true).toBool();
    peerMaxBytes_ =
        settings.value(QStringLiteral("peer/max_bytes"),
                       static_cast<qulonglong>(5ull * 1024ull * 1024ull * 1024ull))
            .toULongLong();
    peerPort_ = settings.value(QStringLiteral("peer/port"), 9101).toInt();
    peerStorageDir_ = settings.value(QStringLiteral("peer/storage_dir")).toString();
    if (peerStorageDir_.isEmpty()) {
      QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
      if (appData.isEmpty()) appData = QStringLiteral(".");
      peerStorageDir_ = appData + QStringLiteral("/peer-storage");
    }
  }

  loadBootstrapSeedsFromSettings();

  {
    QSettings s(QStringLiteral("decent_store"), QStringLiteral("decent_store"));
    peerAdvertiseIp_ =
        s.value(QStringLiteral("network/advertise_ip"), QStringLiteral("127.0.0.1")).toString().trimmed();
    if (peerAdvertiseIp_.isEmpty()) peerAdvertiseIp_ = QStringLiteral("127.0.0.1");
  }
}

void MainWindow::onApplyNetworkSettingsClicked() {
  if (!bootstrapSeedsEdit_ || !advertiseIpEdit_) return;

  QString adv = advertiseIpEdit_->text().trimmed();
  if (adv.isEmpty()) {
    QMessageBox::warning(this, tr("Advertise address"),
                         tr("Advertise address cannot be empty."));
    return;
  }

  const QString defaultSeeds =
      QStringLiteral("127.0.0.1:9101,127.0.0.1:9102,127.0.0.1:9103");
  QString seeds = bootstrapSeedsEdit_->text().trimmed();
  if (seeds.isEmpty()) {
    seeds = defaultSeeds;
    bootstrapSeedsEdit_->setText(seeds);
  }

  QSettings settings(QStringLiteral("decent_store"), QStringLiteral("decent_store"));
  settings.setValue(QStringLiteral("network/bootstrap_seeds"), seeds);
  settings.setValue(QStringLiteral("network/advertise_ip"), adv);

  peerAdvertiseIp_ = adv;
  loadBootstrapSeedsFromSettings();

  const bool hadRunning = embeddedPeerService_ && embeddedPeerService_->isRunning();
  if (hadRunning) stopEmbeddedPeer();
  startEmbeddedPeerIfEnabled();
  discoverPeersFromBootstrap();
  log(tr("Network settings saved; bootstrap list and advertise address updated."));
}

void MainWindow::loadBootstrapSeedsFromSettings() {
  QSettings settings(QStringLiteral("decent_store"), QStringLiteral("decent_store"));
  const QString defaultSeeds =
      QStringLiteral("127.0.0.1:9101,127.0.0.1:9102,127.0.0.1:9103");
  const QString seeds =
      settings.value(QStringLiteral("network/bootstrap_seeds"), defaultSeeds).toString();
  bootstrapNodes_.clear();
  for (const QString& part : seeds.split(',', Qt::SkipEmptyParts)) {
    const QString t = part.trimmed();
    if (!t.isEmpty()) bootstrapNodes_.push_back(t);
  }
  if (bootstrapNodes_.isEmpty()) {
    for (const QString& part : defaultSeeds.split(',', Qt::SkipEmptyParts))
      bootstrapNodes_.push_back(part.trimmed());
  }
}
