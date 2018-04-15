#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define MAX_PROC 64
#define PGSIZE 4096

struct ptr_struct {
  int busy; //whether in use or not
  void *ptr;
  void *stack;
};

struct ptr_struct ptrs[MAX_PROC];

static inline int fetch_and_add(int *var, int val) {   
    __asm__ volatile
    ("lock; xaddl %0, %1"
	: "+r" (val),  "+m" (*var) // input + output
	: // No input
	: "memory"
    );
    return val;
}

void lock_init(lock_t *lock)
{
  lock->ticket = 0;
  lock->turn = 0;
}

void lock_acquire(lock_t *lock)
{
  int turn = fetch_and_add(&lock->ticket, 1);
  while(lock->turn != turn) //spin lock
  {}
}

void lock_release(lock_t *lock)
{
  lock->turn = lock->turn + 1;
}

int thread_create(void (*start_routine)(void*, void*), void* arg1, void* arg2)
{
  void* freeptr = malloc(PGSIZE*2);
  void* stack;
  if(freeptr == 0)
    return -1;
  if((uint)freeptr % PGSIZE == 0)
    stack = freeptr;
  else
    stack = freeptr + (PGSIZE - ((uint)freeptr % PGSIZE));
  for(int i = 0; i < MAX_PROC; i++){
    if(ptrs[i].busy == 0){
      ptrs[i].ptr = freeptr;
      ptrs[i].stack = stack;
      ptrs[i].busy = 1;
      break;
    }
  }
  int ret = clone(start_routine, arg1, arg2, stack);
  return ret;
}

int thread_join()
{
  void* stack;
  int ret = join(&stack);
  for(int i = 0; i < MAX_PROC; i++){
    if(ptrs[i].busy == 1 && ptrs[i].stack == stack){
      free(ptrs[i].ptr);
      ptrs[i].stack = NULL;
      ptrs[i].busy = 0;
      ptrs[i].ptr = NULL;
      break;
    }
  }
  return ret;
}

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}
