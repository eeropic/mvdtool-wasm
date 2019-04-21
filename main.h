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

int bin2txt_main(void);
int txt2bin_main(void);
int hash_main(void);
int strings_main(void);
int split_main(void);
int cut_main(void);
int diff_main(void);
