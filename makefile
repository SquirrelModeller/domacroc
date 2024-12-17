CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread
LDFLAGS = -lrt

SRC = main.cpp CommandInterpreter.cpp VirtualKeyboard.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = macro_program

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
