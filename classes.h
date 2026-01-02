//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef ClassesH
#define ClassesH

//class methods

typedef void *(*ctor_t)(void);

typedef void (*dtor_t)(void *);

typedef int (*iter_t)(void *);

//object methods

typedef void (*pter_t)(void *);

typedef struct class
{ 
  //objects are created by calling
  //class_objects->objects_ctor().
  //objects_ctor() should call register_object() to register an object
  //within the class
  //objects are destroyed by calling
  //class_objects->objects_dtor()
  //objects_dtor() should call deregister_object() to deregister an object
  //from the class
  
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

class_t *init_class(int, ctor_t, dtor_t, iter_t);

void iterate_class(class_t *);

void test_my_object_class(void);

#endif

