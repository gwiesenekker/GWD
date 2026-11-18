// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef QueuesH
#define QueuesH

enum
{
  NMESSAGES_MAX = 1024
};

enum
{
  MESSAGE_SOLVE = 1,
  MESSAGE_EXIT_THREAD = 2,
  MESSAGE_ABORT_SEARCH = 3,
  MESSAGE_READY = 4,
  MESSAGE_INFO = 5,
  MESSAGE_FEN = 6,
  MESSAGE_STATE = 7,
  MESSAGE_GO = 8,
  MESSAGE_MCTS = 9,
  MESSAGE_RESULT = 10
};

enum
{
  MESSAGE_SEARCH_FIRST = 11,
  MESSAGE_SEARCH_AHEAD = 12,
  MESSAGE_SEARCH_SECOND = 13,
  MESSAGE_GO_TIMED = 14,
  MESSAGE_GO_SHORT_CLOCK = 15
};

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
  int Q_nmessages_max;

  my_mutex_t Q_mutex;
  my_random_t Q_my_random;
} queue_t;

void construct_queue(queue_t *, const char *, my_printf_t *);
void enqueue(queue_t *, int, const char *);
int poll_queue(queue_t *);
int dequeue(queue_t *, message_t *);

void test_queues(void);

#endif
