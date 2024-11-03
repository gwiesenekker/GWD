//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
int return_my_score(board_t *with)
{
  int result = 0;

  ++(with->total_evaluations);

  //DO NOT USE board_nmy_man as patterns flip

  int nmy_man = BIT_COUNT(with->my_man_bb);
  int nmy_king = BIT_COUNT(with->my_king_bb);
  int nyour_man = BIT_COUNT(with->your_man_bb);
  int nyour_king = BIT_COUNT(with->your_king_bb);
    
  int material_score = (nmy_man - nyour_man) * SCORE_MAN +
                       (nmy_king - nyour_king) * SCORE_CROWN;

  if (options.material_only)
  {
    ++(with->total_material_only_evaluations);

    result = material_score;
  }
  else
  {
    ++(with->total_neural_evaluations);

    neural_t *neural = NULL;

    neural_t *neural0 = &(with->board_neural0);

    if (!(with->board_neural1_not_null))
    {
      neural = neural0;
    }
    else
    {
      neural_t *neural1 = &(with->board_neural1);
  
      if ((neural0->neural_input_map == NEURAL_INPUT_MAP_V6) or
          (neural1->neural_input_map == NEURAL_INPUT_MAP_V6))
      {
        if ((nmy_king == 0) and (nyour_king == 0))
        {
          if (neural0->neural_input_map == NEURAL_INPUT_MAP_V6)
            neural = neural0;
          else
            neural = neural1;
        }
        else
        {
          if (neural0->neural_input_map != NEURAL_INPUT_MAP_V6)
            neural = neural0;
          else
            neural = neural1;
        }
      } 
      else
      {
        int npieces = nmy_man + nmy_king + nyour_man + nyour_king;
  
        if ((npieces >= neural0->neural_npieces_min) and 
            (npieces <= neural0->neural_npieces_max))
        {
          neural = neural0;
        }
        else if ((npieces >= neural1->neural_npieces_min) and 
                 (npieces <= neural1->neural_npieces_max))
        {
          neural = neural1;
        }
      }
    }

    HARDBUG(neural == NULL)

    double neural_score =
      return_neural_score_scaled(neural, FALSE, TRUE);

#ifdef DEBUG
static int n = 0;
    double double_score =
      return_neural_score_double(neural, FALSE);
  
n++;
if (n < 99) PRINTF("neural_score=%.6f double_score=%.6f\n",
neural_score, double_score);

    if (fabs(neural_score - double_score) > (1.0 / sqrt(SCALED_DOUBLE_FACTOR)))
    {
      my_printf(with->board_my_printf,
        "WARNING neural_score=%.6f double_score=%.6f\n",
        neural_score, double_score);
    }
#endif

    if ((neural->neural_output == NEURAL_OUTPUT_W2M) and
        IS_BLACK(with->board_colour2move))
    {
      neural_score = -neural_score; 
    }

    result = round(neural_score * neural->neural2material_score);
  }

  SOFTBUG(result < (SCORE_LOST + NODE_MAX))
  SOFTBUG(result > (SCORE_WON - NODE_MAX))

  return(result);
}

