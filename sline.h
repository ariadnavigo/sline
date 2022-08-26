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
const char *sline_history_get(int pos);
int sline_setup(void);
void sline_set_prompt(const char *fmt, ...);
const char *sline_version(void);

extern int sline_history, sline_err;
extern size_t sline_hist_entry_size;
