CXX ?= c++
PYTHON ?= python3
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Werror -Wpedantic -O2 -pthread -Iinclude
HEADERS = $(wildcard include/*.hpp include/*.h)
all: build/offshore
build:
	mkdir -p build
build/offshore: src/main.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) src/main.cpp -o $@
build/test: tests/test.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) tests/test.cpp -o $@
test: build/test build/offshore
	./build/test
	$(PYTHON) tests/cli_test.py
clean:
	rm -rf build
.PHONY: all test clean

