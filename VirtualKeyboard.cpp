#include "VirtualKeyboard.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <sys/ioctl.h>
#include <linux/input.h>

int createAndEnableKeys()
{
  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    perror("Error opening /dev/uinput");
    return -1;
  }

  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  for (int i = 0; i <= 248; i++) {
    ioctl(fd, UI_SET_KEYBIT, i);
  }
  return fd;
};

uinput_user_dev createUinputDevice(int fd)
{
  uinput_user_dev uidev{};
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-macro-device");
  uidev.id.bustype = BUS_VIRTUAL;
  uidev.id.vendor = 0x1234;
  uidev.id.product = 0x5678;
  uidev.id.version = 1;

  write(fd, &uidev, sizeof(uidev));
  ioctl(fd, UI_DEV_CREATE);
  return uidev;
}

void emit(int fd, int type, int code, int val, int timestamp)
{
  struct input_event ie{};
  ie.type = type;
  ie.code = code;
  ie.value = val;
  ie.time.tv_sec = timestamp;
  write(fd, &ie, sizeof(ie));
}

void keyPress(int fd, int key_code)
{
  emit(fd, EV_KEY, key_code, 1, 0);
  emit(fd, EV_SYN, SYN_REPORT, 0, 1);
}

void keyRelease(int fd, int key_code)
{
  emit(fd, EV_KEY, key_code, 0, 2);
  emit(fd, EV_SYN, SYN_REPORT, 0, 3);
}

void keyHold(int fd, int key_code, int hold_milliseconds)
{
  keyPress(fd, key_code);
  if (hold_milliseconds > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(hold_milliseconds));
  }
  keyRelease(fd, key_code);
}
