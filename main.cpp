#include "CommandInterpreter.h"
#include "VirtualKeyboard.h"
#include <cstring>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
  const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
  if (!runtime_dir) {
    std::cerr << "XDG_RUNTIME_DIR not set\n";
    return 1;
  }

  std::string sock_path = std::string(runtime_dir) + "/domacro.sock";

  int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return 1;
  }

  unlink(sock_path.c_str());

  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(server_fd);
    return 1;
  }

  if (listen(server_fd, 5) < 0) {
    perror("listen");
    close(server_fd);
    return 1;
  }

  int virtualkeys = createAndEnableKeys();
  if (virtualkeys < 0)
    return 1;

  createUinputDevice(virtualkeys);
  CommandInterpreter interpreter(virtualkeys);

  while (true) {
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    char buffer[256];
    ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    close(client_fd);

    if (n <= 0)
      continue;

    buffer[n] = '\0';
    std::string command(buffer);
    if (!command.empty() && command.back() == '\n')
      command.pop_back();

    if (command == "exit") {
      ioctl(virtualkeys, UI_DEV_DESTROY);
      break;
    }

    interpreter.interpret(command);
  }

  close(server_fd);
  unlink(sock_path.c_str());
  close(virtualkeys);
  return 0;
}
