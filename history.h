/* See LICENSE for copyright and license details. */

#define HISTORY_SIZE 50

const char *history_get(int pos);
void history_next(void);
void history_rotate(void);
void history_set(int pos, const char *input);

extern char *history[HISTORY_SIZE];
extern int hist_top, hist_pos;
extern size_t hist_entry_size;
