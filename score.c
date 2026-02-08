// SCU REVISION 8.1178 zo  8 feb 2026  6:06:03 CET
#include "globals.h"

typedef struct
{
  ui64_t SCS_key;
  i32_t SCS_score;
  ui32_t SCS_crc32;
} score_cache_slot_t;

local i64_t nscore_cache_entries;
local score_cache_slot_t score_cache_slot_default;
local score_cache_slot_t *score_cache[NCOLOUR_ENUM];

local int probe_score_cache(search_t *object,
                            score_cache_slot_t *arg_score_cache,
                            score_cache_slot_t *arg_score_cache_slot)
{
  int result = FALSE;

  *arg_score_cache_slot =
    arg_score_cache[object->S_board.B_key % nscore_cache_entries];

  ui32_t crc32 = 0xFFFFFFFF;
  crc32 = HW_CRC32_U64(crc32, arg_score_cache_slot->SCS_key);
  crc32 = HW_CRC32_U32(crc32, arg_score_cache_slot->SCS_score);
  crc32 = ~crc32;

  if (crc32 != arg_score_cache_slot->SCS_crc32)
  {
    object->S_total_score_cache_crc32_errors++;

    *arg_score_cache_slot = score_cache_slot_default;
  }
  else if (arg_score_cache_slot->SCS_key == object->S_board.B_key)
  {
    result = TRUE;
  }

  return (result);
}

local void update_score_cache(search_t *object,
                              score_cache_slot_t *arg_score_cache,
                              score_cache_slot_t *arg_score_cache_slot)
{
  arg_score_cache[object->S_board.B_key % nscore_cache_entries] =
    *arg_score_cache_slot;
}

#undef GEN_CSV

