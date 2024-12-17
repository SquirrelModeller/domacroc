#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include <linux/uinput.h>
#include <string.h>

int createAndEnableKeys();
struct uinput_user_dev createUinputDevice(int fd);
void emit(int fd, int type, int code, int val, int timestamp);
void keyPress(int fd, int key_code);
void keyRelease(int fd, int key_code);
void keyHold(int fd, int key_code, int hold_milliseconds);

#endif // VIRTUAL_KEYBOARD_H
