// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#include "classes.h"

#include "bstrlib.h"
#include "common.h"
#include "my_bstreams.h"
#include "my_malloc.h"
#include "my_printf.h"

// objects are derived from a class

// the class 'constructor' register_object registers a created object
// with the class so that the class_iterator can loop over all objects and
// it returns the object_id.
// object_id can be used to create logical names for derived properties such as
// log-0.txt for the log-file of the first thread object
// log-1.txt for the log-file of the second thread object
// etc.
// object_id's are not re-used and the class will keep track of
// at most nobject_max objects
// you should call the class constructor from the object constructor

local int register_object(class_t *arg_self, void *arg_object)
{
  HARDBUG(compat_mutex_lock(&(arg_self->objects_mutex)) != 0)

  HARDBUG(arg_self->nobjects >= arg_self->nobjects_max)

  arg_self->objects[arg_self->nobjects++] = arg_object;

  int object_id = arg_self->object_id++;

  HARDBUG(compat_mutex_unlock(&(arg_self->objects_mutex)) != 0)

  return (object_id);
}

// the class 'destructor' deregister_object deregisters an object
// in the class

local void deregister_object(class_t *arg_self, void *arg_object)
{
  HARDBUG(compat_mutex_lock(&(arg_self->objects_mutex)) != 0)

  HARDBUG(arg_self->nobjects < 1)

  int iobject;

  for (iobject = 0; iobject < arg_self->nobjects; iobject++)
    if (arg_self->objects[iobject] == arg_object)
      break;

  HARDBUG(iobject >= arg_self->nobjects)

  for (int jobject = iobject; jobject < arg_self->nobjects - 1; jobject++)
    arg_self->objects[jobject] = arg_self->objects[jobject + 1];

  arg_self->objects[arg_self->nobjects - 1] = NULL;

  arg_self->nobjects--;

  HARDBUG(compat_mutex_unlock(&(arg_self->objects_mutex)) != 0)
}

// you initialize a class by calling init_class
// ctor is a pointer to the constructor for objects of the class
// dtor is a pointer to the destructor for objects of the class
// iter is a is called when iterating over all objects of the class
// init_class registers the class constructor register_object
// and the class destructor deregister_object

class_t *init_class(int arg_nobjects_max,
                    ctor_t arg_ctor,
                    dtor_t arg_dtor,
                    iter_t arg_iter)
{
  class_t *self;

  MY_MALLOC_BY_TYPE(self, class_t, 1)

  // the class keeps track of the (number of) created objects

  self->nobjects_max = arg_nobjects_max;

  self->nobjects = 0;

  self->object_id = 0;

  MY_MALLOC_BY_TYPE(self->objects, void *, arg_nobjects_max)

  for (int iobject = 0; iobject < arg_nobjects_max; iobject++)
    self->objects[iobject] = NULL;

  // register the class 'constructor' and 'destructor'

  self->register_object = register_object;

  self->deregister_object = deregister_object;

  // register the object constructor

  self->objects_ctor = arg_ctor;

  // register the object destructor

  self->objects_dtor = arg_dtor;

  // register the object iterator

  self->objects_iter = arg_iter;

  // make the class constructor thread safe

  HARDBUG(compat_mutex_init(&(self->objects_mutex)) != 0)

  return (self);
}

// the object iterator can return an error value

void iterate_class(class_t *arg_self)
{
  HARDBUG(arg_self->objects_iter == NULL)

  int nerrors = 0;

  for (int iobject = 0; iobject < arg_self->nobjects; iobject++)
  {
    HARDBUG(arg_self->objects[iobject] == NULL)

    nerrors += arg_self->objects_iter(arg_self->objects[iobject]);
  }

  HARDBUG(nerrors > 0)
}

// example

// objects are derived from the generic class object

local class_t *my_object_class;

// define an object with properties and methods
// object_id is set by the class constructor

typedef struct
{
  // generic properties and methods

  int object_id;

  bstring object_stamp;

  // specific methods

  pter_t printf_object;
} my_object_t;

// the object printer

local void printf_my_object(void *arg_self)
{
  my_object_t *object = arg_self;

  PRINTF("printing my_object object_id=%d\n", object->object_id);

  PRINTF("object_stamp=%s\n", bdata(object->object_stamp));
}

// the object constructor

local void *construct_my_object(void)
{
  my_object_t *object;

  MY_MALLOC_BY_TYPE(object, my_object_t, 1)

  // call the class 'constructor'

  object->object_id = my_object_class->register_object(my_object_class, object);

  // construct other parts of the object

  time_t current_time = time(NULL);

  HARDBUG((object->object_stamp = bfromcstr("")) == NULL)

  my_bstrftime(object->object_stamp, "%H:%M:%S-%d/%m/%Y", current_time);

  // register object methods

  object->printf_object = printf_my_object;

  return (object);
}

// the object destructor

local void destroy_my_object(void *arg_self)
{
  my_object_t *object = arg_self;

  PRINTF("destroying my_object object_id=%d\n", object->object_id);

  HARDBUG(bdestroy(object->object_stamp) == BSTR_ERR)

  // call the class 'destructor'

  my_object_class->deregister_object(my_object_class, object);
}

// the object iterator

local int iterate_my_object(void *arg_self)
{
  my_object_t *object = arg_self;

  PRINTF("iterate object_id=%d\n", object->object_id);

  return (0);
}

void test_my_object_class(void)
{
  // initialize the class

  my_object_class =
    init_class(3, construct_my_object, destroy_my_object, iterate_my_object);

  my_object_t *object_a = my_object_class->objects_ctor();

  object_a->printf_object(object_a);

  my_object_t *object_b = my_object_class->objects_ctor();

  object_b->printf_object(object_b);

  my_object_t *object_c = my_object_class->objects_ctor();

  object_c->printf_object(object_c);

  PRINTF("iterate from a to c\n");

  iterate_class(my_object_class);

  my_object_class->objects_dtor(object_a);

  PRINTF("a has been destroyed, b and c should be left\n");

  iterate_class(my_object_class);

  my_object_t *object_d = my_object_class->objects_ctor();

  object_d->printf_object(object_d);

  PRINTF("d has been added, iterate from b to d\n");

  iterate_class(my_object_class);

  my_object_class->objects_dtor(object_c);

  PRINTF("c has been destroyed, b and d should be left\n");

  iterate_class(my_object_class);

  my_object_t *object_e = my_object_class->objects_ctor();

  object_e->printf_object(object_e);

  PRINTF("e has been added\n");

  iterate_class(my_object_class);
}
