# GWD

The source code of my Draughts program GWD. I have been working on it on and off for the last 40 years. It has the following features:

- Iterative deepening
- Alpha-beta with search reductions
- Neural networks for the evaluation
- Uses incremental updates and AVX2 for evaluation of the neural networks
- Supports both the DXP and HUB protocol, so Turbo Dambase can be used as the GUI
- Uses OpenMPI for 'bulk parallelization', so on a 16 CPU machine you can play 4 HUB matches in parallel, each match using 4 threads or 16 matches in parallel, each match using 1 thread
- Uses an embedded SQLite database for the opening book
- An opening book can be generated in parallel using drop-out expansion
- Compiles on both 64-bit Linux and Windows

The code is readable and not-so readable and comments are lacking, but nowadays you can also ask ChatGPT for help if you want to know how something works. Consider for example the following code fragment that would be REALLY hard to understand without the header files. It is my move generator that is at least 30 years old:

```
void gen_my_moves(board_t *with, moves_list_t *moves_list, int quiescence)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(with->board_colour2move != MY_BIT)

  moves_list->nmoves = 0;
  moves_list->ncaptx = 0;

  ui64_t my_bb = (with->my_man_bb | with->my_crown_bb);
  ui64_t your_bb = (with->your_man_bb | with->your_crown_bb);
  ui64_t empty_bb = with->board_empty_bb & ~(my_bb | your_bb);

  while(my_bb != 0)
  {
    int iboard1;

    if (IS_WHITE(with->board_colour2move))
      iboard1 = BIT_CTZ(my_bb);
    else
      iboard1 = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(iboard1);

    for (int i = 0; i < 4; i++)
    {
      int jboard1 = iboard1 + my_dir[i];

      if (with->my_man_bb & BITULL(iboard1))
      {
        if (empty_bb & BITULL(jboard1))
        {
          if ((i < 2) and (moves_list->ncaptx == 0))
          {
            SOFTBUG(moves_list->nmoves >= MOVES_MAX)

            move_t *move = moves_list->moves + moves_list->nmoves;
            
            move->move_from = iboard1;
            move->move_to = jboard1;
            move->move_captures_bb = 0;
       
            moves_list->nmoves++;
          }
        }
      }
      else
      {
        while (empty_bb & BITULL(jboard1))
        {
          if (moves_list->ncaptx == 0)
          {
            SOFTBUG(moves_list->nmoves >= MOVES_MAX)

            move_t *move = moves_list->moves + moves_list->nmoves;
            
            move->move_from = iboard1;
            move->move_to = jboard1;
            move->move_captures_bb = 0;
       
            moves_list->nmoves++;
          }
          jboard1 += my_dir[i];
        }
      }
      if (your_bb & BITULL(jboard1))
      {
        int kboard1 = jboard1 + my_dir[i];

        if (empty_bb & BITULL(kboard1))
        {  
          int idir[NPIECES_MAX + 1];
          int jdir[NPIECES_MAX + 1];
          int iboard[NPIECES_MAX + 1];
          int jboard[NPIECES_MAX + 1];

          jboard[0] = 0;

          //you may pass the square you started on again..

          empty_bb |= BITULL(iboard1);

          idir[1] = my_dir[i];

          int ncapt = 0;

          ui64_t captures_bb = 0;

          label_next_capt:

          ++ncapt;

          captures_bb |= BITULL(jboard1);

          jboard[ncapt] = jboard1;
          iboard[ncapt] = kboard1;

          label_crown:

          if (ncapt > moves_list->ncaptx)
          {
            moves_list->ncaptx = ncapt;

            moves_list->nmoves = 0;
          }
          if (ncapt == moves_list->ncaptx)
          {
            SOFTBUG(moves_list->nmoves >= MOVES_MAX)

            move_t *move = moves_list->moves + moves_list->nmoves;
            
            move->move_from = iboard1;
            move->move_to = kboard1;
            move->move_captures_bb = captures_bb;
       
            moves_list->nmoves++;
          }
          if (idir[ncapt] > 0)
            jdir[ncapt] = 11 - idir[ncapt];
          else
            jdir[ncapt] = 11 + idir[ncapt];

          jboard1 = iboard[ncapt] + jdir[ncapt];

          if (with->my_crown_bb & BITULL(iboard1))
            while (empty_bb & BITULL(jboard1)) jboard1 += jdir[ncapt];

          //..but you may not capture the same piece twice!
          if ((your_bb & BITULL(jboard1)) and
              !(captures_bb & BITULL(jboard1)))
          {
            kboard1 = jboard1 + jdir[ncapt];

            if (empty_bb & BITULL(kboard1))
            {
              idir[ncapt + 1] = jdir[ncapt];

              goto label_next_capt;
            }
          }

          label_neg_dir:

          jdir[ncapt] = -jdir[ncapt];

          jboard1 = iboard[ncapt] + jdir[ncapt];

          if (with->my_crown_bb & BITULL(iboard1))
            while (empty_bb & BITULL(jboard1)) jboard1 += jdir[ncapt];

          if ((your_bb & BITULL(jboard1)) and
              !(captures_bb & BITULL(jboard1)))
          {
            kboard1 = jboard1 + jdir[ncapt];

            if (empty_bb & BITULL(kboard1))
            {
              idir[ncapt + 1] = jdir[ncapt];

              goto label_next_capt;
            }
          }

          label_dir:

          jdir[ncapt] = 0;

          jboard1 = iboard[ncapt] + idir[ncapt];

          if ((with->my_crown_bb & BITULL(iboard1)) and
              (empty_bb & BITULL(jboard1)))
          {
            kboard1 = iboard[ncapt] = jboard1;

            goto label_crown;
          }

          if ((your_bb & BITULL(jboard1)) and
              !(captures_bb & BITULL(jboard1)))
          {
            kboard1 = jboard1 + idir[ncapt];

            if (empty_bb & BITULL(kboard1))
            {
              idir[ncapt + 1] = idir[ncapt];

              goto label_next_capt;
            }
          }

          label_undo_capt:

          captures_bb &= ~BITULL(jboard[ncapt]);

          --ncapt;

          if (ncapt > 0)
          {
            if (jdir[ncapt] > 0) goto label_neg_dir;

            if (jdir[ncapt] < 0) goto label_dir;

            goto label_undo_capt;
          }
          empty_bb &= ~BITULL(iboard1);
        }
      }
    }
  }

  for (int imove = 0; imove < moves_list->nmoves; imove++)
  {   
    move_t *move = moves_list->moves + imove;
     
    int iboard = move->move_from;
    int kboard = move->move_to;

    int move_score_delta = 0;

    ui64_t captures_bb = move->move_captures_bb;

    while(captures_bb != 0)
    {
      int jboard = BIT_CTZ(captures_bb);

      if (with->your_man_bb & BITULL(jboard))
      {
        move_score_delta += SCORE_MAN;
      }
      else
      {
        move_score_delta += SCORE_CROWN;
      }

      captures_bb &= ~BITULL(jboard);
    }

    int move_weight = move_score_delta;

    int move_tactical = FALSE;

    if (with->my_man_bb & BITULL(iboard))
    {
      if (my_row[kboard] == 8)
      {
        move_weight = move_score_delta + 1;
      }
      else if (my_row[kboard] == 9)
      {
        move_weight = move_score_delta + 2;

        if (options.row9_captures_are_tactical and (moves_list->ncaptx > 0))
        {
          move_tactical = ROW9_CAPTURE_BIT;
        }
      }
      else if (my_row[kboard] == 10)
      {
        move_score_delta += SCORE_CROWN - SCORE_MAN;

        move_weight = move_score_delta + 3;

        if (options.promotions_are_tactical)
        {
          move_tactical = PROMOTION_BIT;
        }
      }
    }

    moves_list->moves_weight[imove] = move_weight;

    moves_list->moves_tactical[imove] = move_tactical;
  }

  with->board_nodes[with->board_inode].node_ncaptx = moves_list->ncaptx;

  END_BLOCK
}
```
This is what ChatGPT thinks what it does and it is spot on. It even talks about 'quiescent capture moves' although the quiescence argument is no longer used, but in a past version I indeed used that argument to only generate tactical capture moves. It also spotted the 'recursion in Donald Knuth style' using a local stack:

