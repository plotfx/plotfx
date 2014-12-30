/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef FNORDMETRIC_EV_EVENTLOOP_H
#define FNORDMETRIC_EV_EVENTLOOP_H

#include <sys/select.h>
#include <vector>
#include "fnord/thread/taskscheduler.h"

namespace fnord {
namespace thread {

class EventLoop : public TaskScheduler {
public:
  void run(std::function<void()> task) override;
  void runOnReadable(std::function<void()> task, int fd) override;
  void runOnWritable(std::function<void()> task, int fd) override;
  void runOnWakeup(
      std::function<void()> task,
      Wakeup* wakeup,
      long wakeup_generation) override;

  EventLoop();
  void run();

protected:

  void poll();

  fd_set op_read_;
  fd_set op_write_;
  fd_set op_error_;
  int max_fd_;
  volatile bool running_;
  std::vector<std::function<void()>> callbacks_;
};

}
}
#endif
