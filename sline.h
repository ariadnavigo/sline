/* See LICENSE for copyright and license details. */

int sline_setup(int entry_size);
void sline_end(void);
const char *sline_errmsg(void);
int sline(char *buf, size_t size);
