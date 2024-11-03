//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
void my_pv(int npieces, board_t *with, pv_t *pv, double pv_score, FILE *ffen,
  i64_t *iposition)
{
  moves_list_t moves_list;

  create_moves_list(&moves_list);

  gen_my_moves(with, &moves_list, FALSE);

  if (moves_list.nmoves == 0) return;

  int ncount = 
    BIT_COUNT(with->board_white_man_bb |
              with->board_white_king_bb |
              with->board_black_man_bb |
              with->board_black_king_bb);

  if ((moves_list.nmoves > 1) and
      (moves_list.ncaptx == 0) and
      (ncount == npieces) and
      (fabs(pv_score) < 0.999999))
  {
    char pv_move[MY_LINE_MAX];

    strcpy(pv_move, "NULL");

    if (*pv != INVALID)
    {
      SOFTBUG(*pv >= moves_list.nmoves)
  
      strcpy(pv_move, moves_list.move2string(&moves_list, *pv));
    }

    char fen[MY_LINE_MAX];
  
    board2fen(with->board_id, fen, FALSE);
  
    HARDBUG(fprintf(ffen, "%s {%.6f} ncount=%d pv_move=%s pv_score=C2M\n",
      fen, pv_score, ncount, pv_move) < 0)

    HARDBUG(fflush(ffen) != 0)

    ++(*iposition);
  }

  if (*pv == INVALID) return;

  do_my_move(with, *pv, &moves_list);
  
  your_pv(npieces, with, pv + 1, -pv_score, ffen, iposition);

  undo_my_move(with, *pv, &moves_list);
}
