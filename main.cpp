#include "CommandInterpreter.h"
#include "VirtualKeyboard.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <unistd.h>

int main()
{
  const char *pipe_path = "/tmp/domacro";
  mkfifo(pipe_path, 0666);
  int fd = open(pipe_path, O_RDWR);

  if (fd < 0) {
    perror("Failed to open pipe");
    return 1;
  }

  FILE *pipe = fdopen(fd, "r");
  if (!pipe) {
    perror("fdopen failed");
    close(fd);
    return 1;
  }

  char buffer[256];
  int virtualkeys = createAndEnableKeys();
  if (virtualkeys < 0) return 1;

  createUinputDevice(virtualkeys);
  CommandInterpreter interpreter(virtualkeys);

  while (fgets(buffer, sizeof(buffer), pipe)) {
    std::string command(buffer);
    if (command.back() == '\n') command.pop_back();

    if (command == "exit") {
      ioctl(virtualkeys, UI_DEV_DESTROY);
      break;
    }
    interpreter.interpret(command);
  }

  fclose(pipe);
  close(fd);
  close(virtualkeys);
}
