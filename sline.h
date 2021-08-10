/* See LICENSE for copyright and license details. */

/* sline_err values */
enum {
	SLINE_ERR_DEF,
	SLINE_ERR_EOF,
	SLINE_ERR_IO,
	SLINE_ERR_MEMORY,
	SLINE_ERR_TERMIOS_GET,
	SLINE_ERR_TERMIOS_SET
};

int sline(char *buf, int size, const char *init);
void sline_end(void);
const char *sline_errmsg(void);
int sline_setup(int entry_size);
void sline_set_prompt(const char *fmt, ...);

extern int sline_err;
