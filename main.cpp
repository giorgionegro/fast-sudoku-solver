#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <bit>
#include <chrono>

// Sudoku solver constraints approach

int16_t board[9][9];
int recursiveCount = 0;

void printBoard();

bool solve();

bool
updateConstraints(int16_t row, int16_t col, std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
                  std::vector<int16_t> &boxConstraint);

bool updateAllConstraints(std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
                          std::vector<int16_t> &boxConstraint);

bool recursiveSolver(int count, std::vector<int16_t> emptyCells, std::vector<int16_t> &rowConstraint,
                     std::vector<int16_t> &colConstraint, std::vector<int16_t> &boxConstraint);

static_assert(std::__popcount(0xFULL) == 4, "Bit count assertion failed");

int main(int argc, char *argv[]) {
    std::string filename = "sudoku_board.txt";
    if (argc == 2) {
        filename = argv[1];
        std::cout << "Filename: " << filename << std::endl;
    }

    std::ifstream file(filename);
    if (file.is_open()) {
        for (auto & i : board) {
            for (short & j : i) {
                char buffer;
                file >> buffer;
                j = (buffer == 'x') ? 0 : (buffer - '0');
            }
        }
    } else {
        std::cerr << "Error opening file" << std::endl;
        return 1;  // Exit with error code
    }
    //backup the initial board
    int16_t initialBoard[9][9];
    for (int16_t i = 0; i < 9; i++) {
        for (int16_t j = 0; j < 9; j++) {
            initialBoard[i][j] = board[i][j];
        }
    }


    std::chrono::duration<double> elapsedTotal = std::chrono::duration<double>::zero();
    #define _n_max 100000
    for (int _n = 0; _n < _n_max; ++_n) {
        for (int16_t i = 0; i < 9; i++) {
            for (int16_t j = 0; j < 9; j++) {
                board[i][j] = initialBoard[i][j];
            }
        }
        auto start = std::chrono::high_resolution_clock::now();
        solve();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        elapsedTotal += elapsed;
    }
    std::cout << "Average time taken: " << elapsedTotal.count() / _n_max << "s" << std::endl;


    bool solved = solve();
    if (solved) {
        printBoard();
    } else {
        std::cout << "No solution exists" << std::endl;
    }

    return 0;
}

bool solve() {
    std::vector<int16_t> rowConstraint(9, 0);
    std::vector<int16_t> colConstraint(9, 0);
    std::vector<int16_t> boxConstraint(9, 0);
    updateAllConstraints(rowConstraint, colConstraint, boxConstraint);
    bool modified = true;
    int16_t count ;

    std::vector<bool> emptyCells(81, false);
    while (modified) {
        modified = false;
        int8_t colPossibleCount[9][9] = {0};
        int8_t rowPossibleCount[9][9] = {0};
        int8_t boxPossibleCount[9][9] = {0};
        int8_t rowPossible[9][9] = {0};
        int8_t colPossible[9][9] = {0};
        int8_t boxPossible[9][9][2] = {0};

        count = 0;
        for (int16_t i = 0; i < 9; i++) {
            for (int16_t j = 0; j < 9; j++) {
                if (board[i][j] == 0) {
                    count++;
                    int16_t box = (i / 3) * 3 + j / 3;
                    int16_t constraint = (rowConstraint[i] | colConstraint[j] | boxConstraint[box]);

                    if (std::__popcount(constraint) == 8) {
                        int16_t num = __builtin_ffs(~constraint);
                        board[i][j] = num;
                        modified = true;
                        updateConstraints(i, j, rowConstraint, colConstraint, boxConstraint);
                    } else {
                        for (int16_t k = 0; k < 9; k++) {
                            if (!(constraint & (1 << k))) {
                                rowPossibleCount[i][k]++;
                                colPossibleCount[j][k]++;
                                boxPossibleCount[box][k]++;
                                rowPossible[i][k] = j;
                                colPossible[j][k] = i;
                                boxPossible[box][k][0] = i;
                                boxPossible[box][k][1] = j;
                            }
                        }
                    }
                }
            }
        }

        if (count != 0 && !modified) {
            for (int16_t i = 0; i < 9; i++) {
                for (int16_t j = 0; j < 9; j++) {
                    if (rowPossibleCount[i][j] == 1) {
                        board[i][rowPossible[i][j]] = j + 1;
                        modified = true;
                        count--;
                        updateConstraints(i, rowPossible[i][j], rowConstraint, colConstraint, boxConstraint);
                    }
                    if (colPossibleCount[i][j] == 1) {
                        board[colPossible[i][j]][i] = j + 1;
                        modified = true;
                        count--;
                        updateConstraints(colPossible[i][j], i, rowConstraint, colConstraint, boxConstraint);
                    }
                    if (boxPossibleCount[i][j] == 1) {
                        board[boxPossible[i][j][0]][boxPossible[i][j][1]] = j + 1;
                        modified = true;
                        count--;
                        updateConstraints(boxPossible[i][j][0], boxPossible[i][j][1], rowConstraint, colConstraint,
                                          boxConstraint);
                    }
                }
            }
        }
    }

    if (count != 0) {
        std::vector<int16_t> emptyCells2(count);
        for (int16_t i = 0, j = 0; i < 81; i++) {
            if (board[i / 9][i % 9] == 0) {
                emptyCells2[j] = i;
                j++;
            }
        }
        bool solved = recursiveSolver(0, emptyCells2, rowConstraint, colConstraint, boxConstraint);
        std::cout << "recursive count: " << recursiveCount << std::endl;
        return solved;
    }

    return true;
}

