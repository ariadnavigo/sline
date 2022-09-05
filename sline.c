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

static void key_up(char *buf, size_t size);
static void key_down(char *buf, size_t size);
static void key_left(char *buf);
static void key_right(char *buf);

static void chr_delete(char *buf, size_t size, int bsmode);
static void chr_ins(char *buf, size_t size, const char *utf8);
static void chr_return(const char *buf);

static char sline_prompt[SLINE_PROMPT_SIZE];
static struct termios old, term;

int sline_history = 1; /* History feature on by default */
int sline_err = SLINE_ERR_DEF;
size_t sline_hist_entry_size = SLINE_HIST_ENTRY_DEF_SIZE;

/* Auxiliary VT100 related subroutines */

static void
key_up(char *buf, size_t size)
{
	const char *hist;

	if (sline_history == 0)
		return;

	if (hist_pos > 0)
		--hist_pos;

	if ((hist = sline_history_get(hist_pos)) == NULL)
		return;

	vt100_ln_buf_replace(buf, size, hist);
}

static void
key_down(char *buf, size_t size)
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

	vt100_ln_buf_replace(buf, size, hist);
}

static void
key_left(char *buf)
{
	int nbytes;

	if (vt100_pos == 0)
		return;

	nbytes = vt100_utf8_nbytes_r(&buf[vt100_buf_i - 1]);
	vt100_buf_i -= nbytes;

	write(STDOUT_FILENO, "\x1b[D", 3);
	--vt100_pos;
}

static void
key_right(char *buf)
{
	int nbytes;

	if (vt100_pos == vt100_cur_get_end_pos(buf))
		return;

	nbytes = vt100_utf8_nbytes(&buf[vt100_buf_i]);
	vt100_buf_i += nbytes;

	write(STDOUT_FILENO, "\x1b[C", 3);
	++vt100_pos;
}

static void
chr_delete(char *buf, size_t size, int bsmode)
{
	char *suff, *suff_new;
	int nbytes;
	size_t len;

	if (bsmode > 0) {
		if (vt100_buf_i == 0)
			return;

		nbytes = vt100_utf8_nbytes_r(&buf[vt100_buf_i - 1]);
		vt100_buf_i -= nbytes;
	} else {
		nbytes = vt100_utf8_nbytes_r(&buf[vt100_buf_i]);
	}

	if ((suff = vt100_buf_slice(buf, vt100_buf_i, size)) == NULL)
		return;

	suff_new = suff + nbytes; /* Deleting character from suff; way safer */
	len = strlen(suff_new);
	strlcpy(&buf[strlen(buf)], suff_new, len + 1);

	if (bsmode > 0) {
		write(STDOUT_FILENO, "\b", 1);
		--vt100_pos;
	}
	vt100_ln_redraw(suff_new, len);

	free(suff);

	if (sline_history > 0)
		hist_set(hist_top, buf);
}

static void
chr_ins(char *buf, size_t size, const char *utf8)
{
	char *suff;
	size_t len;
	int i, nbytes;

	if (vt100_pos >= size)
		return;

	if ((suff = vt100_buf_slice(buf, vt100_buf_i, size)) == NULL)
		return;

	len = strlen(suff);
	nbytes = vt100_utf8_nbytes(utf8);
	for (i = 0; i < nbytes; ++i)
		buf[strlen(buf)] = utf8[i];

	strlcpy(&buf[strlen(buf)], suff, len + 1);
	vt100_buf_i += nbytes;

	write(STDOUT_FILENO, utf8, nbytes);
	++vt100_pos;
	vt100_ln_redraw(suff, len);

	free(suff);

	if (sline_history > 0 && strlen(buf) > 0)
		hist_set(hist_top, buf);
}

static void
chr_return(const char *buf)
{
	write(STDOUT_FILENO, "\n", 1);
	if (sline_history > 0) {
		hist_set(hist_pos, buf);
		hist_next();
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
	vt100_pos = 0;
	vt100_buf_i = 0;
	if (init != NULL) {
		/*
		 * Using size instead of wsize because we're already given a
		 * null terminated string.
		 */
		strlcpy(buf, init, size);
		write(STDOUT_FILENO, buf, size);
		vt100_cur_goto_end(buf);	
	}

	memset(utf8, 0, UTF8_BYTES);
	hist_pos = hist_top;
	while ((key = vt100_read_key(utf8)) != -1) {
		switch (key) {
		case VT_BKSPC:
			chr_delete(buf, size, 1);
			hist_pos = hist_top;
			break;
		case VT_DLT:
			chr_delete(buf, size, 0);
			hist_pos = hist_top;
			break;
		case VT_EOF:
			write(STDOUT_FILENO, "\n", 1);
			sline_err = SLINE_ERR_EOF;
			return -1;
		case VT_RET:
			chr_return(buf);
			return 0;
		case VT_UP:
			key_up(buf, wsize);
			break;
		case VT_DWN:
			key_down(buf, wsize);
			break;
		case VT_LFT:
			key_left(buf);
			break;
		case VT_RGHT:
			key_right(buf);
			break;
		case VT_HOME:
			vt100_cur_goto_home();
			break;
		case VT_END:
			vt100_cur_goto_end(buf);
			break;
		case VT_CHR:
			chr_ins(buf, wsize, utf8);
			hist_pos = hist_top;
			break;
		default:
			/* Silently ignore everything that isn't caught. */
			break;
		}

		memset(utf8, 0, UTF8_BYTES);
	}

	/* If we reach this, then term_key() returned -1 */
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
