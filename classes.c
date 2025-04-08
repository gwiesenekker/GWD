//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

//objects are derived from a class

//the class 'constructor' register_object registers a created object
//with the class so that the class_iterator can loop over all objects and
//it returns the object_id.
//object_id can be used to create logical names for derived properties such as
//log-0.txt for the log-file of the first thread object
//log-1.txt for the log-file of the second thread object
//etc.
//object_id's are not re-used and the class will keep track of
//at most nobject_max objects
//you should call the class constructor from the object constructor

local int register_object(class_t *self, void *object)
{
  HARDBUG(compat_mutex_lock(&(self->objects_mutex)) != 0)

  HARDBUG(self->nobjects >= self->nobjects_max)

  self->objects[self->nobjects++] = object;

  int object_id = self->object_id++;

  HARDBUG(compat_mutex_unlock(&(self->objects_mutex)) != 0)

  return(object_id);
}

//the class 'destructor' deregister_object deregisters an object
//in the class

local void deregister_object(class_t *self, void *object)
{
  HARDBUG(compat_mutex_lock(&(self->objects_mutex)) != 0)

  HARDBUG(self->nobjects < 1)

  int iobject;

  for (iobject = 0; iobject < self->nobjects; iobject++)
    if (self->objects[iobject] == object) break;

  HARDBUG(iobject >= self->nobjects) 

  for (int jobject = iobject; jobject < self->nobjects - 1; jobject++)
    self->objects[jobject] = self->objects[jobject + 1];

  self->objects[self->nobjects - 1] = NULL;

  self->nobjects--;

  HARDBUG(compat_mutex_unlock(&(self->objects_mutex)) != 0)
}

//you initialize a class by calling init_class
//ctor is a pointer to the constructor for objects of the class
//dtor is a pointer to the destructor for objects of the class
//iter is a is called when iterating over all objects of the class
//init_class registers the class constructor register_object
//and the class destructor deregister_object

class_t *init_class(int nobjects_max, ctor_t arg_ctor, dtor_t arg_dtor, iter_t arg_iter)
{
  class_t *self;
 
  MY_MALLOC(self, class_t, 1)

  //the class keeps track of the (number of) created objects

  self->nobjects_max = nobjects_max;

  self->nobjects = 0;

  self->object_id = 0;

  MY_MALLOC(self->objects, void *, nobjects_max)

  for (int iobject = 0; iobject < nobjects_max; iobject++)
   self->objects[iobject] = NULL;

  //register the class 'constructor' and 'destructor'

  self->register_object = register_object;

  self->deregister_object = deregister_object;

  //register the object constructor

  self->objects_ctor = arg_ctor;

  //register the object destructor

  self->objects_dtor = arg_dtor;

  //register the object iterator

  self->objects_iter = arg_iter;

  //make the class constructor thread safe

  HARDBUG(compat_mutex_init(&(self->objects_mutex)) != 0)

  return(self);
}

//the object iterator can return an error value

void iterate_class(class_t *self)
{
  HARDBUG(self->objects_iter == NULL)

  int nerrors = 0;

  for (int iobject = 0; iobject < self->nobjects; iobject++)
  {
    HARDBUG(self->objects[iobject] == NULL)

    nerrors += self->objects_iter(self->objects[iobject]);
  }
 
  HARDBUG(nerrors > 0)
}

//example

//objects are derived from the generic class object

local class_t *my_object_class;

//define an object with properties and methods
//object_id is set by the class constructor

typedef struct
{
  //generic properties and methods

  int object_id;

  char object_stamp[MY_LINE_MAX];

  //specific methods

  pter_t printf_object;
} my_object_t;

//the object printer

local void printf_my_object(void *self)
{
  my_object_t *object = self;

  PRINTF("printing my_object object_id=%d\n", object->object_id);

  PRINTF("object_stamp=%s\n", object->object_stamp);
}

//the object constructor

local void *construct_my_object(void)
{
  my_object_t *object;
  
  MY_MALLOC(object, my_object_t, 1)

  //call the class 'constructor'

  object->object_id = my_object_class->register_object(my_object_class, object);

  //construct other parts of the object

  time_t t = time(NULL);

  HARDBUG(strftime(object->object_stamp, MY_LINE_MAX,
                  "%H:%M:%S-%d/%m/%Y", localtime(&t)) == 0)

  //register object methods

  object->printf_object = printf_my_object;

  return(object);
}

//the object destructor

local void destroy_my_object(void *self)
{
  my_object_t *object = self;

  PRINTF("destroying my_object object_id=%d\n", object->object_id);

  //call the class 'destructor'

  my_object_class->deregister_object(my_object_class, object);
}

//the object iterator

local int iterate_my_object(void *self)
{
  my_object_t *object = self;

  PRINTF("iterate object_id=%d\n", object->object_id);

  return(0);
}

void test_my_object_class(void)
{
  //initialize the class 

  my_object_class = init_class(3, construct_my_object, destroy_my_object,
                          iterate_my_object);

  my_object_t *a = my_object_class->objects_ctor();

  a->printf_object(a);

  my_object_t *b = my_object_class->objects_ctor();

  b->printf_object(b);

  my_object_t *c = my_object_class->objects_ctor();

  c->printf_object(c);

  PRINTF("iterate from a to c\n");

  iterate_class(my_object_class);

  my_object_class->objects_dtor(a);

  PRINTF("a has been destroyed, b and c should be left\n");

  iterate_class(my_object_class);

  my_object_t *d = my_object_class->objects_ctor();

  d->printf_object(d);
  
  PRINTF("d has been added, iterate from b to d\n");

  iterate_class(my_object_class);

  my_object_class->objects_dtor(c);

  PRINTF("c has been destroyed, b and d should be left\n");

  iterate_class(my_object_class);

  my_object_t *e = my_object_class->objects_ctor();
  
  e->printf_object(e);

  PRINTF("e has been added\n");

  iterate_class(my_object_class);
}

