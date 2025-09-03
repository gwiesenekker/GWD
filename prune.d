//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
  if ((your_depth == 0) and
      (IS_MINIMAL_WINDOW(arg_node_type)) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_PRUNED_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_NULL_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_REDUCED_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
      (moves_list.ML_ncaptx == 0) and
      (moves_list.ML_nmoves >= 4) and
      (arg_reduction_depth_root == 0) and
      (arg_my_alpha >= (SCORE_LOST + NODE_MAX)) and
      (arg_my_beta <= (SCORE_WON - NODE_MAX)))
  {
    int temp_score = return_my_score(object, &moves_list);

    if (temp_score <= (arg_my_alpha - SCORE_MAN / 2))
    {
      best_score = temp_score;
        
      *arg_best_pv = INVALID;

      *arg_best_depth = 0;

      goto label_return;
    }
  }
