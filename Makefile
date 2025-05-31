CXX = g++
BUILD_TYPE = release
CXXFLAGS = -Wall -std=c++20

ifeq ($(BUILD_TYPE),debug)
	CXXFLAGS = -Wall -Wextra -fsanitize=address,undefined -ggdb -std=c++20
endif

all: make_request read_status parse_request

make_request: make_request.cpp ArgParser.hpp constants.hpp png++/*
	$(CXX) $(CXXFLAGS) `libpng-config --cflags --ldflags` make_request.cpp -o make_request

read_status: read_status.cpp ArgParser.hpp
	$(CXX) $(CXXFLAGS) read_status.cpp -o read_status

parse_request: parse_request.cpp ArgParser.hpp
	$(CXX) $(CXXFLAGS) parse_request.cpp -o parse_request

.PHONY: clean

clean:
	rm -f read_status make_request parse_request