int return_score_from_board(board_t *object, double_t *arg_network_score_scaled)
{
  int result = 0;

  network_thread_t *network_thread = &(object->B_network_thread);

  patterns_thread_t *with_patterns_thread = &(network_thread->NT_patterns);

  for (int input = 0;
       input < network_global.NG_pattern_dense_network_shared.DNS_ninputs;
       input++)
    network_thread->NT_pattern_dense_network_thread.DNT_inputs[input] = 0;

  layer_shared_t *with_current =
    network_global.NG_pattern_dense_network_shared.DNS_layers;

  layer_thread_t *with_current_thread =
    network_thread->NT_pattern_dense_network_thread.DNT_layers;

  for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    with_current_thread->LT_dot[ioutput] = 0;

  for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      patterns_shared.PSS_patterns + ipattern;

    pattern_thread_t *with_pattern_thread =
      with_patterns_thread->PT_patterns + ipattern;

    int istate = base3_index(with_pattern_thread->PT_embed,
                             with_pattern_shared->PS_nlinear);

    if (network_global.NG_architecture == NETWORK_ARCHITECTURE_ADD)
    {
      vadd_ab2a(with_pattern_shared->PS_nembed,
                network_thread->NT_pattern_dense_network_thread.DNT_inputs +
                  with_pattern_shared->PS_offset,
                with_pattern_shared->PS_weights_nstatesxnembed[istate]);
    }
    else
    {
      if (arg_network_score_scaled != NULL)
        vcopy_a2b(with_pattern_shared->PS_nembed,
                  with_pattern_shared->PS_weights_nstatesxnembed[istate],
                  network_thread->NT_pattern_dense_network_thread.DNT_inputs +
                    with_pattern_shared->PS_offset);

      vadd_ab2a(with_current->LS_noutputs,
                with_current_thread->LT_dot,
                with_pattern_shared->PS_sum_nstatesxnoutputs[istate]);
    }
  }

  int nwhite_man = BIT_COUNT(object->B_white_man_bb);
  int nblack_man = BIT_COUNT(object->B_black_man_bb);
  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

  int delta_man = nwhite_man - nblack_man;
  if (delta_man < -DELTA_MAN_MAX)
    delta_man = -DELTA_MAN_MAX;
  if (delta_man > DELTA_MAN_MAX)
    delta_man = DELTA_MAN_MAX;
  delta_man += DELTA_MAN_MAX;

  int delta_king = nwhite_king - nblack_king;
  if (delta_king < -DELTA_KING_MAX)
    delta_king = -DELTA_KING_MAX;
  if (delta_king > DELTA_KING_MAX)
    delta_king = DELTA_KING_MAX;
  delta_king += DELTA_KING_MAX;

  for (int input = 0;
       input < network_global.NG_material_dense_network_shared.DNS_ninputs;
       input++)
    network_thread->NT_material_dense_network_thread.DNT_inputs[input] = 0;

  with_current = network_global.NG_material_dense_network_shared.DNS_layers;

  with_current_thread =
    network_thread->NT_material_dense_network_thread.DNT_layers;

  for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    with_current_thread->LT_dot[ioutput] = 0;

  for (int imaterial = 0; imaterial < materials_global.MSS_nmaterial;
       imaterial++)
  {
    int istate = INVALID;

    if (imaterial == 0)
      istate = nwhite_man;
    else if (imaterial == 1)
      istate = nblack_man;
    else if (imaterial == 2)
      istate = nwhite_king;
    else if (imaterial == 3)
      istate = nblack_king;
    else
    {
      if (materials_global.MSS_nmaterial == 5)
      {
        HARDBUG(imaterial != 4)

        int mwhite_king = nwhite_king;
        int mblack_king = nblack_king;

        if (mwhite_king > COMBINED_KING_MAX)
          mwhite_king = COMBINED_KING_MAX;
        if (mblack_king > COMBINED_KING_MAX)
          mblack_king = COMBINED_KING_MAX;

        istate = (nwhite_man * (COMBINED_KING_MAX + 1) + mwhite_king) * 21 *
                   (COMBINED_KING_MAX + 1) +
                 nblack_man * (COMBINED_KING_MAX + 1) + mblack_king;
      }
      else if (materials_global.MSS_nmaterial == 7)
      {
        if (imaterial == 4)
          istate = delta_man;
        else if (imaterial == 5)
          istate = delta_king;
        else
          istate = delta_man * (2 * DELTA_KING_MAX + 1) + delta_king;
      }
    }

    HARDBUG(istate == INVALID)

    material_shared_t *with_material_shared =
      materials_global.MSS_materials + imaterial;

    if (network_global.NG_architecture == NETWORK_ARCHITECTURE_ADD)
    {
      vadd_ab2a(with_material_shared->MG_nembed,
                network_thread->NT_material_dense_network_thread.DNT_inputs +
                  with_material_shared->MG_offset,
                with_material_shared->MG_weights_nstatesxnembed[istate]);
    }
    else
    {
      if (arg_network_score_scaled != NULL)
        vcopy_a2b(with_material_shared->MG_nembed,
                  with_material_shared->MG_weights_nstatesxnembed[istate],
                  network_thread->NT_material_dense_network_thread.DNT_inputs +
                    with_material_shared->MG_offset);

      vadd_ab2a(with_current->LS_noutputs,
                with_current_thread->LT_dot,
                with_material_shared->MG_sum_nstatesxnoutputs[istate]);
    }
  }

  double network_score_scaled = return_network_score_scaled(
    &(network_global.NG_pattern_dense_network_shared),
    &(object->B_network_thread.NT_pattern_dense_network_thread));

  network_score_scaled = return_network_score_scaled(
    &(network_global.NG_material_dense_network_shared),
    &(object->B_network_thread.NT_material_dense_network_thread));

  // prepare inputs

  dense_network_shared_t *with_pattern_shared =
    &(network_global.NG_pattern_dense_network_shared);

  dense_network_thread_t *with_pattern_thread =
    &(object->B_network_thread.NT_pattern_dense_network_thread);

  vcopy_a2b(
    with_pattern_shared->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
      .LS_noutputs,
    with_pattern_thread->DNT_layers[with_pattern_shared->DNS_nlayers - 1]
      .LT_outputs,
    network_thread->NT_dense_network_thread.DNT_inputs);

  dense_network_shared_t *with_material_shared =
    &(network_global.NG_material_dense_network_shared);

  dense_network_thread_t *with_material_thread =
    &(object->B_network_thread.NT_material_dense_network_thread);

  vcopy_a2b(
    with_material_shared->DNS_layers[with_material_shared->DNS_nlayers - 1]
      .LS_noutputs,
    with_material_thread->DNT_layers[with_material_shared->DNS_nlayers - 1]
      .LT_outputs,
    network_thread->NT_dense_network_thread.DNT_inputs +
      with_pattern_shared->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
        .LS_noutputs);

  network_score_scaled = return_network_score_scaled(
    &(network_global.NG_dense_network_shared),
    &(object->B_network_thread.NT_dense_network_thread));

  if (arg_network_score_scaled != NULL)
    *arg_network_score_scaled = network_score_scaled;

