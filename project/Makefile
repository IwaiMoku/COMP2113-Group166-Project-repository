# COMP2113 Group Project - STARFALL: The Nyx-9 Incident
#
# Build with the exact flags required by the project specification.
# Run `make` in the repository root, then `./main`.

CXX      = g++
CXXFLAGS = -pedantic-errors -std=c++11 -Wall -Wextra -O2
TARGET   = main
SRCS     = $(wildcard *.cpp)
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Every translation unit depends on every header; the project is small and
# this keeps the Makefile simple and always correct.
%.o: %.cpp $(wildcard *.h)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) nyx9_save.txt

.PHONY: all clean run
