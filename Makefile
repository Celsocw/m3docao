CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Isrc/header
LDFLAGS = 

# Source files (moved to src/impl)
SOURCES = src/impl/fs_sim.cpp src/impl/fcb.cpp src/impl/file_system.cpp src/impl/cliente.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = fs_sim

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

# Compile object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
