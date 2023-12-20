#include <signal.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "Logger.hpp"
#include "Tracker.hpp"

using namespace std;

Kapua::IOStreamLogger* stdlog;
Kapua::Tracker* tracker;

volatile bool running = true;
volatile bool stopping = false;

void signal_stop(int signum) {
  stdlog->info("SIGINT Recieved");
  if (!stopping) {
    running = false;
    stopping = true;

  } else {
    stdlog->warn("Hard shutdown!");
    exit(1);
  }
}

void start() {
  tracker = new Kapua::Tracker(stdlog, 8080);
  tracker->start();
}

void stop() {
  tracker->stop();
  delete tracker;
}

int main() {
  stdlog = new Kapua::IOStreamLogger(&cout, Kapua::LOG_LEVEL_DEBUG);

  stdlog->info("kapua Server v0.0.1");
  stdlog->info("-------------------");

  stdlog->debug("Starting...");
  start();
  stdlog->info("Started");

  signal(SIGINT, signal_stop);

  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  stdlog->debug("Stopping...");
  stop();
  stdlog->info("Stopped");

  delete stdlog;

  return EXIT_SUCCESS;
}