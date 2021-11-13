/* See LICENSE for copyright and license details. */

#include <stdio.h>
#include <string.h>

#include "sline.h"

#define BUF_SIZE 64
#define HISTORY_SIZE 64
#define INIT_STR NULL /* Set to a literal string to test init strings */

static void
print_hist(void)
{
	int i;
	const char *entry;

	printf("History contains:\n");
	
	for (i = 0; (entry = sline_history_get(i)) != NULL; ++i) {
		if (strlen(entry) > 0) /* Avoid printing "current blank" */
			printf("%s\n", entry);
	}
}

int
main(void)
{
	char buf[BUF_SIZE];
	int cnt, sline_stat;

	printf("sline_test: compiled with sline %s.\n", sline_version());
	printf("Type 'exit', 'quit', or Ctrl-D to exit.\n");

	if (sline_setup(HISTORY_SIZE) < 0) {
		fprintf(stderr, "sline: %s\n", sline_errmsg());
		return -1;
	}

	cnt = 0;
	while (feof(stdin) == 0) {
		/* Comment line below to use the default prompt. */
		sline_set_prompt("%d> ", cnt++);

		if ((sline_stat = sline(buf, BUF_SIZE, INIT_STR)) < 0)
			goto exit;

		if (strncmp(buf, "hist", BUF_SIZE) == 0) {
			print_hist();
			continue;
		} else if (strncmp(buf, "exit", BUF_SIZE) == 0
		         || strncmp(buf, "quit", BUF_SIZE) == 0) {
			break;
		}

		printf("Input was: %s\n", buf);
	}

exit:
	if (sline_stat < 0 && sline_err != SLINE_ERR_EOF)
		fprintf(stderr, "sline: %s.\n", sline_errmsg());
	sline_end();

	return 0;
}
