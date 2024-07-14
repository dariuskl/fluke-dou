#define nullptr ((void *)0)

typedef unsigned char byte;

struct s {
  const char *ptr;
  int length;
};

struct sbuf {
  char *ptr;
  int length;
  int capacity;
};

void print_cstr(struct sbuf *buf, const char *str);