bool recursiveSolver(int count, std::vector<int16_t> emptyCells, std::vector<int16_t> &rowConstraint,
                     std::vector<int16_t> &colConstraint, std::vector<int16_t> &boxConstraint) {
    if (count == emptyCells.size()) {
        return true;
    }
    recursiveCount++;
    int i = emptyCells[count];
    int16_t row = i / 9;
    int16_t col = i % 9;
    int16_t possible = ~(rowConstraint[row] | colConstraint[col] | boxConstraint[(row / 3) * 3 + col / 3]);
    const int16_t full = 0b1111111000000000;
    while (possible != full) {
        int16_t num = __builtin_ffs(possible);
        possible &= ~(1 << (num - 1));
        int16_t oldRow = rowConstraint[row];
        int16_t oldCol = colConstraint[col];
        int16_t oldBox = boxConstraint[(row / 3) * 3 + col / 3];

        rowConstraint[row] |= (1 << (num - 1));
        colConstraint[col] |= (1 << (num - 1));
        boxConstraint[(row / 3) * 3 + col / 3] |= (1 << (num - 1));
        board[row][col] = num;




        if (recursiveSolver(count + 1, emptyCells, rowConstraint, colConstraint, boxConstraint)) {
            return true;
        }


        rowConstraint[row] = oldRow;
        colConstraint[col] = oldCol;
        boxConstraint[(row / 3) * 3 + col / 3] = oldBox;
        board[row][col] = 0;


    }

    return false;
}

bool updateAllConstraints(std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
                          std::vector<int16_t> &boxConstraint) {
    for (int16_t i = 0; i < 9; i++) {
        for (int16_t j = 0; j < 9; j++) {
            if (board[i][j] != 0) {
                rowConstraint[i] |= (1 << (board[i][j] - 1));
                colConstraint[j] |= (1 << (board[i][j] - 1));
                int16_t box = (i / 3) * 3 + j / 3;
                boxConstraint[box] |= (1 << (board[i][j] - 1));
            }
        }
    }
    return true;
}

bool
updateConstraints(int16_t row, int16_t col, std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
                  std::vector<int16_t> &boxConstraint) {
    rowConstraint[row] |= (1 << (board[row][col] - 1));
    colConstraint[col] |= (1 << (board[row][col] - 1));
    int16_t box = (row / 3) * 3 + col / 3;
    boxConstraint[box] |= (1 << (board[row][col] - 1));
    return true;
}

void printBoard() {
    for (auto & i : board) {
        for (int16_t j = 0; j < 9; j++) {
            std::cout << i[j] << " ";
        }
        std::cout << std::endl;
    }
}
