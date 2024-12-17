#ifndef COMMAND_INTERPRETER_H
#define COMMAND_INTERPRETER_H

#include <string>
#include <unordered_map>
#include <functional>
#include <list>
#include <sstream>
#include <mutex>
#include "VirtualKeyboard.h"

class CommandInterpreter
{
public:
  int &fd;
  CommandInterpreter(int &fd);
  void interpret(const std::string &command);

private:
  std::unordered_map<std::string, std::function<void(std::stringstream &)>> commandMap;
  std::list<std::string> running_macros;
  std::mutex macro_mutex;

  void executeMacro(std::stringstream ss, const std::string &macroname);
  void handleKeyHold(std::stringstream &args);
  void handleKeyPress(std::stringstream &args);
  void handleKeyRelease(std::stringstream &args);
  void handleSleep(std::stringstream &args);
};

#endif // COMMAND_INTERPRETER_H
