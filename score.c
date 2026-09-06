// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST
#include "globals.h"

typedef struct
{
  ui64_t SCS_key;
  score_t SCS_score;
  ui32_t SCS_crc32;
} score_cache_slot_t;

local i64_t nscore_cache_entries;
local score_cache_slot_t score_cache_slot_default;
local score_cache_slot_t *score_cache[NCOLOUR_ENUM];

local int probe_score_cache(search_t *arg_with_search,
                            score_cache_slot_t *arg_score_cache,
                            score_cache_slot_t *arg_score_cache_slot)
{
  int result = FALSE;

  *arg_score_cache_slot =
    arg_score_cache[arg_with_search->S_board.B_key % nscore_cache_entries];

  ui32_t crc32 = 0xFFFFFFFF;
  crc32 = HW_CRC32_U64(crc32, arg_score_cache_slot->SCS_key);
  crc32 = HW_CRC32_U32(crc32, arg_score_cache_slot->SCS_score);
  crc32 = ~crc32;

  if (crc32 != arg_score_cache_slot->SCS_crc32)
  {
    arg_with_search->S_total_score_cache_crc_errors++;

    *arg_score_cache_slot = score_cache_slot_default;
  }
  else if (arg_score_cache_slot->SCS_key == arg_with_search->S_board.B_key)
  {
    result = TRUE;
  }

  return (result);
}

local void update_score_cache(search_t *arg_with_search,
                              score_cache_slot_t *arg_score_cache,
                              score_cache_slot_t *arg_score_cache_slot)
{
  arg_score_cache[arg_with_search->S_board.B_key % nscore_cache_entries] =
    *arg_score_cache_slot;
}

#undef GEN_CSV

