/* See LICENSE for copyright and license details. */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "hist.h"
#include "sline.h"
#include "strlcpy.h"
#include "vt100.h"

#define SLINE_HIST_ENTRY_DEF_SIZE 64 /* Default size for history entries */
#define SLINE_PROMPT_DEFAULT "> "
#define SLINE_PROMPT_SIZE 32

static void on_up(char *buf, size_t size);
static void on_down(char *buf, size_t size);
static void on_return(const char *buf);
static void on_delete(char *buf, size_t size, int bsmode);
static void on_char(char *buf, size_t size, const char *utf8);

static char sline_prompt[SLINE_PROMPT_SIZE];
static struct termios old, term;

int sline_history = 1; /* History feature on by default */
int sline_err = SLINE_ERR_DEF;
size_t sline_hist_entry_size = SLINE_HIST_ENTRY_DEF_SIZE;

/* Auxiliary subroutines */

static void
on_up(char *buf, size_t size)
{
	const char *hist;

	if (sline_history == 0)
		return;

	if (hist_pos > 0)
		--hist_pos;

	if ((hist = sline_history_get(hist_pos)) == NULL)
		return;

	vt100_ln_write(buf, size, hist);
}

static void
on_down(char *buf, size_t size)
{
	const char *hist;

	if (sline_history == 0)
		return;

	if (hist_pos < hist_top)
		++hist_pos;
	else
		hist_pos = hist_top;

	if ((hist = sline_history_get(hist_pos)) == NULL)
		return;

	vt100_ln_write(buf, size, hist);
}

static void
on_return(const char *buf)
{
	write(STDOUT_FILENO, "\n", 1);
	if (sline_history > 0) {
		hist_set(hist_pos, buf);
		hist_next();
	}
}

static void
on_delete(char *buf, size_t size, int bsmode)
{
	vt100_utf8_delete(buf, size, bsmode);

	if (sline_history > 0) {
		hist_set(hist_top, buf);
		hist_pos = hist_top;
	}
}

static void
on_char(char *buf, size_t size, const char *utf8)
{
	vt100_utf8_insert(buf, size, utf8);

	if (sline_history > 0 && strlen(buf) > 0) {
		hist_set(hist_top, buf);
		hist_pos = hist_top;
	}
}

/* Public sline API subroutines follow */

int
sline(char *buf, int size, const char *init)
{
	char utf8[UTF8_BYTES];
	int key;
	size_t wsize;

	write(STDOUT_FILENO, sline_prompt, SLINE_PROMPT_SIZE);

	/*
	 * We're always writing one less, so together with the memset() call
	 * below, using wsize will guarantee the last character in buf to
	 * always be '\0'.
	 */
	wsize = size - 1;
	memset(buf, 0, size);
	if (init != NULL) {
		/*
		 * Using size instead of wsize because we're already given a
		 * null terminated string.
		 */
		vt100_ln_write(buf, size, init);
	}

	memset(utf8, 0, UTF8_BYTES);
	hist_pos = hist_top;
	while ((key = vt100_read_key(utf8)) != -1) {
		switch (key) {
		case VT_BKSPC:
			on_delete(buf, size, 1);
			break;
		case VT_DLT:
			on_delete(buf, size, 0);
			break;
		case VT_EOF:
			write(STDOUT_FILENO, "\n", 1);
			sline_err = SLINE_ERR_EOF;
			return -1;
		case VT_RET:
			on_return(buf);
			return 0;
		case VT_UP:
			on_up(buf, wsize);
			break;
		case VT_DWN:
			on_down(buf, wsize);
			break;
		case VT_LFT:
			vt100_cur_mov_left(buf);
			break;
		case VT_RGHT:
			vt100_cur_mov_right(buf);
			break;
		case VT_HOME:
			vt100_cur_mov_home();
			break;
		case VT_END:
			vt100_cur_mov_end(buf);
			break;
		case VT_CHR:
			on_char(buf, wsize, utf8);
			break;
		default:
			/* Silently ignore everything that isn't caught. */
			break;
		}

		memset(utf8, 0, UTF8_BYTES);
	}

	/* If we reach this, then vt100_read_key() returned -1 */
	sline_err = SLINE_ERR_IO;

	return -1;
}

void
sline_end(void)
{
	int i;

	if (sline_history == 0 || hist_top < 0)
		goto termios;

	for (i = 0; i < HIST_SIZE; ++i) {
		if (hist[i] != NULL)
			free(hist[i]);
	}

termios:
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
}

const char *
sline_errmsg(void)
{
	switch (sline_err) {
	case SLINE_ERR_EOF:
		return "EOF caught.";
	case SLINE_ERR_IO:
		return "I/O error.";
	case SLINE_ERR_MEMORY:
		return "could not allocate internal memory.";
	case SLINE_ERR_TERMIOS_GET:
		return "could not read attributes.";
	case SLINE_ERR_TERMIOS_SET:
		return "could not set attributes.";
	default:
		return "unknown error.";
	}
}

const char *
sline_history_get(int pos)
{
	if (pos < 0 || pos > hist_top)
		return NULL;

	return hist[pos];
}

int
sline_setup(void)
{
	sline_set_prompt(SLINE_PROMPT_DEFAULT);

	if (sline_history > 0) {
		if (hist_setup(sline_hist_entry_size) < 0) {
			sline_err = SLINE_ERR_MEMORY;
			return -1;
		}
	}

	if (tcgetattr(STDIN_FILENO, &old) < 0) {
		sline_err = SLINE_ERR_TERMIOS_GET;
		return -1;
	}

	term = old;
	term.c_lflag &= ~(ICANON | ECHO);
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) {
		sline_err = SLINE_ERR_TERMIOS_SET;
		return -1;
	}

	return 0;
}

void
sline_set_prompt(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	/* vsnprintf() is ISO C99 */
	vsnprintf(sline_prompt, SLINE_PROMPT_SIZE, fmt, ap);

	va_end(ap);
}

const char *
sline_version(void)
{
	return VERSION;
}
