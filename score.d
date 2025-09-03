//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
int return_my_score(search_t *object, moves_list_t *arg_moves_list)
{
  int result = 0;

  score_cache_slot_t score_cache_slot;

  if (options.score_cache_size > 0)
  {
    if (probe_score_cache(object, my_score_cache, &score_cache_slot))
    {
       ++(object->S_total_score_cache_hits);
       
       result = score_cache_slot.SCS_score;

       return(result);
    }
  }

  board_t *with = &(object->S_board);

  //DO NOT USE board_nmy_man as PS_patterns flip

  int material_score = return_material_score(with);

  if (options.material_only)
  {
    ++(object->S_total_material_only_evaluations);

    result = material_score;

    return(result);
  }
  else
  {
    ++(object->S_total_network_evaluations);

    network_thread_t *network_thread = &(with->B_network_thread);

    patterns_shared_t *with_patterns_shared = &patterns_shared;

    patterns_thread_t *with_patterns_thread = &(network_thread->NT_patterns);

    for (int input = 0; input < network_shared.NS_ninputs; input++)
      network_thread->NT_inputs[input] = 0;

    layer_shared_t *with_current = network_shared.NS_layers;

    layer_thread_t *with_current_thread = network_thread->NT_layers;

    for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
      with_current_thread->LT_dot[ioutput] = 0;

    for (int ipattern = 0; ipattern < with_patterns_shared->PS_npatterns;
         ipattern++)
    {
      pattern_shared_t *with_pattern_shared =
        with_patterns_shared->PS_patterns + ipattern;

      pattern_thread_t *with_pattern_thread =
        with_patterns_thread->PT_patterns + ipattern;
  
      int istate = base3_index(with_pattern_thread->PT_embed,
                               with_pattern_shared->PS_nlinear);
  
      if ((network_shared.NS_embedding == NETWORK_EMBEDDING_SUM) or
          (network_shared.NS_embedding == NETWORK_EMBEDDING_SUM2))
      {
        vadd_aba(with_pattern_shared->PS_nembed,
                 network_thread->NT_inputs + with_pattern_shared->PS_offset,
                 with_pattern_shared->PS_weights_nstatesxnembed[istate]);
      }
      else if (network_shared.NS_embedding == NETWORK_EMBEDDING_CONCAT)
      {
#ifdef DEBUG
        vcopy_ab(with_pattern_shared->PS_nembed,
                 with_pattern_shared->PS_weights_nstatesxnembed[istate],
                 network_thread->NT_inputs + with_pattern_shared->PS_offset);
#endif
        vadd_aba(with_current->LS_noutputs, with_current_thread->LT_dot,
                 with_pattern_shared->PS_sum_nstatesxnoutputs[istate]);
      }
      else
        FATAL("unknown embedding", EXIT_FAILURE)
    }

    int nwhite_man = BIT_COUNT(with->B_white_man_bb);
    int nblack_man = BIT_COUNT(with->B_black_man_bb);
    int nwhite_king = BIT_COUNT(with->B_white_king_bb);
    int nblack_king = BIT_COUNT(with->B_black_king_bb);
  
    for (int imaterial = 0; imaterial < 4; imaterial++)
    {
      int istate;

      if (imaterial == 0)
        istate = nwhite_man;
      else if (imaterial == 1)
        istate = nblack_man;
      else if (imaterial == 2)
        istate = nwhite_king;
      else
        istate = nblack_king;

      material_shared_t *with_material_shared =
        &(network_shared.NS_material[imaterial]);
  
      if ((network_shared.NS_embedding == NETWORK_EMBEDDING_SUM) or
          (network_shared.NS_embedding == NETWORK_EMBEDDING_SUM2))
      {
        vadd_aba(with_material_shared->MS_nembed,
                 network_thread->NT_inputs + with_material_shared->MS_offset,
                 with_material_shared->MS_weights_nstatesxnembed[istate]);
      }
      else if (network_shared.NS_embedding == NETWORK_EMBEDDING_CONCAT)
      {
#ifdef DEBUG
        vcopy_ab(with_material_shared->MS_nembed,
                 with_material_shared->MS_weights_nstatesxnembed[istate],
                 network_thread->NT_inputs + with_material_shared->MS_offset);
#endif
        vadd_aba(with_current->LS_noutputs, with_current_thread->LT_dot,
                 with_material_shared->MS_sum[istate]);
      }
      else
        FATAL("unknown embedding", EXIT_FAILURE)
    }

    double network_score =
      return_network_score_scaled(&(with->B_network_thread));

#ifdef DEBUG
static int n = 0;
    double double_score =
      return_network_score_double(&(with->B_network_thread));
  
n++;
if (n < 99) PRINTF("network_score=%.6f double_score=%.6f\n",
network_score, double_score);

    if (fabs(network_score - double_score) > 0.001)
    {
      my_printf(with->B_my_printf,
        "WARNING network_score=%.6f double_score=%.6f\n",
        network_score, double_score);
    }
#endif

    result = round(network_score * network_shared.NS_network2material_score);
 
    if (IS_BLACK(with->B_colour2move)) result = -result;

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

    if (options.score_cache_size > 0)
    {
      score_cache_slot.SCS_key = object->S_board.B_key;
      score_cache_slot.SCS_score = result;
  
      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = HW_CRC32_U64(crc32, score_cache_slot.SCS_key);
      crc32 = HW_CRC32_U32(crc32, score_cache_slot.SCS_score);
      score_cache_slot.SCS_crc32 = ~crc32;

      update_score_cache(object, my_score_cache, &score_cache_slot);
    }
  }

  SOFTBUG(result < (SCORE_LOST + NODE_MAX))
  SOFTBUG(result > (SCORE_WON - NODE_MAX))

  return(result);
}

