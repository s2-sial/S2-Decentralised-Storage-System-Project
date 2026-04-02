#include "main_window.h"

#include <QApplication>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("decent_store"));
  QApplication::setOrganizationName(QStringLiteral("decent_store"));
  QApplication::setApplicationDisplayName(QStringLiteral("Decent Store"));
  QApplication::setApplicationVersion(QStringLiteral("0.1"));

  MainWindow w;
  w.show();
  return app.exec();
}
