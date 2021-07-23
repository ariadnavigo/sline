/* See LICENSE for copyright and license details. */

#include <stddef.h>
#include <stdio.h>

#include "sline.h"

#define BUF_SIZE 64
#define HISTORY_SIZE 64

int
main(void)
{
	char buf[BUF_SIZE];

	if (sline_setup(HISTORY_SIZE) < 0) {
		fprintf(stderr, "sline: %s", sline_errmsg());
		return -1;
	}

	while (feof(stdin) == 0) {
		printf("> ");
		fflush(stdout);
		if (sline(buf, BUF_SIZE) < 0)
			goto exit;

		printf("Input was: %s\n", buf);
	}

exit:
	sline_end();

	return 0;
}
