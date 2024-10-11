//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
int return_my_score(board_t *with)
{
  int result = 0;

  ++(with->total_evaluations);

  //DO NOT USE board_nmy_man as patterns flip

  int nmy_man = BIT_COUNT(with->my_man_bb);
  int nmy_crown = BIT_COUNT(with->my_crown_bb);
  int nyour_man = BIT_COUNT(with->your_man_bb);
  int nyour_crown = BIT_COUNT(with->your_crown_bb);
    
  int material_score = (nmy_man - nyour_man) * SCORE_MAN +
                       (nmy_crown - nyour_crown) * SCORE_CROWN;

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
    neural_t *neural1 = &(with->board_neural1);

    if ((neural0->neural_input_map == NEURAL_INPUT_MAP_V6) or
        (neural1->neural_input_map == NEURAL_INPUT_MAP_V6))
    {
      if ((nmy_crown == 0) and (nyour_crown == 0))
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
      int npieces = nmy_man + nmy_crown + nyour_man + nyour_crown;

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

    HARDBUG(neural == NULL)

    double neural_score =
      return_neural_score_scaled(neural, FALSE, TRUE);

#ifdef DEBUG
    double debug_score =
      return_neural_score_double(neural, FALSE);
  
    debug_score = DOUBLE2SCALED(debug_score);
    debug_score = SCALED2DOUBLE(debug_score);

    //PRINTF("%.6f %.6f\n", neural_score, debug_score);
    if (fabs(neural_score - debug_score) > (1.0 / sqrt(SCALED_DOUBLE_FACTOR)))
    {
      my_printf(with->board_my_printf,
        "WARNING neural_score=%.6f debug_score=%.6f\n",
        neural_score, debug_score);
    }
#endif

    if ((neural->neural_output == NEURAL_OUTPUT_W2M) and
        IS_BLACK(with->board_colour2move))
    {
      neural_score = -neural_score; 
    }

    int score = round(neural_score * neural->neural2material_score);
    
    if ((options.material_blend_parameter == 0) or
         ((nmy_crown == 0) and (nyour_crown == 0)))
    {
      result = score;
    }
    else
    {
      double delta = abs(score - material_score) / (double) SCORE_MAN;

      double threshold = options.material_blend_parameter / 100.0;

      double huber = 0.0;

      if (delta <= threshold)
        huber = delta * delta / 2.0;
      else
        huber = threshold * (delta - 0.5 * threshold);

      double alpha = 2.0 / (1.0 + exp(huber * delta));

      result = round(alpha * score + (1.0 - alpha) * material_score);
    }
  }

  SOFTBUG(result < (SCORE_LOST + NODE_MAX))
  SOFTBUG(result > (SCORE_WON - NODE_MAX))

  return(result);
}

