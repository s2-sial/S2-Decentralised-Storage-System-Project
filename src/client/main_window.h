#pragma once

#include <QMainWindow>
#include <QThread>

class DssWorker;
class QProgressBar;
class QPlainTextEdit;
class QLineEdit;
class QSpinBox;

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

private:
  void setupUi();
  QWidget* makePutTab();
  QWidget* makeGetTab();
  QWidget* makeRepairTab();
  QString trackerIp() const;
  int trackerPort() const;
  void setBusy(bool busy);

  QThread workerThread_;
  DssWorker* worker_{};
  QProgressBar* progressBar_{};
  QPlainTextEdit* logEdit_{};

  QLineEdit* trackerIpEdit_{};
  QSpinBox* trackerPortSpin_{};  // unused in DHT mode (kept for compatibility)

  QLineEdit* putFileEdit_{};
  QLineEdit* putChunkEdit_{};
  QSpinBox* putReplicasSpin_{};

  QLineEdit* getManifestEdit_{};
  QLineEdit* getOutputEdit_{};

  QSpinBox* repairReplicasSpin_{};
  QSpinBox* repairBatchSpin_{};
};
