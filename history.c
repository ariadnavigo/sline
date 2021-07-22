/* See LICENSE for copyright and license details. */

#include <string.h>

#include "history.h"
#include "strlcpy.h"

#define HISTORY_SIZE 50

char *history[HISTORY_SIZE];
int hist_curr, hist_pos;
size_t hist_entry_size;

const char *
history_get(int pos)
{
	if (pos < 0 || pos > hist_curr)
		return NULL;

	return history[pos];
}

void
history_next(void)
{
	if (strlen(history[hist_curr]) == 0)
		return;

	++hist_curr;
	if (hist_curr >= HISTORY_SIZE)
		history_rotate();
}

void
history_rotate(void)
{
	int i;

	for (i = 1; i < HISTORY_SIZE; ++i)
		strlcpy(history[i - 1], history[i], hist_entry_size);

	--hist_curr;
}

void
history_set(int pos, const char *input)
{
	/* Ignoring blank lines */
	if (strlen(input) == 0)
		return;

	strlcpy(history[pos], input, hist_entry_size);
}