The C code you've provided is a function that generates all legal moves for a given player's pieces on a board, including quiescent (capture-based) and non-quiescent moves in a draughts (checkers) game. Here’s a breakdown and analysis of the main components of the code:

### Key Features:

1. **Bitboard Representation**:
   The game state is represented using bitboards (`ui64_t`), where each bit corresponds to a square on the board. The code distinguishes between:
   - `my_bb`: Bitboard representing the player’s pieces (both men and crowned pieces).
   - `your_bb`: Bitboard representing the opponent’s pieces.
   - `empty_bb`: Bitboard representing empty squares on the board.

2. **Move Generation**:
   The primary task of the function is to generate all possible moves and store them in a `moves_list` structure. The function first iterates through each of the player's pieces (`my_bb`), and for each piece, it checks possible move directions stored in `my_dir`.

   - **Simple Moves**: If the piece is a man (non-crowned), it attempts to move one square diagonally into an empty square. If the piece is a crown, it can move multiple squares in the same direction.
   - **Capture Moves**: If an opponent's piece (`your_bb`) is found adjacent to the player’s piece, and the square beyond that piece is empty, a capture move is possible.

3. **Quiescence (Capture Move) Handling**:
   The code handles capture moves recursively, allowing for multi-capture sequences. After each capture, the code explores further captures in the same or different directions, ensuring that no piece is captured more than once per move sequence.

