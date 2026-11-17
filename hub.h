// SCU REVISION 8.1279 zo 15 nov 2026 16:49:05 CET
#ifndef HubH
#define HubH

extern double hub_search_time_left;

void init_hub(void);
void hub(void);
double search_clock_used(my_timer_t *);

#endif
