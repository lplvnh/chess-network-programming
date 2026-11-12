CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRC      = $(shell find src -name '*.cpp')
OBJ      = $(SRC:src/%.cpp=build/%.o)
INC      = $(addprefix -I,$(shell find src -type d))

server: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf build server server.log

.PHONY: clean