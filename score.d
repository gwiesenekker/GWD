//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
int return_my_score(board_t *object, moves_list_t *arg_moves_list)
{
  int result = 0;

  //++(with->S_total_evaluations);

  //DO NOT USE board_nmy_man as patterns flip

  int material_score = return_material_score(object);

  if (options.material_only)
  {
    //++(with->S_total_material_only_evaluations);

    result = material_score;
  }
  else
  {
    //++(with->S_total_network_evaluations);

    int nwhite_king_delta = 
      -object->board_network.network_inputs[object->board_network
                                        .nwhite_king_input_map] +
      BIT_COUNT(object->board_white_king_bb);

    if (nwhite_king_delta != 0)
    {
      update_layer0(&(object->board_network),
                    object->board_network.nwhite_king_input_map,
                    nwhite_king_delta);
    }

    int nblack_king_delta = 
      -object->board_network.network_inputs[object->board_network
                                        .nblack_king_input_map] +
      BIT_COUNT(object->board_black_king_bb);

    if (nblack_king_delta != 0)
    {
      update_layer0(&(object->board_network),
                    object->board_network.nblack_king_input_map,
                    nblack_king_delta);
    }

    double network_score =
      return_network_score_scaled(&(object->board_network), FALSE, TRUE);

#ifdef DEBUG
static int n = 0;
    double double_score =
      return_network_score_double(&(object->board_network), FALSE);
  
n++;
if (n < 99) PRINTF("network_score=%.6f double_score=%.6f\n",
network_score, double_score);

    if (fabs(network_score - double_score) > (1.0 / sqrt(SCALED_DOUBLE_FACTOR)))
    {
      my_printf(object->board_my_printf,
        "WARNING network_score=%.6f double_score=%.6f\n",
        network_score, double_score);
    }
#endif

    result = round(network_score * object->board_network.network2material_score);
 
    if (IS_BLACK(object->board_colour2move)) result = -result;

#ifdef GEN_CSV
    if ((BIT_COUNT(with->board_black_king_bb) == 0) and
        (BIT_COUNT(with->board_white_king_bb) == 0))
    { 
       static int ncsv = 0;

       PRINTF("CSV %d,%d\n", material_score, result);
       ncsv++;
       HARDBUG(ncsv > 1000000) 
    }
#endif
  }

  SOFTBUG(result < (SCORE_LOST + NODE_MAX))
  SOFTBUG(result > (SCORE_WON - NODE_MAX))

  return(result);
}

