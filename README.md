# Sudoku Solver

A fast and efficient Sudoku solver for 9x9 matrices, optimized for speed and accuracy. In testing, it demonstrates performance comparable to tdoku on Kaggle's 1 million Sudoku dataset, achieving ~1.8 million solutions per second compared to tdoku's ~1.3 million, ~3e5 on hard puzzle compared to tdoku's ~2e4 but slower on unsolvable puzzle 1.6 million on puzzles8_gen_puzzles compared to tdoku's 5 million



## Roadmap
### TODOs
1. **Argument Parser:**
   - Add command-line options to switch between batch prediction (from a file) and single prediction modes.
2. **Dancing Links Method:**
   - Test and integrate the Dancing Links algorithm for solving.
3. **File Output:**
   - Implement functionality to save the solved puzzles to an output file.
4. **Manual Vectorization to harvest more preformance**