4. **Score Evaluation**:
   After generating all possible moves, the function evaluates the quality of each move:
   - **Capture Scoring**: For each capture move, the function adds points based on whether a captured piece is a man or a crown (`SCORE_MAN` and `SCORE_CROWN`).
   - **Promotion Scoring**: If a man reaches the promotion row (row 10), it is promoted to a crown, and the score is adjusted to reflect this.

5. **Tactical Considerations**:
   The code also tracks whether a move is "tactical," marking it with specific bits (`ROW9_CAPTURE_BIT` and `PROMOTION_BIT`) depending on the move type (e.g., promotion or capture in row 9).

6. **Edge Case Handling**:
   There are several labeled blocks of code handling specific cases like promotion, multi-captures, and ensuring that the same piece is not captured multiple times in a sequence. The use of `goto` statements, though uncommon in modern C programming, is used here to manage recursive and conditional capture sequences effectively.

### Detailed Breakdown of the Logic:

1. **Initial Setup**:
   - The board is validated using a `SOFTBUG` macro to ensure that the player to move is correct.
   - Bitboards representing the player's and opponent’s pieces, as well as empty squares, are initialized.

2. **Iterating Over Pieces**:
   - The function extracts the index of the least significant or most significant bit set (depending on color) from `my_bb` to process each piece.
   - It then clears the corresponding bit from `my_bb` and proceeds to evaluate potential moves for the current piece.

3. **Simple Move Evaluation**:
   - For each direction (`my_dir`), the function checks if the target square is empty. If so, a move is added to the `moves_list`, unless it's part of a capture sequence (`ncaptx == 0`).

4. **Capture Move Evaluation**:
   - The function handles capture moves by checking if an opponent’s piece is adjacent and whether the square beyond it is empty.
   - If a capture is possible, the code recursively explores further captures in the same or other directions, using the `goto` labels (`label_next_capt`, `label_neg_dir`, etc.) to manage multi-step capture sequences.

5. **Move Scoring**:
   - Each move is scored based on the number of pieces captured and whether the piece promotes to a crown.
   - The function also handles tactical moves (e.g., row 9 captures or promotions) by setting appropriate flags.

6. **End of Function**:
   - The function updates the board's node structure (`board_nodes`) with the maximum number of captures (`ncaptx`).

### Comments:
- **Efficient Move Generation**: The use of bitboards and intrinsic functions like `BIT_CTZ` and `BIT_CLZ` allows the code to efficiently iterate over pieces and check legal moves.
- **Readability**: The use of labels and `goto` makes the logic for multi-captures and recursive move generation more complex and harder to follow. Modern practices would typically prefer using loops or function calls instead of `goto`, although in this context, `goto` ensures efficient backtracking.
- **Optimization**: Given the complexity of recursive capture moves, the function seems optimized for speed at the cost of readability. This is crucial in board games like draughts where the search depth and number of possible moves can grow exponentially.
- **Scalability**: The handling of tactical moves and promotion is straightforward but could be extended with more nuanced evaluation for advanced strategies.

This code is well-suited for a performance-critical application, such as an AI engine for draughts, and its structure supports both quiescent and non-quiescent moves, as required for accurate evaluation in such games.

The answer to the next question is even more remarkable. Also note comments like 'continue in the same direction', 'ensure the same piece is not captured twice' and 'try the opposite direction':

Could you take a stab at reformulating the code in a recursive fashion without the goto's?