#ifdef DEBUG
  static int n = 0;

  double network_score_double = return_network_score_double(
    &(network_global.NG_dense_network_shared),
    &(object->B_network_thread.NT_dense_network_thread));

  n++;

  if (n < 99)
    PRINTF("network_score_scaled=%.6f network_score_double=%.6f\n",
           network_score_scaled,
           network_score_double);

  if (fabs(network_score_scaled - network_score_double) > 0.001)
  {
    my_printf(object->B_my_printf,
              "WARNING network_score_scaled=%.6f network_score_double=%.6f\n",
              network_score_scaled,
              network_score_double);
  }
#endif

  result =
    round(network_score_scaled * network_global.NG_network2material_score);

  if (IS_BLACK(object->B_colour2move))
    result = -result;

#ifdef GEN_CSV
  if ((BIT_COUNT(object->B_black_king_bb) == 0) and
      (BIT_COUNT(object->B_white_king_bb) == 0))
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

int return_score(search_t *object)
{
  colour_enum my_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  int result = 0;

  score_cache_slot_t score_cache_slot;

  if (options.score_cache_size > 0)
  {
    if (probe_score_cache(object, score_cache[my_colour], &score_cache_slot))
    {
      ++(object->S_total_score_cache_hits);

      result = score_cache_slot.SCS_score;

      return (result);
    }
  }

  board_t *with = &(object->S_board);

  int material_score = return_material_score(with);

  if (options.material_only)
  {
    ++(object->S_total_material_only_evaluations);

    result = material_score;
  }
  else
  {
    ++(object->S_total_network_evaluations);

    result = return_score_from_board(with, NULL);

    if (options.score_cache_size > 0)
    {
      score_cache_slot.SCS_key = object->S_board.B_key;
      score_cache_slot.SCS_score = result;

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = HW_CRC32_U64(crc32, score_cache_slot.SCS_key);
      crc32 = HW_CRC32_U32(crc32, score_cache_slot.SCS_score);
      score_cache_slot.SCS_crc32 = ~crc32;

      update_score_cache(object, score_cache[my_colour], &score_cache_slot);
    }
  }

  HARDBUG(IS_SCORE_LOST(result))
  HARDBUG(IS_SCORE_WON(result))

  return (result);
}

int return_npieces(board_t *self)
{
  board_t *object = self;

  return (
    BIT_COUNT(object->B_white_man_bb) + BIT_COUNT(object->B_white_king_bb) +
    BIT_COUNT(object->B_black_man_bb) + BIT_COUNT(object->B_black_king_bb));
}

int return_material_score(board_t *self)
{
  board_t *object = self;

  int nwhite_man = BIT_COUNT(object->B_white_man_bb);
  int nblack_man = BIT_COUNT(object->B_black_man_bb);
  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

  int material_score = (nwhite_man - nblack_man) * SCORE_MAN +
                       (nwhite_king - nblack_king) * SCORE_KING;
  if (IS_WHITE(object->B_colour2move))
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
