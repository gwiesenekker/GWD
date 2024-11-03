//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
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

