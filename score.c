//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

#define MY_BIT      WHITE_BIT
#define my_colour   white
#define your_colour black

#include "score.d"

#undef MY_BIT
#undef my_colour
#undef your_colour

#define MY_BIT      BLACK_BIT
#define my_colour   black
#define your_colour white

#include "score.d"

#undef MY_BIT
#undef my_colour
#undef your_colour

