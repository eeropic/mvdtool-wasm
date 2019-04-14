uint8_t *load_bin(FILE *fp, size_t *size_p);
node_t *read_bin(FILE *fp);
node_t *read_bin_size(FILE *fp, size_t *size_p);
size_t write_bin(FILE *fp, node_t *nodes);
