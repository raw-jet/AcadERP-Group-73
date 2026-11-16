CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -Werror -O2
CPPFLAGS ?= -Isrc

SOURCES := src/grp_73_main.cpp src/grp_73_CommandParser.cpp src/grp_73_Model.cpp src/grp_73_Registrar.cpp
HEADERS := src/grp_73_CommandParser.h src/grp_73_ErpException.h src/grp_73_Model.h src/grp_73_Registrar.h

.PHONY: all clean test

all: acaderp

acaderp: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $@

test: acaderp
	./run_tests.sh

clean:
	rm -f acaderp