score_t return_score_from_board(board_t *arg_with_board,
                                double_t *arg_network_score_scaled)
{
  score_t result = SCORE(0);

  network_t *with_network = networks + arg_with_board->B_network_id;

  network_state_t *network_state = &(arg_with_board->B_network_state);

  for (int input = 0;
       input < with_network->N_pattern_dense_network_shared.DNS_ninputs;
       input++)
    network_state->NS_pattern_dense_network_state.DNS_inputs[input] = 0;

  layer_shared_t *with_current =
    with_network->N_pattern_dense_network_shared.DNS_layers;

  layer_thread_t *with_current_thread =
    network_state->NS_pattern_dense_network_state.DNS_layers;

  for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    with_current_thread->LT_dot[ioutput] = 0;

  int nwhite_man = BIT_COUNT(arg_with_board->B_white_man_bb);
  int nblack_man = BIT_COUNT(arg_with_board->B_black_man_bb);
  int nwhite_king = BIT_COUNT(arg_with_board->B_white_king_bb);
  int nblack_king = BIT_COUNT(arg_with_board->B_black_king_bb);

  for (int ipattern = 0;
       ipattern < with_network->N_patterns_shared.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_patterns_shared.PSS_patterns + ipattern;

    int istate = INVALID;

    if (with_pattern_shared->PS_id == PATTERN_BASE3_INDEX_ID)
    {
      istate = base3_index_v2(arg_with_board, with_pattern_shared->PS_bb);
    }
    else if (with_pattern_shared->PS_id == PATTERN_NWHITE_MAN_ID)
    {
      istate = nwhite_man;
    }
    else if (with_pattern_shared->PS_id == PATTERN_NBLACK_MAN_ID)
    {
      istate = nblack_man;
    }
    else if (with_pattern_shared->PS_id == PATTERN_NWHITE_KING_ID)
    {
      istate = nwhite_king;

      if (istate > PATTERN_NKING_MAX)
        istate = PATTERN_NKING_MAX;
    }
    else if (with_pattern_shared->PS_id == PATTERN_NBLACK_KING_ID)
    {
      istate = nblack_king;

      if (istate > PATTERN_NKING_MAX)
        istate = PATTERN_NKING_MAX;
    }
    else if (with_pattern_shared->PS_id == PATTERN_WHITE_MOBILITY_ID)
    {
      istate = return_mobility(arg_with_board, WHITE_ENUM);
    }
    else if (with_pattern_shared->PS_id == PATTERN_BLACK_MOBILITY_ID)
    {
      istate = return_mobility(arg_with_board, BLACK_ENUM);
    }
    else if (with_pattern_shared->PS_id == PATTERN_WHITE_IMMOBILITY_ID)
    {
      istate = return_immobility(arg_with_board, WHITE_ENUM);
    }
    else if (with_pattern_shared->PS_id == PATTERN_BLACK_IMMOBILITY_ID)
    {
      istate = return_immobility(arg_with_board, BLACK_ENUM);
    }
    else if (with_pattern_shared->PS_id == PATTERN_NWHITE_KING_DIAGONAL_5_46_ID)
    {
      istate = BIT_COUNT(arg_with_board->B_white_king_bb & diagonal_5_46_bb);
    }
    else if (with_pattern_shared->PS_id == PATTERN_NBLACK_KING_DIAGONAL_5_46_ID)
    {
      istate = BIT_COUNT(arg_with_board->B_black_king_bb & diagonal_5_46_bb);
    }

    HARDBUG(istate == INVALID)

    if (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_ADD)
    {
      vadd_ab2a(with_pattern_shared->PS_nembed,
                network_state->NS_pattern_dense_network_state.DNS_inputs +
                  with_pattern_shared->PS_offset,
                with_pattern_shared->PS_weights_nstatesxnembed[istate]);
    }
    else
    {
      if (arg_network_score_scaled != NULL)
        vcopy_a2b(with_pattern_shared->PS_nembed,
                  with_pattern_shared->PS_weights_nstatesxnembed[istate],
                  network_state->NS_pattern_dense_network_state.DNS_inputs +
                    with_pattern_shared->PS_offset);

      vadd_ab2a(with_current->LS_noutputs,
                with_current_thread->LT_dot,
                with_pattern_shared->PS_sum_nstatesxnoutputs[istate]);
    }
  }

  double network_score_scaled = return_dense_network_score_scaled(
    &(with_network->N_pattern_dense_network_shared),
    &(arg_with_board->B_network_state.NS_pattern_dense_network_state));

  if (with_network->N_architecture_material != NETWORK_ARCHITECTURE_NULL)
  {
    for (int input = 0;
         input < with_network->N_material_dense_network_shared.DNS_ninputs;
         input++)
      network_state->NS_material_dense_network_state.DNS_inputs[input] = 0;

    with_current = with_network->N_material_dense_network_shared.DNS_layers;

    with_current_thread =
      network_state->NS_material_dense_network_state.DNS_layers;

    for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
      with_current_thread->LT_dot[ioutput] = 0;

    for (int imaterial = 0;
         imaterial < with_network->N_materials_shared.MSS_nmaterials;
         imaterial++)
    {
      material_shared_t *with_material_shared =
        with_network->N_materials_shared.MSS_materials + imaterial;

      int istate = INVALID;

      if (with_material_shared->MS_id == MATERIAL_NWHITE_MAN_ID)
      {
        istate = nwhite_man;
      }
      else if (with_material_shared->MS_id == MATERIAL_NBLACK_MAN_ID)
      {
        istate = nblack_man;
      }
      else if (with_material_shared->MS_id == MATERIAL_NWHITE_KING_ID)
      {
        istate = nwhite_king;

        if (istate >= with_material_shared->MS_nstates)
          istate = with_material_shared->MS_nstates - 1;
      }
      else if (with_material_shared->MS_id == MATERIAL_NBLACK_KING_ID)
      {
        istate = nblack_king;

        if (istate >= with_material_shared->MS_nstates)
          istate = with_material_shared->MS_nstates - 1;
      }
      else if (with_material_shared->MS_id == MATERIAL_MAN_KING_ID)
      {
        int mwhite_king = nwhite_king;
        int mblack_king = nblack_king;

        if (mwhite_king > MATERIAL_KING_MAX)
          mwhite_king = MATERIAL_KING_MAX;
        if (mblack_king > MATERIAL_KING_MAX)
          mblack_king = MATERIAL_KING_MAX;

        istate = (nwhite_man * (MATERIAL_KING_MAX + 1) + mwhite_king) *
                   (NPIECES_MAX + 1) * (MATERIAL_KING_MAX + 1) +
                 nblack_man * (MATERIAL_KING_MAX + 1) + mblack_king;
      }
      else if (with_material_shared->MS_id == MATERIAL_WHITE_MOBILITY_ID)
      {
        istate = return_mobility(arg_with_board, WHITE_ENUM);
      }
      else if (with_material_shared->MS_id == MATERIAL_BLACK_MOBILITY_ID)
      {
        istate = return_mobility(arg_with_board, BLACK_ENUM);
      }
      else if (with_material_shared->MS_id == MATERIAL_WHITE_IMMOBILITY_ID)
      {
        istate = return_immobility(arg_with_board, WHITE_ENUM);
      }
      else if (with_material_shared->MS_id == MATERIAL_BLACK_IMMOBILITY_ID)
      {
        istate = return_immobility(arg_with_board, BLACK_ENUM);
      }
      else if (with_material_shared->MS_id == MATERIAL_WHITE_UNSUPPORTED_ID)
      {
        istate = return_unsupported(arg_with_board, WHITE_ENUM);
      }
      else if (with_material_shared->MS_id == MATERIAL_BLACK_UNSUPPORTED_ID)
      {
        istate = return_unsupported(arg_with_board, BLACK_ENUM);
      }

      HARDBUG(istate == INVALID)

      if (with_network->N_architecture_material == NETWORK_ARCHITECTURE_ADD)
      {
        vadd_ab2a(with_material_shared->MS_nembed,
                  network_state->NS_material_dense_network_state.DNS_inputs +
                    with_material_shared->MS_offset,
                  with_material_shared->MS_weights_nstatesxnembed[istate]);
      }
      else if (with_network->N_architecture_material ==
               NETWORK_ARCHITECTURE_CONCAT)
      {
        if (arg_network_score_scaled != NULL)
          vcopy_a2b(with_material_shared->MS_nembed,
                    with_material_shared->MS_weights_nstatesxnembed[istate],
                    network_state->NS_material_dense_network_state.DNS_inputs +
                      with_material_shared->MS_offset);

        vadd_ab2a(with_current->LS_noutputs,
                  with_current_thread->LT_dot,
                  with_material_shared->MS_sum_nstatesxnoutputs[istate]);
      }
      else
        FATAL("unknown with_network->N_architecture_maerial", EXIT_FAILURE)
    }

    network_score_scaled = return_dense_network_score_scaled(
      &(with_network->N_material_dense_network_shared),
      &(arg_with_board->B_network_state.NS_material_dense_network_state));
  }

  // prepare inputs

  dense_network_shared_t *with_pattern_shared =
    &(with_network->N_pattern_dense_network_shared);

  dense_network_state_t *with_pattern_thread =
    &(arg_with_board->B_network_state.NS_pattern_dense_network_state);

  vcopy_a2b(
    with_pattern_shared->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
      .LS_noutputs,
    with_pattern_thread->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
      .LT_outputs,
    network_state->NS_dense_network_state.DNS_inputs);

  if (with_network->N_architecture_material != NETWORK_ARCHITECTURE_NULL)
  {
    dense_network_shared_t *with_material_shared =
      &(with_network->N_material_dense_network_shared);

    dense_network_state_t *with_material_thread =
      &(arg_with_board->B_network_state.NS_material_dense_network_state);

    vcopy_a2b(
      with_material_shared->DNS_layers[with_material_shared->DNS_nlayers - 1]
        .LS_noutputs,
      with_material_thread->DNS_layers[with_material_shared->DNS_nlayers - 1]
        .LT_outputs,
      network_state->NS_dense_network_state.DNS_inputs +
        with_pattern_shared->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
          .LS_noutputs);
  }

  for (int isignal = 0; isignal < with_network->N_signals_shared.SS_nsignals;
       isignal++)
  {
    signal_shared_t *with_signal_shared =
      with_network->N_signals_shared.SS_signals + isignal;

    if (with_signal_shared->SS_id == SIGNAL_NULL_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] = 0.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_NWHITE_MAN_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] = (float)nwhite_man / 20.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_NBLACK_MAN_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] = (float)nblack_man / 20.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_NWHITE_KING_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] = (float)nwhite_king / 20.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_NBLACK_KING_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] = (float)nblack_king / 20.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_NPIECES_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] =
        (float)(nwhite_man + nblack_man + nwhite_king + nblack_king) / 40.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_NWHITE_KING_DIAGONAL_5_46_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] =
        (float)BIT_COUNT(arg_with_board->B_white_king_bb & diagonal_5_46_bb) /
        10.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_NBLACK_KING_DIAGONAL_5_46_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] =
        (float)BIT_COUNT(arg_with_board->B_black_king_bb & diagonal_5_46_bb) /
        10.0f;
    }
    else if (with_signal_shared->SS_id == SIGNAL_24_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] =
        return_signal_24(arg_with_board);
    }
    else if (with_signal_shared->SS_id == SIGNAL_27_ID)
    {
      network_state->NS_dense_network_state
        .DNS_inputs[with_signal_shared->SS_offset] =
        return_signal_27(arg_with_board);
    }
    else
      FATAL("unknown with_signal_shared->SS_id", EXIT_FAILURE)
  }

  network_score_scaled = return_dense_network_score_scaled(
    &(with_network->N_dense_network_shared),
    &(arg_with_board->B_network_state.NS_dense_network_state));

  if (arg_network_score_scaled != NULL)
    *arg_network_score_scaled = network_score_scaled;

