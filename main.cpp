#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <bit>
#include <chrono>
#include <immintrin.h>  // For AVX2 intrinsics
#include <bitset>
#include <queue>

// Sudoku solver constraints approach

int16_t board[81]; //TODO: consider transforming this into a local variable

void printBoard();

bool solve();


void updateAllConstraints(int16_t *__restrict__ rowConstraint, int16_t *__restrict__ colConstraint,
                          int16_t *__restrict__ boxConstraint);

bool recursiveSolver(int count, std::priority_queue<std::pair<int, int>> &pq, int16_t *__restrict__ rowConstraint,
                     int16_t *__restrict__ colConstraint, int16_t *__restrict__ boxConstraint);

static_assert(std::__popcount(0xFULL) == 4, "Bit count assertion failed");


void initializePriorityQueue(const std::vector<int16_t> &emptyCells,
                             const int16_t *__restrict__ rowConstraint,
                             const int16_t *__restrict__ colConstraint,
                             const int16_t *__restrict__ boxConstraint);


void loadBoard(const std::string &line) {
    for (int i = 0; i < 81; ++i) {
        board[(i / 9) * 9 + (i % 9)] = 1 << ((line[i] == '.') ? 0 : (line[i] - '0') - 1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) { //TODO: write an argument parser to make it more usable from the command line
        std::cerr << "Usage: ./sudoku_solver <input_file>" << std::endl;
        return 1;
    }

    std::ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    std::string line;
    std::vector<std::string> puzzles;
    std::getline(inputFile, line);
    std::getline(inputFile, line);
    while (std::getline(inputFile, line)) {
        if (line.size() == 81) {
            puzzles.push_back(line);
        }
    }
    inputFile.close();

    std::cout << "Loaded " << puzzles.size() << " puzzles" << std::endl;

    std::chrono::duration<double> totalTime = std::chrono::duration<double>::zero();
    for (const auto &puzzle: puzzles) {
        loadBoard(puzzle);

        auto start = std::chrono::high_resolution_clock::now();
        bool solved = solve();
        auto end = std::chrono::high_resolution_clock::now();

        totalTime += (end - start);
        if (solved) {//TODO: save the solutions into a file/files
//            printBoard();
//           std::cout << std::endl;
        } else {
            std::cout << "No solution exists for the puzzle" << std::endl;
        }
    }
    auto average = totalTime.count() / puzzles.size();
    std::cout << "puzzle per second: " << 1 / average << std::endl;

    return 0;
}


std::priority_queue<std::pair<int, int>> cellQueue;



int recursiveCount = 0;

bool recursiveSolver(int count, std::priority_queue<std::pair<int, int>> &pq, int16_t *__restrict__ rowConstraint,
                     int16_t *__restrict__ colConstraint, int16_t *__restrict__ boxConstraint) {
    if (pq.empty()) {
        return true;  // All cells filled
    }
    recursiveCount++;

    auto bestpair = pq.top();
    pq.pop();
    auto idx = bestpair.second;
    int16_t bestRow = idx / 9;
    int16_t bestCol = idx % 9;
    int16_t bestBox = (bestRow / 3) * 3 + bestCol / 3;
    int16_t bestPossible = ~(rowConstraint[bestRow] | colConstraint[bestCol] | boxConstraint[bestBox]) & 0x1FF;


    while (bestPossible != 0) {
        int16_t num = __builtin_ffs(bestPossible);
        bestPossible &= ~(1 << (num - 1));

        // Save old constraints
        int16_t oldRow = rowConstraint[bestRow];
        int16_t oldCol = colConstraint[bestCol];
        int16_t oldBox = boxConstraint[bestBox];

        // Update constraints and board
        rowConstraint[bestRow] |= (1 << (num - 1));
        colConstraint[bestCol] |= (1 << (num - 1));
        boxConstraint[bestBox] |= (1 << (num - 1));
        board[bestRow * 9 + bestCol] = (1 << (num - 1));

        if (recursiveSolver(count + 1, pq, rowConstraint, colConstraint, boxConstraint)) {
            return true;
        }

        // Backtrack: Restore constraints and board
        rowConstraint[bestRow] = oldRow;
        colConstraint[bestCol] = oldCol;
        boxConstraint[bestBox] = oldBox;
        board[bestRow * 9 + bestCol] = 0;
    }

    return false;
}

bool solve() {
    std::vector<int16_t> rowConstraint_vec(9, 0);
    std::vector<int16_t> colConstraint_vec(9, 0);
    std::vector<int16_t> boxConstraint_vec(9, 0);
    int16_t *__restrict__ rowConstraint = rowConstraint_vec.data();
    int16_t *__restrict__ colConstraint = colConstraint_vec.data();
    int16_t *__restrict__ boxConstraint = boxConstraint_vec.data();
    updateAllConstraints(rowConstraint, colConstraint, boxConstraint);

    bool modified;
    int16_t zeroCount;
    zeroCount = 0;
    for (int16_t i = 0; i < 9; i++) {
        for (int16_t j = 0; j < 9; j++) {
            if (board[i * 9 + j] == 0) {
                zeroCount++; //first round so count the number of zeros
                int16_t box = (i / 3) * 3 + j / 3;

                int16_t constraint = (rowConstraint[i] | colConstraint[j] |
                                      boxConstraint[box]); //combine all relevant constraints to get individual cell constraint

                int16_t possible = ~constraint;//possible is the negation of constraints
                int16_t num = __builtin_ffs(
                        possible); //find the first set bit in possible, i.e. get the first possible value from the right of possible

                int16_t value = 1 << (num - 1); // value are saved as a set bit in the nth position

                int16_t mask = -(std::__popcount(constraint) ==
                                 8); //zeroCount numbers of constraints, if 8 only one possible value. We use mask to do all this without Ifs
                board[i * 9 + j] |= value & mask; //so set it to the board
                // and update constraints
                rowConstraint[i] |= value & mask;
                colConstraint[j] |= value & mask;
                boxConstraint[box] |= value & mask;
                //subtruct one if value was assigned
                zeroCount += mask;
                //and set modified to True
                modified |= mask;

            }
        }
    }
    do {
        modified = false;

        for (int16_t i = 0; i < 9; i++) {
            for (int16_t j = 0; j < 9; j++) {
                if (board[i * 9 + j] == 0) {
                    int16_t box = (i / 3) * 3 + j / 3;

                    int16_t constraint = (rowConstraint[i] | colConstraint[j] |
                                          boxConstraint[box]); //combine all relevant constraints to get individual cell constraint

                    int16_t possible = ~constraint;//possible is the negation of constraints
                    int16_t num = __builtin_ffs(
                            possible); //find the first set bit in possible, i.e. get the first possible value from the right of possible

                    int16_t value = 1 << (num - 1); // value are saved as a set bit in the nth position

                    int16_t mask = -(std::__popcount(constraint) ==
                                     8); //zeroCount numbers of constraints, if 8 only one possible value. We use mask to do all this without Ifs
                    board[i * 9 + j] |= value & mask; //so set it to the board
                    // and update constraints
                    rowConstraint[i] |= value & mask;
                    colConstraint[j] |= value & mask;
                    boxConstraint[box] |= value & mask;
                    //subtruct one if value was assigned
                    zeroCount += mask;
                    //and set modified to True
                    modified |= mask;
                    }
            }
        }


        /*  // Hidden Singles really slow down the solver
          if (zeroCount !=0 &&! modified && false)
          for (int16_t num = 1; num <= 9; num++) {
              // Check rows
              for (int16_t i = 0; i < 9; i++) {
                  int16_t possibleCell = -1;
                  for (int16_t j = 0; j < 9; j++) {
                      if (board[i*9+j] == 0 && !(rowConstraint[i] & (1 << (num - 1))) && !(colConstraint[j] & (1 << (num - 1))) && !(boxConstraint[(i / 3) * 3 + (j / 3)] & (1 << (num - 1)))) {
                          if (possibleCell == -1) {
                              possibleCell = j; // Found a candidate cell
                          } else {
                              possibleCell = -1; // More than one candidate, break
                              break;
                          }
                      }
                  }
                  if (possibleCell != -1) {

                          board[i*9+possibleCell] = num; // Fill the hidden single
                          modified = true;
                          zeroCount--;
                          updateConstraints(i, possibleCell, rowConstraint, colConstraint, boxConstraint);

                  }
              }

              // Check columns
              for (int16_t j = 0; j < 9; j++) {
                  int16_t possibleCell = -1;
                  for (int16_t i = 0; i < 9; i++) {
                      if (board[i*9+j] == 0 && !(rowConstraint[i] & (1 << (num - 1))) && !(colConstraint[j] & (1 << (num - 1))) && !(boxConstraint[(i / 3) * 3 + (j / 3)] & (1 << (num - 1)))) {
                          if (possibleCell == -1) {
                              possibleCell = i;
                          } else {
                              possibleCell = -1;
                              break;
                          }
                      }
                  }
                  if (possibleCell != -1) {

                          board[possibleCell*9+j] = num; // Fill the hidden single
                          modified = true;
                          zeroCount--;
                          updateConstraints(possibleCell, j, rowConstraint, colConstraint, boxConstraint);

                  }
              }


          }
  */

    } while (modified);

    if (zeroCount != 0) {
        std::vector<int16_t> emptyCells2(zeroCount);
        for (int16_t i = 0, j = 0; i < 81; i++) {
            if (board[i] == 0) {
                emptyCells2[j++] = i;
            }
        }
        initializePriorityQueue(emptyCells2, rowConstraint, colConstraint, boxConstraint);
        bool solved = recursiveSolver(0, cellQueue, rowConstraint, colConstraint, boxConstraint);
        return solved;
    }

    return true;
}


void updateAllConstraints(int16_t *__restrict__ rowConstraint, int16_t *__restrict__ colConstraint,
                          int16_t *__restrict__ boxConstraint) {
    for (int16_t i = 0; i < 9; i++) {
        for (int16_t j = 0; j < 9; j++) {
            int16_t value = board[i * 9 + j];
            rowConstraint[i] |= value;
            colConstraint[j] |= value;
                int16_t box = (i / 3) * 3 + j / 3;
            boxConstraint[box] |= value;

        }
    }
}



void printBoard() {
    for (int16_t j = 0; j < 81; j++) {
        //go back from 1 << (num -1 )
        int value = __builtin_ffs(board[j]);
        std::cout << value << " ";
        if ((j + 1) % 9 == 0)
            std::cout << std::endl;
    }

}

void initializePriorityQueue(const std::vector<int16_t> &emptyCells, const int16_t *rowConstraint,
                             const int16_t *colConstraint, const int16_t *boxConstraint) {
    cellQueue = std::priority_queue<std::pair<int, int>>();

    for (int idx: emptyCells) {
        int row = idx / 9;
        int col = idx % 9;
        int box = (row / 3) * 3 + col / 3;
        int options = std::__popcount(~(rowConstraint[row] | colConstraint[col] | boxConstraint[box]) & 0x1FF);
        cellQueue.emplace(-options, idx); //we use - to ensure that the smaller is at the top
    }
}

