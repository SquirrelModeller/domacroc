#include "CommandInterpreter.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

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
  keyHold(fd, std::stoi(keycode), time);
}

void CommandInterpreter::handleKeyPress(std::stringstream &args)
{
  std::string keycode;
  args >> keycode;
  if (!args) {
    std::cerr << "Error: Invalid arguments for keyPress\n";
    return;
  }
  keyPress(fd, std::stoi(keycode));
}

void CommandInterpreter::handleKeyRelease(std::stringstream &args)
{
  std::string keycode;
  args >> keycode;
  if (!args) {
    std::cerr << "Error: Invalid arguments for keyRelease\n";
    return;
  }
  keyRelease(fd, std::stoi(keycode));
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
