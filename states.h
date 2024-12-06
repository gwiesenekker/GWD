//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef StatesH
#define StatesH

typedef struct
{
  cJSON *cjson_object;

  char string[MY_LINE_MAX];

  void (*printf_state)(void  *);
  void (*set_state)(void  *, char *);
  void (*set_event)(void  *, char *);
  void (*set_date)(void  *, char *);
  void (*set_white)(void  *, char *);
  void (*set_black)(void  *, char *);
  void (*set_result)(void  *, char *);
  void (*set_starting_position)(void  *, char *);
  void (*push_move)(void  *, char *, char *);
  int (*pop_move)(void  *);
  void (*set_depth)(void  *, int);
  void (*set_time)(void  *, int);
  void (*save)(void  *, char *);
  void (*save2pdn)(void  *, char *);

  char *(*get_state)(void  *);
  char *(*get_event)(void  *);
  char *(*get_date)(void  *);
  char *(*get_white)(void  *);
  char *(*get_black)(void  *);
  char *(*get_result)(void  *);
  char *(*get_starting_position)(void  *);
  cJSON *(*get_moves)(void  *);
  int (*get_depth)(void  *);
  int (*get_time)(void  *);
  void (*load)(void  *, char *);
} state_t;

void construct_state(void *);
void destroy_state(void *);
void test_states(void);

#endif

