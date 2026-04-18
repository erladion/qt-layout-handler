#include <QApplication>
#include "mainwindow.h"

#include <gst/gst.h>

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  QApplication app(argc, argv);
  MainWindow w;
  w.show();
  return app.exec();
}
