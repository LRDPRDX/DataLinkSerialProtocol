INC_DIR = ../../protocol
CXX_FLAGS = -std=c++11
CXX = g++
SRC = $(wildcard *.cpp)
BIN = $(SRC:.cpp=)

test : $(BIN)
	./$<

$(BIN) : $(SRC)
	$(CXX) $(CXX_FLAGS) -I $(INC_DIR) $< -o $@ 

.PHONY : clean
clean :
	rm $(BIN)
