//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
int return_my_score(board_t *with, moves_list_t *moves_list)
{
  int result = 0;

  //++(with->S_total_evaluations);

  //DO NOT USE board_nmy_man as patterns flip

  int nmy_man = BIT_COUNT(with->my_man_bb);
  int nmy_king = BIT_COUNT(with->my_king_bb);
  int nyour_man = BIT_COUNT(with->your_man_bb);
  int nyour_king = BIT_COUNT(with->your_king_bb);
    
  int material_score = (nmy_man - nyour_man) * SCORE_MAN +
                       (nmy_king - nyour_king) * SCORE_KING;

  if (options.material_only)
  {
    //++(with->S_total_material_only_evaluations);

    result = material_score;
  }
  else
  {
    //++(with->S_total_network_evaluations);

    int colour2move = 
      with->board_network.network_inputs[with->board_network
                                       .colour2move_input_map];

    if (IS_WHITE(with->board_colour2move) and (colour2move == 0))
    {
      update_layer0(&(with->board_network),
                    with->board_network.colour2move_input_map, 1);
    }

    if (IS_BLACK(with->board_colour2move) and (colour2move == 1))
    {
      update_layer0(&(with->board_network),
                    with->board_network.colour2move_input_map, -1);
    }

    int nmy_king_delta = 
      -with->board_network.network_inputs[with->board_network
                                        .nmy_king_input_map] +
      BIT_COUNT(with->my_king_bb);

    if (nmy_king_delta != 0)
    {
      update_layer0(&(with->board_network),
                    with->board_network.nmy_king_input_map,
                    nmy_king_delta);
    }

    int nyour_king_delta = 
      -with->board_network.network_inputs[with->board_network
                                        .nyour_king_input_map] +
      BIT_COUNT(with->your_king_bb);

    if (nyour_king_delta != 0)
    {
      update_layer0(&(with->board_network),
                    with->board_network.nyour_king_input_map,
                    nyour_king_delta);
    }

    if (with->board_network.network_wings > 0)
    {
      int nleft_wing_delta = 
        -with->board_network.network_inputs[with->board_network
                                          .nleft_wing_input_map] +
        (BIT_COUNT(with->my_man_bb & left_wing_bb) - 
         BIT_COUNT(with->your_man_bb & left_wing_bb)) * 
        with->board_network.network_wings;
  
      if (nleft_wing_delta != 0)
      {
        update_layer0(&(with->board_network),
                      with->board_network.nleft_wing_input_map,
                      nleft_wing_delta);
      }
  
      int ncenter_delta = 
        -with->board_network.network_inputs[with->board_network
                                          .ncenter_input_map] +
        (BIT_COUNT(with->my_man_bb & center_bb) - 
         BIT_COUNT(with->your_man_bb & center_bb)) *
        with->board_network.network_wings;
  
      if (ncenter_delta != 0)
      {
        update_layer0(&(with->board_network),
                      with->board_network.ncenter_input_map,
                      ncenter_delta);
      }
  
      int nright_wing_delta = 
        -with->board_network.network_inputs[with->board_network
                                          .nright_wing_input_map] +
        (BIT_COUNT(with->my_man_bb & right_wing_bb) - 
         BIT_COUNT(with->your_man_bb & right_wing_bb)) *
        with->board_network.network_wings;
  
      if (nright_wing_delta != 0)
      {
        update_layer0(&(with->board_network),
                      with->board_network.nright_wing_input_map,
                      nright_wing_delta);
      }
    }

    if (with->board_network.network_half > 0)
    {
      int nleft_half_delta = 
        -with->board_network.network_inputs[with->board_network
                                          .nleft_half_input_map] +
        (BIT_COUNT(with->my_man_bb & left_half_bb) - 
         BIT_COUNT(with->your_man_bb & left_half_bb)) * 
        with->board_network.network_half;
  
      if (nleft_half_delta != 0)
      {
        update_layer0(&(with->board_network),
                      with->board_network.nleft_half_input_map,
                      nleft_half_delta);
      }
  
      int nright_half_delta = 
        -with->board_network.network_inputs[with->board_network
                                          .nright_half_input_map] +
        (BIT_COUNT(with->my_man_bb & right_half_bb) - 
         BIT_COUNT(with->your_man_bb & right_half_bb)) * 
        with->board_network.network_half;
  
      if (nright_half_delta != 0)
      {
        update_layer0(&(with->board_network),
                      with->board_network.nright_half_input_map,
                      nright_half_delta);
      }
    }

    //blocked

    if (with->board_network.network_blocked > 0)
    {
      int nblocked_delta = 0;
 
      if (with->board_network.network_blocked == 1)
      {
        nblocked_delta = 
          -with->board_network.network_inputs[with->board_network
                                            .nblocked_input_map] +
        moves_list->nblocked;
      }
      else if (with->board_network.network_blocked == 2)
      {
        nblocked_delta = 
          -with->board_network.network_inputs[with->board_network
                                            .nblocked_input_map] +
          2 * nmy_man - moves_list->nblocked;
      }
  
      if (nblocked_delta != 0)
      {
        update_layer0(&(with->board_network),
                      with->board_network.nblocked_input_map,
                      nblocked_delta);

        HARDBUG(with->board_network.network_inputs[with->board_network
                                                 .nblocked_input_map] < 0)
      }
    }

    double network_score =
      return_network_score_scaled(&(with->board_network), FALSE, TRUE);

#ifdef DEBUG
static int n = 0;
    double double_score =
      return_network_score_double(&(with->board_network), FALSE);
  
n++;
if (n < 99) PRINTF("network_score=%.6f double_score=%.6f\n",
network_score, double_score);

    if (fabs(network_score - double_score) > (1.0 / sqrt(SCALED_DOUBLE_FACTOR)))
    {
      my_printf(with->board_my_printf,
        "WARNING network_score=%.6f double_score=%.6f\n",
        network_score, double_score);
    }
#endif

    result = round(network_score * with->board_network.network2material_score);
  }

  SOFTBUG(result < (SCORE_LOST + NODE_MAX))
  SOFTBUG(result > (SCORE_WON - NODE_MAX))

  return(result);
}

