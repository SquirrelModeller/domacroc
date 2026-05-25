#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: domacro-send <command> [args...]\n";
    return 1;
  }

  const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
  if (!runtime_dir) {
    std::cerr << "XDG_RUNTIME_DIR not set\n";
    return 1;
  }

  std::string sock_path = std::string(runtime_dir) + "/domacro.sock";

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    close(fd);
    return 1;
  }

  std::string command;
  for (int i = 1; i < argc; i++) {
    if (i > 1)
      command += ' ';
    command += argv[i];
  }

  send(fd, command.c_str(), command.size(), 0);
  close(fd);
  return 0;
}
