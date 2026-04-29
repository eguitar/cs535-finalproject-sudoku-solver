CXX      = mpic++
CXXFLAGS = -std=c++17 -O2 -fopenmp -Wall

SRCS = main.cpp \
       sudoku/sudoku.cpp \
       solver/brute_algo.cpp \
       solver/par_algo.cpp

TARGET = sudoku_solver

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

run-brute:
	mpirun -np 4 ./$(TARGET) test/boards/easy.txt brute

run-par:
	mpirun -np 4 ./$(TARGET) test/boards/easy.txt par

run-hard:
	mpirun -np 4 ./$(TARGET) test/boards/hard.txt par
