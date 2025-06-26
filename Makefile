# Compiler
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Target executable
TARGET = csim

# Source files
SRCS = csim.cpp BP.cpp RAT.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)
