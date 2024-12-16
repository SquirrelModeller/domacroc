#define _GLIBCXX_USE_NANOSLEEP
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <mutex>

int create_and_enable_keys()
{
  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

  if (fd < 0) {
    perror("Error opening /dev/uinput");
    return -1;
  }

  if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
    perror("Error enabling EV_KEY");
    close(fd);
    return -1;
  }

  for (int i = 0; i <= 248; i++) {
    if (ioctl(fd, UI_SET_KEYBIT, i) < 0) {
      perror("Error enabling key");
      close(fd);
      return -1;
    }
  }

  return fd;
}

void emit(int fd, int type, int code, int val, int timestamp)
{
  struct input_event ie;

  ie.type = type;
  ie.code = code;
  ie.value = val;

  ie.time.tv_sec = timestamp;

  write(fd, &ie, sizeof(ie));
}

uinput_user_dev create_uinput_device(int &fd)
{
  struct uinput_user_dev uidev;
  memset(&uidev, 0, sizeof(uidev));

  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-macro-device");
  uidev.id.bustype = BUS_VIRTUAL;
  uidev.id.vendor = 0x1234;
  uidev.id.product = 0x5678;
  uidev.id.version = 1;

  if (write(fd, &uidev, sizeof(uidev)) < 0) {
    perror("Error writing device information");
  }

  if (ioctl(fd, UI_DEV_CREATE) < 0) {
    perror("Error creating uinput device");
  }

  return uidev;
}

void keyPress(int &fd, int key_code)
{
  emit(fd, EV_KEY, key_code, 1, 0);
  emit(fd, EV_SYN, SYN_REPORT, 0, 1);
}

void keyRelease(int &fd, int key_code)
{
  emit(fd, EV_KEY, key_code, 0, 2);
  emit(fd, EV_SYN, SYN_REPORT, 0, 3);
}

void keyHold(int &fd, int key_code,  int hold_milliseconds = 0)
{
  keyPress(fd, key_code);
  if (hold_milliseconds != 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(hold_milliseconds));
  }
  keyRelease(fd, key_code);
}

void sleepFor(int milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

class CommandInterpreter {
public:
  int& fd;
  std::unordered_map<std::string, std::function<void(std::stringstream&)>> commandMap;
  std::list<std::string> running_macros;
  std::mutex macro_mutex;

  CommandInterpreter(int& fd) : fd(fd) {
    commandMap["keyHold"] = [this](std::stringstream& args) { handleKeyHold(args); };
    commandMap["keyPress"] = [this](std::stringstream& args) { handleKeyPress(args); };
    commandMap["keyRelease"] = [this](std::stringstream& args) { handleKeyRelease(args); };
    commandMap["sleep"] = [this](std::stringstream& args) { handleSleep(args); };
  }

  void interpret(const std::string& command)
  {
    std::stringstream ss(command);
    std::string commandName;
    std::string macroname = "";

    ss >> commandName;
    if (commandName == "unique") {
      ss >> macroname;
      {
	std::lock_guard<std::mutex> lock(macro_mutex);
	auto macro = std::find(running_macros.begin(), running_macros.end(), macroname);
	if (macro != running_macros.end()) {
	  macroname = "";
	  return;
	}
	running_macros.push_back(macroname);
      }
    } else {
      ss.seekg(ss.beg);
    }

    std::thread([this, ss = std::move(ss), macroname]() mutable {
      this->executeMacro(std::move(ss), macroname);
    }).detach();
  }

private:
  void executeMacro(std::stringstream ss, const std::string& macroname)
  {
    std::string commandName = "";

    while (ss >> commandName) {
      auto it = commandMap.find(commandName);
      if (it != commandMap.end()) {
	it->second(ss);
      } else {
	std::cerr << "Unknown command: " << commandName << std::endl;
	ss.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
    }

    if (!macroname.empty()) {
      std::lock_guard<std::mutex> lock(macro_mutex);
      running_macros.remove(macroname);
    }
  }

  void handleKeyHold(std::stringstream& args)
  {
    std::string keycode;
    int time;

    args >> keycode >> time;
    if (!args) {
      std::cerr << "Error: Invalid arguments for keyHold\n";
      return;
    }

    keyHold(fd, std::stoi(keycode), time);
  }

  void handleKeyPress(std::stringstream& args)
  {
    std::string keycode;

    args >> keycode;
    if (!args) {
      std::cerr << "Error: Invalid arguments for keyPress\n";
      return;
    }

    keyPress(fd, std::stoi(keycode));
  }

  void handleKeyRelease(std::stringstream& args)
  {
    std::string keycode;

    args >> keycode;
    if (!args) {
      std::cerr << "Error: Invalid arguments for keyRelease\n";
      return;
    }

    keyRelease(fd, std::stoi(keycode));
  }

  void handleSleep(std::stringstream& args)
  {
    int time;

    args >> time;
    if (!args) {
      std::cerr << "Error: Invalid arguments for sleep\n";
      return;
    }

    sleepFor(time);
  }
};

int main()
{
  const char* pipe_path = "/tmp/domacro";
  mkfifo(pipe_path, 0666);

  int fd = open(pipe_path, O_RDWR);
  
  if (fd < 0) {
    perror("Failed to open pipe");
  }

  FILE* pipe = fdopen(fd, "r");
  if (!pipe) {
    perror("fdopen failed");
    close(fd);
  }

  char buffer[256];

  int virtualkeys = create_and_enable_keys();
  if (virtualkeys < 0) {
    return 1;
  }
  create_uinput_device(virtualkeys);

  CommandInterpreter interpreter(virtualkeys);

  while (fgets(buffer, sizeof(buffer), pipe)) {
    std::string command(buffer);

    if (!command.empty() && command.back() == '\n') {
      command.pop_back();
    }

    if (command == "exit") {
      if (ioctl(virtualkeys, UI_DEV_DESTROY) < 0) {
        perror("Error destroying uinput device");
      }

      fclose(pipe);
      close(virtualkeys);
      close(fd);
      return 0;
    }

    interpreter.interpret(command);

  }
}
