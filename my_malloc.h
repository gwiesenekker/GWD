//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef MyMallocH
#define MyMallocH

#define MARK_ARRAY_READONLY(A)\
  {\
    readonly_object_t *ro = readonly_class->objects_ctor();\
    ro->mark_readonly(ro, #A, A, sizeof(A));\
  }

typedef struct
{
  int object_id;

  pter_t printf_object;

  iter_t iterate_object;

  char readonly_object_name[MY_LINE_MAX];

  void *readonly_object_pointer;
  size_t readonly_object_size;
  ui64_t readonly_object_crc64;

  void (*mark_readonly)(void *, char *, void *, size_t);
} readonly_object_t;

extern class_t *readonly_class;

void init_my_malloc(void);
void check_readonly_objects(void);

#endif

