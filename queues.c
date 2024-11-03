//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#include "globals.h"

void construct_queue(void *self,
  char *arg_queue_name, my_printf_t *arg_queue_my_printf)
{
  queue_t *object = self;

  HARDBUG((object->Q_name = bfromcstr(arg_queue_name)) == NULL)

  object->Q_my_printf = arg_queue_my_printf;

  for (int imessage = 0; imessage < NMESSAGES_MAX; imessage++)
    HARDBUG((object->Q_messages[imessage].message_text = bfromcstr("")) ==
            NULL)

  object->Q_nread = 0;
  object->Q_nwrite = -1;
  object->Q_length_max = 0;

  HARDBUG(compat_mutex_init(&(object->Q_mutex)) != 0)
}

void enqueue(void *self, int arg_message_id, char *arg_message_text)
{
  queue_t *object = self;
  
  HARDBUG(compat_mutex_lock(&(object->Q_mutex)) != 0)

  //nread, nwrite
  //0      -1     : 0 messages in queue
  //0      0      : 1 message in queue
  //0      1      : 2 messages in queue
  //number of messages = (nwrite - nread + 1)

  if ((object->Q_nwrite - object->Q_nread + 1) >= NMESSAGES_MAX)
  {
    my_printf(object->Q_my_printf, "Q_name=%s", bdata(object->Q_name));

    FATAL("NMESSAGE_MAX", EXIT_FAILURE)
  }

  object->Q_nwrite++;

  message_t *message =
    &(object->Q_messages[object->Q_nwrite % NMESSAGES_MAX]);

  message->message_id = arg_message_id;

  HARDBUG(bassigncstr(message->message_text, arg_message_text) != BSTR_OK)

  message->message_read = FALSE;

  int length = object->Q_nwrite - object->Q_nread + 1;

  if (length > object->Q_length_max)
  {
#ifdef DEBUG
    my_printf(object->Q_my_printf,
      "length queue %s after enqueue is %d message_id=%d message=%s\n",
      bdata(object->Q_name), length, arg_message_id, arg_message_text);
#endif
    object->Q_length_max = length;
  }

  HARDBUG(compat_mutex_unlock(&(object->Q_mutex)) != 0)
}

int poll_queue(void *self)
{
  queue_t *object = self;

  int result = INVALID;

  if (object == NULL) return(result);

  HARDBUG(compat_mutex_lock(&(object->Q_mutex)) != 0)

  if (object->Q_nread <= object->Q_nwrite)
    result = object->Q_messages[object->Q_nread % NMESSAGES_MAX].message_id;

  HARDBUG(compat_mutex_unlock(&(object->Q_mutex)) != 0)

  return(result);
}

int dequeue(void *self, message_t *arg_message)
{
  queue_t *object = self;

  int result = INVALID;
  
  HARDBUG(compat_mutex_lock(&(object->Q_mutex)) != 0)

  if (object->Q_nread <= object->Q_nwrite)
  {
    message_t *message = 
      &(object->Q_messages[object->Q_nread % NMESSAGES_MAX]);

    message->message_read = TRUE;

    object->Q_nread++;

    result = message->message_id;

    if (arg_message != NULL) *arg_message = *message;
  }

  HARDBUG(compat_mutex_unlock(&(object->Q_mutex)) != 0)

  return(result);
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

        snprintf(text, MY_LINE_MAX, "message %d from %d",
          imessage, itest);

        enqueue(test + jtest, itest, text);
      }
    }
  }

  for (int itest = 0; itest < NTEST; itest++)
  {
    message_t message;

    while(dequeue(test + itest, &message) != INVALID)
    {
      PRINTF("%d received %d '%s'\n",
        itest, message.message_id, bdata(message.message_text));
    }

    //test WARNING

    char text[MY_LINE_MAX];

    snprintf(text, MY_LINE_MAX, "warning %d", itest);

    enqueue(test + itest, itest, text);
  }
}