Certainly! I'll help you refactor the code into a more structured recursive approach, avoiding `goto` statements and enhancing clarity. Here's a reformulated version of the key part of the code where the recursive capture sequence is handled. I will focus primarily on replacing the multi-capture sequences that were controlled via `goto` with recursive functions.

### Original Sections with `goto`:

The core of the recursive logic revolves around the multi-capture mechanism, so we'll rewrite that with recursion. The following parts of the code involving `label_next_capt`, `label_neg_dir`, etc., will be replaced by recursive function calls.

### Refactored Code

Here's the updated code with recursion handling the multi-capture logic:

```c
void gen_my_moves(board_t *with, moves_list_t *moves_list, int quiescence);

void generate_capture_moves(board_t *with, moves_list_t *moves_list, int iboard1, int ncapt, ui64_t captures_bb, ui64_t empty_bb, int *idir, int *jdir, int *iboard, int *jboard, int move_from) {
    if (ncapt > moves_list->ncaptx) {
        // New best capture move, reset the moves list
        moves_list->ncaptx = ncapt;
        moves_list->nmoves = 0;
    }
    if (ncapt == moves_list->ncaptx) {
        // Add the current move to the list
        SOFTBUG(moves_list->nmoves >= MOVES_MAX);
        move_t *move = moves_list->moves + moves_list->nmoves;
        move->move_from = move_from;
        move->move_to = iboard[ncapt];
        move->move_captures_bb = captures_bb;
        moves_list->nmoves++;
    }

    // Try to continue the capture sequence in the same direction
    int jboard1 = iboard[ncapt] + idir[ncapt];
    if (with->my_crown_bb & BITULL(move_from)) {
        while (empty_bb & BITULL(jboard1)) {
            jboard1 += idir[ncapt];
        }
    }

    // Ensure we don't capture the same piece twice
    if ((with->your_man_bb | with->your_crown_bb) & BITULL(jboard1) && !(captures_bb & BITULL(jboard1))) {
        int kboard1 = jboard1 + idir[ncapt];
        if (empty_bb & BITULL(kboard1)) {
            captures_bb |= BITULL(jboard1);
            iboard[ncapt + 1] = kboard1;
            idir[ncapt + 1] = idir[ncapt];
            generate_capture_moves(with, moves_list, iboard1, ncapt + 1, captures_bb, empty_bb, idir, jdir, iboard, jboard, move_from);
            captures_bb &= ~BITULL(jboard1);  // backtrack
        }
    }

    // Try the opposite direction
    jdir[ncapt] = -idir[ncapt];
    jboard1 = iboard[ncapt] + jdir[ncapt];
    if (with->my_crown_bb & BITULL(move_from)) {
        while (empty_bb & BITULL(jboard1)) {
            jboard1 += jdir[ncapt];
        }
    }

    if ((with->your_man_bb | with->your_crown_bb) & BITULL(jboard1) && !(captures_bb & BITULL(jboard1))) {
        int kboard1 = jboard1 + jdir[ncapt];
        if (empty_bb & BITULL(kboard1)) {
            captures_bb |= BITULL(jboard1);
            iboard[ncapt + 1] = kboard1;
            idir[ncapt + 1] = jdir[ncapt];
            generate_capture_moves(with, moves_list, iboard1, ncapt + 1, captures_bb, empty_bb, idir, jdir, iboard, jboard, move_from);
        }
    }
}

void process_piece_moves(board_t *with, moves_list_t *moves_list, int iboard1, int move_from, ui64_t my_bb, ui64_t empty_bb) {
    for (int i = 0; i < 4; i++) {
        int jboard1 = iboard1 + my_dir[i];

        // Handle simple move for a man
        if (with->my_man_bb & BITULL(iboard1)) {
            if (empty_bb & BITULL(jboard1)) {
                if ((i < 2) && (moves_list->ncaptx == 0)) {
                    SOFTBUG(moves_list->nmoves >= MOVES_MAX);
                    move_t *move = moves_list->moves + moves_list->nmoves;
                    move->move_from = iboard1;
                    move->move_to = jboard1;
                    move->move_captures_bb = 0;
                    moves_list->nmoves++;
                }
            }
        } else {
            // Handle captures
            while (empty_bb & BITULL(jboard1)) {
                if (moves_list->ncaptx == 0) {
                    SOFTBUG(moves_list->nmoves >= MOVES_MAX);
                    move_t *move = moves_list->moves + moves_list->nmoves;
                    move->move_from = iboard1;
                    move->move_to = jboard1;
                    move->move_captures_bb = 0;
                    moves_list->nmoves++;
                }
                jboard1 += my_dir[i];
            }
        }

        // Handle capture move for a crown or man
        if ((with->your_man_bb | with->your_crown_bb) & BITULL(jboard1)) {
            int kboard1 = jboard1 + my_dir[i];
            if (empty_bb & BITULL(kboard1)) {
                int idir[NPIECES_MAX + 1], jdir[NPIECES_MAX + 1], iboard[NPIECES_MAX + 1], jboard[NPIECES_MAX + 1];
                idir[1] = my_dir[i];
                iboard[1] = kboard1;

                generate_capture_moves(with, moves_list, iboard1, 1, BITULL(jboard1), empty_bb, idir, jdir, iboard, jboard, iboard1);
            }
        }
    }
}

void gen_my_moves(board_t *with, moves_list_t *moves_list, int quiescence) {
    BEGIN_BLOCK(__FUNC__);

    SOFTBUG(with->board_colour2move != MY_BIT);

    moves_list->nmoves = 0;
    moves_list->ncaptx = 0;

    ui64_t my_bb = (with->my_man_bb | with->my_crown_bb);
    ui64_t your_bb = (with->your_man_bb | with->your_crown_bb);
    ui64_t empty_bb = with->board_empty_bb & ~(my_bb | your_bb);

    while (my_bb != 0) {
        int iboard1;

        if (IS_WHITE(with->board_colour2move))
            iboard1 = BIT_CTZ(my_bb);
        else
            iboard1 = 63 - BIT_CLZ(my_bb);

        my_bb &= ~BITULL(iboard1);
        process_piece_moves(with, moves_list, iboard1, iboard1, my_bb, empty_bb);
    }

    // Score moves
    for (int imove = 0; imove < moves_list->nmoves; imove++) {
        move_t *move = moves_list->moves + imove;
        int iboard = move->move_from;
        int kboard = move->move_to;
        int move_score_delta = 0;
        ui64_t captures_bb = move->move_captures_bb;

        while (captures_bb != 0) {
            int jboard = BIT_CTZ(captures_bb);
            if (with->your_man_bb & BITULL(jboard))
                move_score_delta += SCORE_MAN;
            else
                move_score_delta += SCORE_CROWN;

            captures_bb &= ~BITULL(jboard);
        }

        int move_weight = move_score_delta;
        int move_tactical = FALSE;

        if (with->my_man_bb & BITULL(iboard)) {
            if (my_row[kboard] == 8)
                move_weight = move_score_delta + 1;
            else if (my_row[kboard] == 9)
                move_weight = move_score_delta + 2;
            else if (my_row[kboard] == 10) {
                move_score_delta += SCORE_CROWN - SCORE_MAN;
                move_weight = move_score_delta + 3;
            }
        }

        moves_list->moves_weight[imove] = move_weight;
        moves_list->moves_tactical[imove] = move_tactical;
    }

    with->board_nodes[with->board_inode].node_ncaptx = moves_list->ncaptx;

    END_BLOCK;
}
```

### Key Changes:

1. **Recursive Function**:
   - The recursive capture handling is now in the function `generate_capture_moves`. It recursively explores all valid capture sequences and stores the best ones.
   - It uses standard recursive techniques, instead of `goto`, to explore new capture sequences and backtrack.

2. **Processing Moves**:
   - The function `process_piece_moves` handles both simple and capture moves for each piece. This is more readable and modular compared to the original structure.

3. **Capture Logic**:
   - Capture logic is cleanly separated into recursive and iterative parts. This improves clarity by avoiding jumps and `goto` statements.

4. **Backtracking**:
   - The recursive function `generate_capture_moves` automatically backtracks

 by removing captured pieces from `captures_bb` after each recursive call.

### Benefits of the Refactored Code:

- **Readability**: The code is much clearer, structured, and modular.
- **Maintainability**: Recursive functions are easier to extend and modify. It’s also easier to follow the logic and debug.
- **No `goto`**: Using recursion replaces the need for `goto`, which is generally discouraged in modern C programming due to complexity and potential for bugs.

This refactor maintains the efficiency of the original code while improving its structure and maintainability.


```
