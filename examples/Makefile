INC_DIR = ../src
CXX_FLAGS = -std=c++17
CXX = g++
SRC = $(wildcard *.cpp)
BIN = $(SRC:.cpp=)

run : $(BIN)
	./$<

$(BIN) : $(SRC)
	$(CXX) $(CXX_FLAGS) -I $(INC_DIR) $< -o $@ 

.PHONY : clean
clean :
	rm $(BIN)
