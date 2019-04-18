uint8_t *load_bin(FILE *fp, size_t *size_p);
node_t *read_bin(FILE *fp);
node_t *read_bin_size(FILE *fp, size_t *size_p);
size_t write_bin(FILE *fp, node_t *nodes);

uint8_t *load_dm2_bin(FILE *fp, size_t *size_p);
node_t *read_dm2_bin(FILE *fp);
size_t write_dm2_bin(FILE *fp, node_t *nodes);

node_t *read_bin_any(FILE *fp);
