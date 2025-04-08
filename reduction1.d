//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
        if (allow_reductions and
            !(MOVE_DO_NOT_REDUCE(moves_list.ML_move_flags[jmove])) and
            (best_score >= (SCORE_LOST + NODE_MAX)) and
            (npassed >= npassed_min))
        {
          for (int ireduce = 0; ireduce <= 1; ireduce++)
          {
            int reduced_depth = roundf((1.0f - factor) * your_depth) + ireduce;

            if (reduced_depth >= your_depth) break; 

            if (ireduce == 0) pass[jmove] = 1;

            temp_score = -your_alpha_beta(object,
              -arg_my_beta, -arg_my_alpha, reduced_depth,
              arg_node_type,
              arg_reduction_depth_root,
              your_tweaks | TWEAK_PREVIOUS_MOVE_REDUCED_BIT,
              temp_pv + 1, &temp_depth);
  
            if (object->S_interrupt != 0)
            {
              undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
      
              best_score = SCORE_PLUS_INFINITY;
                    
              goto label_return;
            }

            if (temp_score > best_score)
            {
              pass[jmove] = 0;

              break;
            }
          }
        }//if (!allow_reduction   
