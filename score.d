//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
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

    if (object->B_network.N__king_weight > 0)
    {
      int nwhite_king_delta = 
        -object->B_network.N_inputs[object->B_network
                                          .N_nwhite_king_input_map] +
        BIT_COUNT(object->B_white_king_bb);
  
      if (nwhite_king_delta != 0)
      {
        update_layer0(&(object->B_network),
                      object->B_network.N_nwhite_king_input_map,
                      nwhite_king_delta * object->B_network.N__king_weight);
      }
  
      int nblack_king_delta = 
        -object->B_network.N_inputs[object->B_network
                                          .N_nblack_king_input_map] +
        BIT_COUNT(object->B_black_king_bb);
  
      if (nblack_king_delta != 0)
      {
        update_layer0(&(object->B_network),
                      object->B_network.N_nblack_king_input_map,
                      nblack_king_delta * object->B_network.N__king_weight);
      }
    }

    double network_score =
      return_network_score_scaled(&(object->B_network), FALSE, TRUE);

#ifdef DEBUG
static int n = 0;
    double double_score =
      return_network_score_double(&(object->B_network), FALSE);
  
n++;
if (n < 99) PRINTF("network_score=%.6f double_score=%.6f\n",
network_score, double_score);

    if (fabs(network_score - double_score) > (1.0 / sqrt(SCALED_DOUBLE_FACTOR)))
    {
      my_printf(object->B_my_printf,
        "WARNING network_score=%.6f double_score=%.6f\n",
        network_score, double_score);
    }
#endif

    if (object->B_network.N__king_weight == NETWORK_KING_WEIGHT_ARES)
    {
      int nwhite_man = BIT_COUNT(object->B_white_man_bb);
      int nblack_man = BIT_COUNT(object->B_black_man_bb);
      int nwhite_kings = BIT_COUNT(object->B_white_king_bb);
      int nblack_kings = BIT_COUNT(object->B_black_king_bb);
   
      if ((nwhite_kings - nblack_kings) != 0)
      {
        //v = 0.082487 * x + 0.223163 * y * (0.5 + 0.5 * (1.0 - z / 40.0))
  
        network_score += 0.223163 * (nwhite_kings - nblack_kings) *
                         (0.5 + 0.5 / (1.0 - (nwhite_man + nblack_man) / 40.0));
      }
   
      result = round(network_score * (SCORE_MAN / 0.082487));
    }
    else if (object->B_network.N__king_weight == NETWORK_KING_WEIGHT_GWD)
    {
      int nwhite_man = BIT_COUNT(object->B_white_man_bb);
      int nblack_man = BIT_COUNT(object->B_black_man_bb);
      int nwhite_kings = BIT_COUNT(object->B_white_king_bb);
      int nblack_kings = BIT_COUNT(object->B_black_king_bb);
   
      if ((nwhite_kings - nblack_kings) != 0)
      {
        //v = 0.121933 * x + 0.509370 * y * (0.5 + 0.5 * (1.0 - z / 40.0))

        network_score += 0.509370 * (nwhite_kings - nblack_kings) *
                         (0.5 + 0.5 / (1.0 - (nwhite_man + nblack_man) / 40.0));
      }
   
      result = round(network_score * (SCORE_MAN / 0.121933));
    }
    else
    {
      result = round(network_score * object->B_network.N_network2material_score);
    }
 
    if (IS_BLACK(object->B_colour2move)) result = -result;

#ifdef GEN_CSV
    if ((BIT_COUNT(with->B_black_king_bb) == 0) and
        (BIT_COUNT(with->B_white_king_bb) == 0))
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

