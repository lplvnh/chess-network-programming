CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
DEPFLAGS = -MMD -MP   # sinh file .d: sửa file .h thì các .cpp dùng nó được biên dịch lại
SRC      = $(shell find src -name '*.cpp')
OBJ      = $(SRC:src/%.cpp=build/%.o)
DEP      = $(OBJ:.o=.d)
INC      = $(addprefix -I,$(shell find src -type d))
LDLIBS   = -lcrypt 

server: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INC) -c $< -o $@

-include $(DEP)

clean:
	rm -rf build server server.log chess_test

test: tests/chess_test.cpp src/game/chess.cpp
	$(CXX) -std=c++17 -Wall -Wextra -O2 -Isrc/game -o chess_test $^
	./chess_test

.PHONY: clean