EXE := ipk-rdt
TEST_EXE := test-$(EXE)

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:.cpp=.o)

# TEST_SRCS := tests/test.cpp src/packets.cpp src/params.cpp src/network.cpp
# TEST_OBJS := $(TEST_SRCS:.cpp=.o)

CXX := g++
CXXFLAGS := -std=c++20 -g -Wall -Wextra -Werror -pedantic
LDFLAGS = -lpcap

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	chmod +x $@

$(TEST_EXE): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -lpcap -lgtest -lgtest_main -pthread
	chmod +x $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

NixDevShellName:
	@echo c

#test: $(TEST_EXE) $(EXE)
#	@echo "Running tests..."
#	./$(TEST_EXE)

test: tests/test.cpp $(EXE)			
	$(CXX) $(CXXFLAGS) tests/test.cpp $(LDFLAGS) -lgtest -lgtest_main -pthread -o $(TEST_EXE)
	@echo "Running tests..."
	./$(TEST_EXE)

zip:
	zip xczudet00.zip $(SRCS) src/*.hpp tests/*.cpp Makefile README.md LICENSE CHANGELOG.md

clean:
	rm -f $(EXE) $(OBJS) $(TEST_EXE) $(TEST_OBJS) src/*.o tests/*.o *.zip

.PHONY: all clean test NixDevShellName zip
