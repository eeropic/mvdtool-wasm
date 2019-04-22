#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define MVDTOOL_VERSION "0.3"

typedef enum { false, true } bool;

void fatal(const char *fmt, ...) __attribute__((noreturn));
void read_raw(void *buf, size_t len, FILE *fp);
void read_raw_safe(void *buf, size_t len, FILE *fp);
void write_raw(void *buf, size_t len, FILE *fp);
bool parse_timespec(const char *s, unsigned *blocknum);

#define MODE_DM2    1
#define MODE_WRITE  2
#define MODE_TXT    4
#define MODE_EOF    8

struct node_s;
typedef struct node_s node_t;

typedef struct {
    FILE *fp;
    char *path;
    unsigned mode;
    unsigned blocknum;
} demo_t;

demo_t *open_demo(const char *path, const char *mode);
void close_demo(demo_t *demo);
node_t *read_demo(demo_t *demo);
size_t write_demo(demo_t *demo, node_t *n);

int bin2txt_main(void);
int txt2bin_main(void);
int hash_main(void);
int strings_main(void);
int split_main(void);
int cut_main(void);
int diff_main(void);
