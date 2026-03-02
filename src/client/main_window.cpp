#include "main_window.h"
#include "dss_worker.h"

#include <QApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  worker_ = new DssWorker;
  worker_->moveToThread(&workerThread_);
  connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

  connect(worker_, &DssWorker::progress, this, &MainWindow::onProgress, Qt::QueuedConnection);
  connect(worker_, &DssWorker::putFinished, this, &MainWindow::onPutFinished, Qt::QueuedConnection);
  connect(worker_, &DssWorker::getFinished, this, &MainWindow::onGetFinished, Qt::QueuedConnection);
  connect(worker_, &DssWorker::repairFinished, this, &MainWindow::onRepairFinished, Qt::QueuedConnection);
  connect(worker_, &DssWorker::error, this, &MainWindow::onError, Qt::QueuedConnection);

  workerThread_.start();
  setupUi();
  setWindowTitle(tr("Decent Store — Client"));
  resize(520, 420);
}

MainWindow::~MainWindow() {
  workerThread_.quit();
  workerThread_.wait();
}

void MainWindow::setupUi() {
  QWidget* central = new QWidget(this);
  QVBoxLayout* mainLayout = new QVBoxLayout(central);

  QGroupBox* connGroup = new QGroupBox(tr("Peers"));
  QFormLayout* connLayout = new QFormLayout(connGroup);
  trackerIpEdit_ = new QLineEdit(this);
  trackerIpEdit_->setPlaceholderText(tr("ip:port,ip:port (comma separated)"));
  trackerIpEdit_->setText(QStringLiteral("127.0.0.1:9101"));
  connLayout->addRow(tr("Peer list:"), trackerIpEdit_);
  trackerPortSpin_ = nullptr;
  mainLayout->addWidget(connGroup);

  QTabWidget* tabs = new QTabWidget(this);
  tabs->addTab(makePutTab(), tr("Put"));
  tabs->addTab(makeGetTab(), tr("Get"));
  tabs->addTab(makeRepairTab(), tr("Repair"));
  mainLayout->addWidget(tabs);

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
  QFormLayout* form = new QFormLayout(w);
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
  QPushButton* btn = new QPushButton(tr("Put file"), this);
  connect(btn, &QPushButton::clicked, this, &MainWindow::onPutClicked);
  form->addRow(btn);
  return w;
}

QWidget* MainWindow::makeGetTab() {
  QWidget* w = new QWidget(this);
  QFormLayout* form = new QFormLayout(w);
  getManifestEdit_ = new QLineEdit(this);
  getManifestEdit_->setPlaceholderText(tr("Path to .manifest.txt"));
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
  QPushButton* btn = new QPushButton(tr("Get file"), this);
  connect(btn, &QPushButton::clicked, this, &MainWindow::onGetClicked);
  form->addRow(btn);
  return w;
}

QWidget* MainWindow::makeRepairTab() {
  QWidget* w = new QWidget(this);
  QFormLayout* form = new QFormLayout(w);
  repairReplicasSpin_ = new QSpinBox(this);
  repairReplicasSpin_->setRange(1, 32);
  repairReplicasSpin_->setValue(2);
  form->addRow(tr("Desired replicas:"), repairReplicasSpin_);
  repairBatchSpin_ = new QSpinBox(this);
  repairBatchSpin_->setRange(1, 1000);
  repairBatchSpin_->setValue(50);
  form->addRow(tr("Batch size:"), repairBatchSpin_);
  QPushButton* btn = new QPushButton(tr("Repair"), this);
  connect(btn, &QPushButton::clicked, this, &MainWindow::onRepairClicked);
  form->addRow(btn);
  return w;
}

QString MainWindow::trackerIp() const {
  return trackerIpEdit_->text().trimmed();
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
    QMessageBox::warning(this, tr("Put"), tr("Enter peers (ip:port,ip:port)."));
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
  QMetaObject::invokeMethod(worker_, "putFile", Qt::QueuedConnection,
                            Q_ARG(QString, trackerIp()),
                            Q_ARG(QString, path),
                            Q_ARG(quint64, chunkSize),
                            Q_ARG(int, putReplicasSpin_->value()));
}

void MainWindow::onPutFinished(const QString& manifestPath) {
  setBusy(false);
  log(tr("[Put] Done. Manifest: %1").arg(manifestPath));
  QMessageBox::information(this, tr("Put"), tr("Manifest written to:\n%1").arg(manifestPath));
}

void MainWindow::onGetClicked() {
  if (trackerIp().isEmpty()) {
    QMessageBox::warning(this, tr("Get"), tr("Enter peers (ip:port,ip:port)."));
    return;
  }
  QString manifest = getManifestEdit_->text().trimmed();
  QString output = getOutputEdit_->text().trimmed();
  if (manifest.isEmpty() || output.isEmpty()) {
    QMessageBox::warning(this, tr("Get"), tr("Enter manifest and output paths."));
    return;
  }
  setBusy(true);
  log(tr("[Get] Started: %1 → %2").arg(manifest, output));
  QMetaObject::invokeMethod(worker_, "getFile", Qt::QueuedConnection,
                            Q_ARG(QString, trackerIp()),
                            Q_ARG(QString, manifest),
                            Q_ARG(QString, output));
}

void MainWindow::onGetFinished(const QString& outputPath) {
  setBusy(false);
  log(tr("[Get] Done: %1").arg(outputPath));
  QMessageBox::information(this, tr("Get"), tr("Download complete:\n%1").arg(outputPath));
}

void MainWindow::onRepairClicked() {
  if (trackerIp().isEmpty()) {
    QMessageBox::warning(this, tr("Repair"), tr("Enter peers (ip:port,ip:port)."));
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
