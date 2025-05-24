CXX = g++
CXXFLAGS = -Wall -Wextra -fsanitize=address,undefined -std=c++17

all: wr read_status

wr: wr.cpp ArgParser.hpp png++/*
	$(CXX) $(CXXFLAGS) `libpng-config --cflags --ldflags` wr.cpp -o wr

read_status: read_status.cpp ArgParser.hpp
	$(CXX) $(CXXFLAGS) read_status.cpp -o read_status

.PHONY: clean

clean:
	rm read_status wr
