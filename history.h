/* See LICENSE for copyright and license details. */

#define HISTORY_SIZE 50

void history_next(void);
void history_rotate(void);
void history_set(int pos, const char *input);
int history_setup(void);

extern char *history[HISTORY_SIZE];
extern int hist_top, hist_pos;
