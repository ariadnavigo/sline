/* See LICENSE for copyright and license details. */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hist.h"
#include "strlcpy.h"
#include "sline.h"

static void hist_rotate(void);

char *hist[HIST_SIZE];
int hist_top, hist_pos;

static void
hist_rotate(void)
{
	int i;

	for (i = 1; i < HIST_SIZE; ++i)
		strlcpy(hist[i - 1], hist[i], sline_hist_entry_size);

	--hist_top;
}

void
hist_next(void)
{
	if (strlen(hist[hist_top]) == 0)
		return;

	++hist_top;
	if (hist_top >= HIST_SIZE)
		hist_rotate();
}

void
hist_set(int pos, const char *input)
{
	strlcpy(hist[pos], input, sline_hist_entry_size);
}

int
hist_setup(void)
{
	int i;

	for (i = 0; i < HIST_SIZE; ++i) {
		hist[i] = calloc(sline_hist_entry_size, sizeof(char));
		if (hist[i] == NULL) {
			sline_err = SLINE_ERR_MEMORY;
			return -1;
		}
	}

	return 0;
}
