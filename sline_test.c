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
	int cnt;

	if (sline_setup(HISTORY_SIZE) < 0) {
		fprintf(stderr, "sline: %s", sline_errmsg());
		return -1;
	}

	cnt = 0;
	while (feof(stdin) == 0) {
		/* Comment line below to use the default prompt. */
		sline_set_prompt("%d> ", cnt++);

		if (sline(buf, BUF_SIZE) < 0)
			goto exit;

		printf("Input was: %s\n", buf);
	}

exit:
	sline_end();

	return 0;
}
