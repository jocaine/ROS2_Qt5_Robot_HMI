/*
 * @Author: chengyangkj chengyangkj@qq.com
 * @Date: 2023-09-28 14:56:04
 * @LastEditors: chengyangkj chengyangkj@qq.com
 * @LastEditTime: 2023-10-05 11:39:01
 * @FilePath: /ROS2_Qt5_Gui_App/src/main.cpp
 */
#include <QApplication>
#include <QLabel>
#include <QMovie>
#include <QPixmap>
#include <QSocketNotifier>
#include <QSplashScreen>
#include <QThread>
#include <csignal>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include "logger/logger.h"
#include "runtime/application_manager.h"

static int g_sig_pipe[2] = {-1, -1};

void signalHandler(int) {
  char byte = 1;
  (void)write(g_sig_pipe[1], &byte, 1);
}

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  if (pipe(g_sig_pipe) != 0) {
    std::cerr << "Failed to create signal pipe: " << strerror(errno) << std::endl;
    return 1;
  }
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  QSocketNotifier notifier(g_sig_pipe[0], QSocketNotifier::Read);
  QObject::connect(&notifier, &QSocketNotifier::activated, [&]() {
    char byte;
    (void)read(g_sig_pipe[0], &byte, 1);
    close(g_sig_pipe[0]);
    close(g_sig_pipe[1]);
    a.quit();
  });

  ApplicationManager manager_;
  LOG_INFO("ros_qt5_gui_app init!")
  return a.exec();
}