#ifdef DEBUG
  static int n = 0;

  double network_score_double = return_dense_network_score_double(
    &(with_network->N_dense_network_shared),
    &(arg_with_board->B_network_state.NS_dense_network_state));

  n++;

  if (n < 99)
    PRINTF("network_score_scaled=%.6f network_score_double=%.6f\n",
           network_score_scaled,
           network_score_double);

  if (fabs(network_score_scaled - network_score_double) > 0.001)
  {
    my_printf(arg_with_board->B_my_printf,
              "WARNING network_score_scaled=%.6f network_score_double=%.6f\n",
              network_score_scaled,
              network_score_double);
  }
#endif

  result = round(network_score_scaled * with_network->N_network2material_score);

  if (IS_BLACK(arg_with_board->B_colour2move))
    result = -result;

#ifdef GEN_CSV
  if ((BIT_COUNT(arg_with_board->B_black_king_bb) == 0) and
      (BIT_COUNT(arg_with_board->B_white_king_bb) == 0))
  {
    static int ncsv = 0;

    PRINTF("CSV %d,%d\n", material_score, result);
    ncsv++;
    HARDBUG(ncsv > 1000000)
  }
#endif

  HARDBUG(IS_SCORE_LOST(result))
  HARDBUG(IS_SCORE_WON(result))

  return (result);
}

