/* See LICENSE for copyright and license details. */

#define HIST_SIZE 50

void hist_next(void);
void hist_set(int pos, const char *input);
int hist_setup(size_t entry_size);

extern char *hist[HIST_SIZE];
extern int hist_top, hist_pos;
