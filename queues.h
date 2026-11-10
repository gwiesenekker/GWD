// SCU REVISION 8.1263 di 10 nov 2026 16:08:19 CET
#ifndef QueuesH
#define QueuesH

#define NMESSAGES_MAX 1024

#define MESSAGE_SOLVE 1
#define MESSAGE_EXIT_THREAD 2
#define MESSAGE_ABORT_SEARCH 3
#define MESSAGE_READY 4
#define MESSAGE_INFO 5
#define MESSAGE_FEN 6
#define MESSAGE_STATE 7
#define MESSAGE_GO 8
#define MESSAGE_MCTS 9
#define MESSAGE_RESULT 10

#define MESSAGE_SEARCH_FIRST 11
#define MESSAGE_SEARCH_AHEAD 12
#define MESSAGE_SEARCH_SECOND 13

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
  my_random_t Q_my_random;
} queue_t;

void construct_queue(queue_t *, char *, my_printf_t *);
void enqueue(queue_t *, int, char *);
int poll_queue(queue_t *);
int dequeue(queue_t *, message_t *);

void test_queues(void);

#endif
