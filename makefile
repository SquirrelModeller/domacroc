CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

DAEMON_SRC = main.cpp CommandInterpreter.cpp VirtualKeyboard.cpp
DAEMON_OBJ = $(DAEMON_SRC:.cpp=.o)
DAEMON = domacroc

CLIENT_SRC = client.cpp
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)
CLIENT = domacro-send

all: $(DAEMON) $(CLIENT)

$(DAEMON): $(DAEMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $(DAEMON) $(DAEMON_OBJ)

$(CLIENT): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $(CLIENT) $(CLIENT_OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(DAEMON_OBJ) $(CLIENT_OBJ) $(DAEMON) $(CLIENT)

.PHONY: all clean
