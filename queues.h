//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#ifndef QueuesH
#define QueuesH

#define NMESSAGES_MAX   1024

#define MESSAGE_SOLVE          1
#define MESSAGE_EXIT_THREAD    2 
#define MESSAGE_ABORT_SEARCH   3
#define MESSAGE_READY          4
#define MESSAGE_INFO           5
#define MESSAGE_FEN            6
#define MESSAGE_STATE          7
#define MESSAGE_BOARD          8
#define MESSAGE_MOVE           9
#define MESSAGE_GO            10
#define MESSAGE_RESULT        11
#define MESSAGE_LAST          12

#define MESSAGE_SEARCH_FIRST  15
#define MESSAGE_SEARCH_AHEAD  16
#define MESSAGE_SEARCH_SECOND 17

typedef struct
{
  int message_id;

  bstring message_text;

  int message_read;
} message_t;

typedef struct
{
  bstring Q_name;

  my_printf_t *Q_my_printf;

  message_t Q_messages[NMESSAGES_MAX];

  int Q_nread;
  int Q_nwrite;
  int Q_length_max;

  my_mutex_t Q_mutex;
} queue_t;

void construct_queue(void *, char *, my_printf_t *);
void enqueue(void *, int, char *);
int poll_queue(void *);
int dequeue(void *, message_t *);

void test_queues(void);

#endif

