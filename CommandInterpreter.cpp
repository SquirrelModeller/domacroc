#include "CommandInterpreter.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <linux/input-event-codes.h>

static const std::unordered_map<std::string, int> keyNames = {
  {"a", KEY_A}, {"b", KEY_B}, {"c", KEY_C}, {"d", KEY_D}, {"e", KEY_E},
  {"f", KEY_F}, {"g", KEY_G}, {"h", KEY_H}, {"i", KEY_I}, {"j", KEY_J},
  {"k", KEY_K}, {"l", KEY_L}, {"m", KEY_M}, {"n", KEY_N}, {"o", KEY_O},
  {"p", KEY_P}, {"q", KEY_Q}, {"r", KEY_R}, {"s", KEY_S}, {"t", KEY_T},
  {"u", KEY_U}, {"v", KEY_V}, {"w", KEY_W}, {"x", KEY_X}, {"y", KEY_Y},
  {"z", KEY_Z},
  {"f1", KEY_F1}, {"f2", KEY_F2}, {"f3", KEY_F3}, {"f4", KEY_F4},
  {"f5", KEY_F5}, {"f6", KEY_F6}, {"f7", KEY_F7}, {"f8", KEY_F8},
  {"f9", KEY_F9}, {"f10", KEY_F10}, {"f11", KEY_F11}, {"f12", KEY_F12},
  {"enter", KEY_ENTER}, {"space", KEY_SPACE}, {"tab", KEY_TAB},
  {"backspace", KEY_BACKSPACE}, {"esc", KEY_ESC}, {"escape", KEY_ESC},
  {"delete", KEY_DELETE}, {"insert", KEY_INSERT},
  {"home", KEY_HOME}, {"end", KEY_END},
  {"pageup", KEY_PAGEUP}, {"pagedown", KEY_PAGEDOWN},
  {"up", KEY_UP}, {"down", KEY_DOWN}, {"left", KEY_LEFT}, {"right", KEY_RIGHT},
  {"lctrl", KEY_LEFTCTRL}, {"rctrl", KEY_RIGHTCTRL},
  {"lshift", KEY_LEFTSHIFT}, {"rshift", KEY_RIGHTSHIFT},
  {"lalt", KEY_LEFTALT}, {"ralt", KEY_RIGHTALT},
  {"lmeta", KEY_LEFTMETA}, {"rmeta", KEY_RIGHTMETA},
  {"capslock", KEY_CAPSLOCK}, {"numlock", KEY_NUMLOCK},
  {"minus", KEY_MINUS}, {"equal", KEY_EQUAL}, {"backslash", KEY_BACKSLASH},
  {"semicolon", KEY_SEMICOLON}, {"apostrophe", KEY_APOSTROPHE},
  {"grave", KEY_GRAVE}, {"comma", KEY_COMMA}, {"dot", KEY_DOT},
  {"slash", KEY_SLASH}, {"leftbrace", KEY_LEFTBRACE}, {"rightbrace", KEY_RIGHTBRACE},
};

static int resolveKey(const std::string &key)
{
  auto it = keyNames.find(key);
  if (it != keyNames.end())
    return it->second;
  return std::stoi(key);
}

CommandInterpreter::CommandInterpreter(int &fd) : fd(fd)
{
  commandMap["keyHold"] = [this](std::stringstream &args) { handleKeyHold(args); };
  commandMap["keyPress"] = [this](std::stringstream &args) { handleKeyPress(args); };
  commandMap["keyRelease"] = [this](std::stringstream &args) { handleKeyRelease(args); };
  commandMap["sleep"] = [this](std::stringstream &args) { handleSleep(args); };
}

void CommandInterpreter::interpret(const std::string &command)
{
  std::stringstream ss(command);
  std::string commandName, macroname;

  ss >> commandName;
  if (commandName == "unique") {
    ss >> macroname;
    {
      std::lock_guard<std::mutex> lock(macro_mutex);
      if (std::find(running_macros.begin(), running_macros.end(), macroname) != running_macros.end())
	return;
      running_macros.push_back(macroname);
    }
  } else {
    ss.seekg(ss.beg);
  }

  std::thread([this, ss = std::move(ss), macroname]() mutable {
    this->executeMacro(std::move(ss), macroname);
  }).detach();
}

void CommandInterpreter::handleKeyHold(std::stringstream &args)
{
  std::string keycode;
  int time;
  args >> keycode >> time;
  if (!args) {
    std::cerr << "Error: Invalid arguments for keyHold\n";
    return;
  }
  keyHold(fd, resolveKey(keycode), time);
}

void CommandInterpreter::handleKeyPress(std::stringstream &args)
{
  std::string keycode;
  args >> keycode;
  if (!args) {
    std::cerr << "Error: Invalid arguments for keyPress\n";
    return;
  }
  keyPress(fd, resolveKey(keycode));
}

void CommandInterpreter::handleKeyRelease(std::stringstream &args)
{
  std::string keycode;
  args >> keycode;
  if (!args) {
    std::cerr << "Error: Invalid arguments for keyRelease\n";
    return;
  }
  keyRelease(fd, resolveKey(keycode));
}

void CommandInterpreter::handleSleep(std::stringstream &args)
{
  int time;
  args >> time;
  if (!args) {
    std::cerr << "Error: Invalid arguments for sleep\n";
    return;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(time));
}

void CommandInterpreter::executeMacro(std::stringstream ss, const std::string &macroname)
{
  std::string commandName;
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
