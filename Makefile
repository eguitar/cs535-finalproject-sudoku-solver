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

run-brute-4x4:
	mpirun -np 1 ./$(TARGET) test/boards/4x4/easy.txt brute

run-brute-9x9:
	mpirun -np 1 ./$(TARGET) test/boards/9x9/easy.txt brute

run-brute-16x16:
	mpirun -np 1 ./$(TARGET) test/boards/16x16/easy.txt brute

run-par-9x9:
	mpirun -np 4 ./$(TARGET) test/boards/9x9/hard.txt par

run-par-16x16:
	mpirun -np 4 ./$(TARGET) test/boards/16x16/hard.txt par
