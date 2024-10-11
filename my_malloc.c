//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

class_t *readonly_class;

local void printf_readonly_object(void *self)
{
  readonly_object_t *readonly_object = (readonly_object_t *) self;

  PRINTF("printing readonly_object\n");

  PRINTF("readonly_object_name=%s\n", readonly_object->readonly_object_name);
}

local void mark_readonly(void * self, 
  char *arg_name, void *arg_pointer, size_t arg_size)
{
  readonly_object_t *readonly_object = (readonly_object_t *) self;

  strncpy(readonly_object->readonly_object_name, arg_name, MY_LINE_MAX);
  readonly_object->readonly_object_pointer = arg_pointer;
  readonly_object->readonly_object_size = arg_size;

  readonly_object->readonly_object_crc64 =
    return_crc64(arg_pointer, arg_size, TRUE);
}

local void *construct_readonly_object(void)
{
  readonly_object_t *self;

  MALLOC(self, readonly_object_t, 1)

  self->object_id =
    readonly_class->register_object(readonly_class, self);

  self->printf_object = printf_readonly_object;

  self->mark_readonly = mark_readonly;

  return(self);
}

local int iterate_readonly_object(void *self)
{
  readonly_object_t *readonly_object = (readonly_object_t *) self;
 
  ui64_t crc64 = return_crc64(readonly_object->readonly_object_pointer,
                              readonly_object->readonly_object_size, TRUE);
 
  int error = 0;

  if (crc64 != readonly_object->readonly_object_crc64)
  {
    PRINTF("%s corrupted!\n", readonly_object->readonly_object_name);

    error = 1;
  }
  else
  {
    PRINTF("%s intact!\n", readonly_object->readonly_object_name);
  }

  return(error);
}

void init_my_malloc(void)
{
  readonly_class = init_class(1024, construct_readonly_object, NULL,
                                iterate_readonly_object);
}

void check_readonly_objects(void)
{
  iterate_class(readonly_class);
}