score_t return_score(search_t *arg_with_search)
{
  colour_enum my_colour;

  if (IS_WHITE(arg_with_search->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  score_t result = SCORE(0);

  score_cache_slot_t score_cache_slot;

  if (options.score_cache_size > 0)
  {
    if (probe_score_cache(arg_with_search,
                          score_cache[my_colour],
                          &score_cache_slot))
    {
      ++(arg_with_search->S_total_score_cache_hits);

      result = score_cache_slot.SCS_score;

      return (result);
    }
  }

  board_t *with = &(arg_with_search->S_board);

  score_t material_score = return_material_score(with);

  if (options.material_only)
  {
    ++(arg_with_search->S_total_material_only_evaluations);

    result = material_score;
  }
  else
  {
    ++(arg_with_search->S_total_network_evaluations);

    result = return_score_from_board(with, NULL);

    if (options.score_cache_size > 0)
    {
      score_cache_slot.SCS_key = arg_with_search->S_board.B_key;
      score_cache_slot.SCS_score = result;

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = HW_CRC32_U64(crc32, score_cache_slot.SCS_key);
      crc32 = HW_CRC32_U32(crc32, score_cache_slot.SCS_score);
      score_cache_slot.SCS_crc32 = ~crc32;

      update_score_cache(arg_with_search,
                         score_cache[my_colour],
                         &score_cache_slot);
    }
  }

  HARDBUG(IS_SCORE_LOST(result))
  HARDBUG(IS_SCORE_WON(result))

  return (result);
}

int return_npieces(board_t *arg_with_board)
{
  return (BIT_COUNT(arg_with_board->B_white_man_bb) +
          BIT_COUNT(arg_with_board->B_white_king_bb) +
          BIT_COUNT(arg_with_board->B_black_man_bb) +
          BIT_COUNT(arg_with_board->B_black_king_bb));
}

score_t return_material_score(board_t *arg_with_board)
{
  int nwhite_man = BIT_COUNT(arg_with_board->B_white_man_bb);
  int nblack_man = BIT_COUNT(arg_with_board->B_black_man_bb);
  int nwhite_king = BIT_COUNT(arg_with_board->B_white_king_bb);
  int nblack_king = BIT_COUNT(arg_with_board->B_black_king_bb);

  score_t material_score = SCORE((nwhite_man - nblack_man) * SCORE_MAN +
                                 (nwhite_king - nblack_king) * SCORE_KING);
  if (IS_WHITE(arg_with_board->B_colour2move))
    return (material_score);
  else
    return (-material_score);
}

local void clear_score_caches(void)
{
  PRINTF("clear_score_caches..\n");

  for (i64_t ientry = 0; ientry < nscore_cache_entries; ientry++)
    score_cache[WHITE_ENUM][ientry] = score_cache_slot_default;

  for (i64_t ientry = 0; ientry < nscore_cache_entries; ientry++)
    score_cache[BLACK_ENUM][ientry] = score_cache_slot_default;

  PRINTF("..done\n");
}

void init_score(void)
{
  PRINTF("sizeof(score_cache_slot_t)=%lld\n",
         (i64_t)sizeof(score_cache_slot_t));

  score_cache[WHITE_ENUM] = NULL;
  score_cache[BLACK_ENUM] = NULL;

  nscore_cache_entries = 0;

  if (options.score_cache_size > 0)
  {
    i64_t score_cache_size = options.score_cache_size * MBYTE / 2;

    nscore_cache_entries =
      first_prime_below(roundf(score_cache_size / sizeof(score_cache_slot_t)));

    HARDBUG(nscore_cache_entries < 3)

    PRINTF("nscore_cache_entries=%lld\n", nscore_cache_entries);

    score_cache_slot_default.SCS_key = 0;
    score_cache_slot_default.SCS_score = SCORE_MINUS_INFINITY;

    ui32_t crc32 = 0xFFFFFFFF;
    crc32 = HW_CRC32_U64(crc32, score_cache_slot_default.SCS_key);
    crc32 = HW_CRC32_U32(crc32, score_cache_slot_default.SCS_score);
    score_cache_slot_default.SCS_crc32 = ~crc32;

    MY_MALLOC_BY_TYPE(score_cache[WHITE_ENUM],
                      score_cache_slot_t,
                      nscore_cache_entries)

    MY_MALLOC_BY_TYPE(score_cache[BLACK_ENUM],
                      score_cache_slot_t,
                      nscore_cache_entries)

    clear_score_caches();
  }
}
