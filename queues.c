// SCU REVISION 8.12464 vr  4 sep 2026  8:14:53 CEST
#include "globals.h"

void construct_queue(queue_t *arg_with_queue,
                     char *arg_queue_name,
                     my_printf_t *arg_queue_my_printf)
{
  HARDBUG((arg_with_queue->Q_name = bfromcstr(arg_queue_name)) == NULL)

  arg_with_queue->Q_my_printf = arg_queue_my_printf;

  for (int imessage = 0; imessage < NMESSAGES_MAX; imessage++)
    HARDBUG((arg_with_queue->Q_messages[imessage].message_text =
               bfromcstr("")) == NULL)

  arg_with_queue->Q_nread = 0;
  arg_with_queue->Q_nwrite = -1;
  arg_with_queue->Q_length_max = 0;

  HARDBUG(compat_mutex_init(&(arg_with_queue->Q_mutex)) != 0)

  ui64_t seed;

  construct_my_random(&(arg_with_queue->Q_my_random), &seed);

  PRINTF("construct_queue arg_queue_name=%s"
         " seed=%llX\n",
         arg_queue_name,
         seed);
}

void enqueue(queue_t *arg_with_queue,
             int arg_message_id,
             char *arg_message_text)
{
  progress_t progress;

  construct_progress(&progress, "enqueue", 0, 10);

  while (TRUE)
  {
    (void)make_progress(&progress, 0);

    HARDBUG(compat_mutex_lock(&(arg_with_queue->Q_mutex)) != 0)

    // nread, nwrite
    // 0      -1     : 0 messages in queue
    // 0      0      : 1 message in queue
    // 0      1      : 2 messages in queue
    // number of messages = (nwrite - nread + 1)

    if ((arg_with_queue->Q_nwrite - arg_with_queue->Q_nread + 1) <
        NMESSAGES_MAX)
      break;

    double delay = delay =
      (1.0 + return_my_random(&(arg_with_queue->Q_my_random)) % 1000) *
      MICRO_SECOND;

    if ((return_ndone(&progress) % 1000) == 0)
    {
      PRINTF("queue full arg_message_text=%s"
             " return_ndone(&progress)=%lld"
             " delay=%.6f\n",
             arg_message_text,
             return_ndone(&progress),
             delay);
    }

    compat_sleep(delay);

    HARDBUG(compat_mutex_unlock(&(arg_with_queue->Q_mutex)) != 0)
  }

  arg_with_queue->Q_nwrite++;

  message_t *message =
    &(arg_with_queue->Q_messages[arg_with_queue->Q_nwrite % NMESSAGES_MAX]);

  message->message_id = arg_message_id;

  HARDBUG(bassigncstr(message->message_text, arg_message_text) == BSTR_ERR)

  message->message_read = FALSE;

  int length = arg_with_queue->Q_nwrite - arg_with_queue->Q_nread + 1;

  if (length > arg_with_queue->Q_length_max)
  {
#ifdef DEBUG
    my_printf(arg_with_queue->Q_my_printf,
              "length queue %s after enqueue is %d message_id=%d message=%s\n",
              bdata(arg_with_queue->Q_name),
              length,
              arg_message_id,
              arg_message_text);
#endif
    arg_with_queue->Q_length_max = length;
  }

  HARDBUG(compat_mutex_unlock(&(arg_with_queue->Q_mutex)) != 0)
}

int poll_queue(queue_t *arg_with_queue)
{
  int result = INVALID;

  if (arg_with_queue == NULL)
    return (result);

  HARDBUG(compat_mutex_lock(&(arg_with_queue->Q_mutex)) != 0)

  if (arg_with_queue->Q_nread <= arg_with_queue->Q_nwrite)
    result = arg_with_queue->Q_messages[arg_with_queue->Q_nread % NMESSAGES_MAX]
               .message_id;

  HARDBUG(compat_mutex_unlock(&(arg_with_queue->Q_mutex)) != 0)

  return (result);
}

int dequeue(queue_t *arg_with_queue, message_t *arg_message)
{
  int result = INVALID;

  HARDBUG(compat_mutex_lock(&(arg_with_queue->Q_mutex)) != 0)

  if (arg_with_queue->Q_nread <= arg_with_queue->Q_nwrite)
  {
    message_t *message =
      &(arg_with_queue->Q_messages[arg_with_queue->Q_nread % NMESSAGES_MAX]);

    message->message_read = TRUE;

    arg_with_queue->Q_nread++;

    result = message->message_id;

    if (arg_message != NULL)
      *arg_message = *message;
  }

  HARDBUG(compat_mutex_unlock(&(arg_with_queue->Q_mutex)) != 0)

  return (result);
}

#define NTEST 4

void test_queues(void)
{
  queue_t test[NTEST];

  for (int itest = 0; itest < NTEST; itest++)
  {
    char name[MY_LINE_MAX];

    snprintf(name, MY_LINE_MAX, "test%d", itest);

    construct_queue(test + itest, name, STDOUT);
  }

  for (int itest = 0; itest < NTEST; itest++)
  {
    for (int jtest = 0; jtest < NTEST; jtest++)
    {
      for (int imessage = 0; imessage < 16; imessage++)
      {
        char text[MY_LINE_MAX];

        snprintf(text, MY_LINE_MAX, "message %d from %d", imessage, itest);

        enqueue(test + jtest, itest, text);
      }
    }
  }

  for (int itest = 0; itest < NTEST; itest++)
  {
    message_t message;

    while (dequeue(test + itest, &message) != INVALID)
    {
      PRINTF("%d received %d '%s'\n",
             itest,
             message.message_id,
             bdata(message.message_text));
    }

    // test WARNING

    char text[MY_LINE_MAX];

    snprintf(text, MY_LINE_MAX, "warning %d", itest);

    enqueue(test + itest, itest, text);
  }
}
