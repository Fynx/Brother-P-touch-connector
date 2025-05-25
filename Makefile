CXX = g++
CXXFLAGS = -Wall -Wextra -fsanitize=address,undefined -std=c++17

all: wr read_status parse_request

wr: wr.cpp ArgParser.hpp png++/*
	$(CXX) $(CXXFLAGS) `libpng-config --cflags --ldflags` wr.cpp -o wr

read_status: read_status.cpp ArgParser.hpp
	$(CXX) $(CXXFLAGS) read_status.cpp -o read_status

parse_request: parse_request.cpp ArgParser.hpp
	$(CXX) $(CXXFLAGS) parse_request.cpp -o parse_request

.PHONY: clean

clean:
	rm read_status wr parse_request
