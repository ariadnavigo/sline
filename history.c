/* See LICENSE for copyright and license details. */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"
#include "strlcpy.h"
#include "sline.h"

char *history[HISTORY_SIZE];
int hist_top, hist_pos;

void
history_next(void)
{
	if (strlen(history[hist_top]) == 0)
		return;

	++hist_top;
	if (hist_top >= HISTORY_SIZE)
		history_rotate();
}

void
history_rotate(void)
{
	int i;

	for (i = 1; i < HISTORY_SIZE; ++i)
		strlcpy(history[i - 1], history[i], sline_hist_entry_size);

	--hist_top;
}

void
history_set(int pos, const char *input)
{
	strlcpy(history[pos], input, sline_hist_entry_size);
}

int
history_setup(void)
{
	int i;

	for (i = 0; i < HISTORY_SIZE; ++i) {
		history[i] = calloc(sline_hist_entry_size, sizeof(char));
		if (history[i] == NULL) {
			sline_err = SLINE_ERR_MEMORY;
			return -1;
		}
	}
	
	return 0;
}
