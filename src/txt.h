#define TXT_MAGIC           mkle32('T','X','T','2')

node_t *read_txt( FILE *fp );
void write_txt( FILE *fp, node_t *nodes, unsigned blocknum );
