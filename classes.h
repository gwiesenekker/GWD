// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef ClassesH
#define ClassesH

#include "compat.h"

// class methods

typedef void *(*ctor_t)(void);

typedef void (*dtor_t)(void *);

typedef int (*iter_t)(void *);

// object methods

typedef void (*pter_t)(void *);

typedef struct class
{
  // objects are created by calling
  // class_objects->objects_ctor().
  // objects_ctor() should call register_object() to register an object
  // within the class
  // objects are destroyed by calling
  // class_objects->objects_dtor()
  // objects_dtor() should call deregister_object() to deregister an object
  // from the class

  int (*register_object)(struct class *, void *);
  void (*deregister_object)(struct class *, void *);

  int nobjects_max;
  int nobjects;
  int object_id;

  void **objects;

  ctor_t objects_ctor;
  dtor_t objects_dtor;
  iter_t objects_iter;

  my_mutex_t objects_mutex;
} class_t;

class_t *init_class(int arg_nobjects_max,
                    ctor_t arg_ctor,
                    dtor_t arg_dtor,
                    iter_t arg_iter);

void iterate_class(class_t *arg_self);

void test_my_object_class(void);

#endif
