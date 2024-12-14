#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <bit>
#include <chrono>
#include <immintrin.h>  // For AVX2 intrinsics
#include <bitset>

// Sudoku solver constraints approach

int16_t board[9][9];

void printBoard();

bool solve();

void updateConstraints(int16_t row, int16_t col, std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
                       std::vector<int16_t> &boxConstraint);

void updateAllConstraints(std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
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

    // Backup the initial board
    int16_t initialBoard[9][9];
    for (int16_t i = 0; i < 9; i++) {
        std::copy(std::begin(board[i]), std::end(board[i]), std::begin(initialBoard[i]));
    }

    std::chrono::duration<double> elapsedTotal = std::chrono::duration<double>::zero();
    constexpr int _n_max = 100000;
    for (int _n = 0; _n < _n_max; ++_n) {
        for (int16_t i = 0; i < 9; i++) {
            std::copy(std::begin(initialBoard[i]), std::end(initialBoard[i]), std::begin(board[i]));
        }
        auto start = std::chrono::high_resolution_clock::now();
        solve();
        auto end = std::chrono::high_resolution_clock::now();
        elapsedTotal += (end - start);
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
int recursiveCount =0;
bool recursiveSolver(int count, std::vector<int16_t> emptyCells, std::vector<int16_t> &rowConstraint,
                     std::vector<int16_t> &colConstraint, std::vector<int16_t> &boxConstraint) {
    if (count == emptyCells.size()) {
        return true;
    }
    recursiveCount++;

    int minOptions = 10;
    int bestCell = -1;
    int16_t bestRow, bestCol, bestPossible = 0;

    for (int idx = count; idx < emptyCells.size(); ++idx) {
        int i = emptyCells[idx];
        int16_t row = i / 9;
        int16_t col = i % 9;
        int16_t possible = ~(rowConstraint[row] | colConstraint[col] | boxConstraint[(row / 3) * 3 + col / 3]) & 0x1FF;

        int numOptions = std::__popcount(possible);
        if (numOptions < minOptions) {
            minOptions = numOptions;
            bestCell = idx;
            bestRow = row;
            bestCol = col;
            bestPossible = possible;

            if (minOptions == 1) {
                break;
            }
        }
    }

    if (bestCell == -1) {
        return false;
    }

    std::swap(emptyCells[count], emptyCells[bestCell]);

    while (bestPossible != 0) {
        int16_t num = __builtin_ffs(bestPossible);
        bestPossible &= ~(1 << (num - 1));

        // Save old constraints
        int16_t oldRow = rowConstraint[bestRow];
        int16_t oldCol = colConstraint[bestCol];
        int16_t oldBox = boxConstraint[(bestRow / 3) * 3 + bestCol / 3];

        // Update constraints and board
        rowConstraint[bestRow] |= (1 << (num - 1));
        colConstraint[bestCol] |= (1 << (num - 1));
        boxConstraint[(bestRow / 3) * 3 + bestCol / 3] |= (1 << (num - 1));
        board[bestRow][bestCol] = num;

        if (recursiveSolver(count + 1, emptyCells, rowConstraint, colConstraint, boxConstraint)) {
            return true;
        }

        // Backtrack: Restore constraints and board
        rowConstraint[bestRow] = oldRow;
        colConstraint[bestCol] = oldCol;
        boxConstraint[(bestRow / 3) * 3 + bestCol / 3] = oldBox;
        board[bestRow][bestCol] = 0;
    }

    return false;
}

bool solve() {
    std::vector<int16_t> rowConstraint(9, 0);
    std::vector<int16_t> colConstraint(9, 0);
    std::vector<int16_t> boxConstraint(9, 0);
    updateAllConstraints(rowConstraint, colConstraint, boxConstraint);

    bool modified;
    int16_t count;
    bool firstRun = true;
    do {
        modified = false;


        if(firstRun) count = 0;
        for (int16_t i = 0; i < 9; i++) {
            for (int16_t j = 0; j < 9; j++) {
                if (board[i][j] == 0) {
                    if(firstRun) count++;
                    int16_t box = (i / 3) * 3 + j / 3;

                    int16_t constraint =   (rowConstraint[i] | colConstraint[j] | boxConstraint[box]);
                    if (std::bitset<9>(constraint).count() == 8) {
                        int16_t num = __builtin_ffs(~constraint);
                        board[i][j] = num;
                        modified = true;
                        updateConstraints(i, j, rowConstraint, colConstraint, boxConstraint);
                        count--;
                    }

                }
            }
        }


        // Hidden Singles really slow down the solver
        if (count !=0 &&! modified && false)
        for (int16_t num = 1; num <= 9; num++) {
            // Check rows
            for (int16_t i = 0; i < 9; i++) {
                int16_t possibleCell = -1;
                for (int16_t j = 0; j < 9; j++) {
                    if (board[i][j] == 0 && !(rowConstraint[i] & (1 << (num - 1))) && !(colConstraint[j] & (1 << (num - 1))) && !(boxConstraint[(i / 3) * 3 + (j / 3)] & (1 << (num - 1)))) {
                        if (possibleCell == -1) {
                            possibleCell = j; // Found a candidate cell
                        } else {
                            possibleCell = -1; // More than one candidate, break
                            break;
                        }
                    }
                }
                if (possibleCell != -1) {

                        board[i][possibleCell] = num; // Fill the hidden single
                        modified = true;
                        count--;
                        updateConstraints(i, possibleCell, rowConstraint, colConstraint, boxConstraint);

                }
            }

            // Check columns
            for (int16_t j = 0; j < 9; j++) {
                int16_t possibleCell = -1;
                for (int16_t i = 0; i < 9; i++) {
                    if (board[i][j] == 0 && !(rowConstraint[i] & (1 << (num - 1))) && !(colConstraint[j] & (1 << (num - 1))) && !(boxConstraint[(i / 3) * 3 + (j / 3)] & (1 << (num - 1)))) {
                        if (possibleCell == -1) {
                            possibleCell = i;
                        } else {
                            possibleCell = -1;
                            break;
                        }
                    }
                }
                if (possibleCell != -1) {

                        board[possibleCell][j] = num; // Fill the hidden single
                        modified = true;
                        count--;
                        updateConstraints(possibleCell, j, rowConstraint, colConstraint, boxConstraint);

                }
            }


        }


    } while (modified);

    if (count != 0) {
        std::vector<int16_t> emptyCells2(count);
        for (int16_t i = 0, j = 0; i < 81; i++) {
            if (board[i / 9][i % 9] == 0) {
                emptyCells2[j++] = i;
            }
        }
        bool solved = recursiveSolver(0, emptyCells2, rowConstraint, colConstraint, boxConstraint);
        return solved;
    }

    return true;
}

/*
bool recursiveSolver(int count, std::vector<int16_t> emptyCells, std::vector<int16_t> &rowConstraint,
                     std::vector<int16_t> &colConstraint, std::vector<int16_t> &boxConstraint) {
    if (count == emptyCells.size()) {
        return true;
    }
    int i = emptyCells[count];
    int16_t row = i / 9;
    int16_t col = i % 9;
    int16_t possible = ~(rowConstraint[row] | colConstraint[col] | boxConstraint[(row / 3) * 3 + col / 3]);
    const int16_t full = 0b1111111000000000;

    while (possible != full) {
        int16_t num = __builtin_ffs(possible);
        //auto num_bit = std::bitset<9>(possible);
        //int16_t num = num_bit._Find_first();

        possible &= ~(1 << (num - 1));
        int16_t oldRow = rowConstraint[row];
        int16_t oldCol = colConstraint[col];
        int16_t oldBox = boxConstraint[(row / 3) * 3 + col / 3];

        // Update constraints and board
        rowConstraint[row] |= (1 << (num - 1));
        colConstraint[col] |= (1 << (num - 1));
        boxConstraint[(row / 3) * 3 + col / 3] |= (1 << (num - 1));
        board[row][col] = num;

        if (recursiveSolver(count + 1, emptyCells, rowConstraint, colConstraint, boxConstraint)) {
            return true;
        }

        // Revert constraints and board
        rowConstraint[row] = oldRow;
        colConstraint[col] = oldCol;
        boxConstraint[(row / 3) * 3 + col / 3] = oldBox;
        board[row][col] = 0;
    }

    return false;
}
*/

void updateAllConstraints(std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
                          std::vector<int16_t> &boxConstraint) {
    for (int16_t i = 0; i < 9; i++) {
        for (int16_t j = 0; j < 9; j++) {
            if (board[i][j] != 0) {
                int16_t value = board[i][j] - 1;
                rowConstraint[i] |= (1 << value);
                colConstraint[j] |= (1 << value);
                int16_t box = (i / 3) * 3 + j / 3;
                boxConstraint[box] |= (1 << value);
            }
        }
    }
}

void updateConstraints(int16_t row, int16_t col, std::vector<int16_t> &rowConstraint, std::vector<int16_t> &colConstraint,
                       std::vector<int16_t> &boxConstraint) {
    int16_t value = board[row][col] - 1;
    rowConstraint[row] |= (1 << value);
    colConstraint[col] |= (1 << value);
    int16_t box = (row / 3) * 3 + col / 3;
    boxConstraint[box] |= (1 << value);
}

void printBoard() {
    for (auto & i : board) {
        for (int16_t j = 0; j < 9; j++) {
            std::cout << i[j] << " ";
        }
        std::cout << std::endl;
    }
}
