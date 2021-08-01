/* See LICENSE for copyright and license details. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "history.h"
#include "sline.h"
#include "strlcpy.h"

#define CURSOR_BUF_SIZE 16 /* Used for cursor movement directives */
#define SLINE_PROMPT_DEFAULT "> " 
#define SLINE_PROMPT_SIZE 32

enum {
	VT_DEF,
	VT_CHR,
	VT_BKSPC,
	VT_DLT,
	VT_EOF,
	VT_RET,
	VT_UP,
	VT_DWN,
	VT_LFT,
	VT_RGHT,
	VT_HOME,
	VT_END
};

static char *buf_slice(char *src, int pivot);
static void ln_redraw(const char *str, size_t nbytes);

static int term_key(char *chr);
static int term_esc(char *seq);

static size_t key_up(char *buf, size_t size, size_t pos);
static size_t key_down(char *buf, size_t size, size_t pos);
static size_t key_left(size_t pos);
static size_t key_right(char *buf, size_t pos);
static size_t key_home(size_t pos);
static size_t key_end(char *buf, size_t pos);

static size_t chr_delete(char *buf, size_t pos, int bsmode);
static size_t chr_insert(char *buf, size_t pos, size_t size, char chr);
static void chr_return(void);

static char sline_prompt[SLINE_PROMPT_SIZE];
static int sline_history = 1; /* History feature on by default */
static struct termios old, term;

int sline_errno = SLINE_ERR_DEF;

/* Auxiliary VT100 related subroutines */

static char *
buf_slice(char *src, int pivot)
{
	char *suff;
	size_t len;

	len = strlen(src);

	if ((suff = calloc(len + 1, sizeof(char))) == NULL)
		return NULL;

	strlcpy(suff, src + pivot, len - pivot + 1);
	memset(src + pivot, 0, len - pivot);

	return suff;
}

static void
ln_redraw(const char *str, size_t nbytes)
{
	write(STDOUT_FILENO, "\x1b[0K", 4);
	write(STDOUT_FILENO, "\x1b", 1);
	write(STDOUT_FILENO, "7", 1); /* ESC 7: portable save cursor */
	write(STDOUT_FILENO, str, nbytes);
	write(STDOUT_FILENO, "\x1b", 1);
	write(STDOUT_FILENO, "8", 1); /* ESC 8: portable restore cursor */
}

static int
term_esc(char *seq)
{
	if (read(STDIN_FILENO, &seq[0], 1) != 1)
		return -1;
	if (read(STDIN_FILENO, &seq[1], 1) != 1)
		return -1;

	if (seq[0] != '[')
		return -1;

	if (seq[1] >= '0' && seq[1] <= '9') {
		if (read(STDIN_FILENO, &seq[2], 1) != 1)
			return -1;
	}

	return 0;
}

static int
term_key(char *chr)
{
	char key;
	char seq[3];
	int nread;

	while ((nread = read(STDIN_FILENO, &key, 1)) != 1) {
		if (nread == -1) {
			sline_errno = SLINE_ERR_IO;
			return -1;
		}
	}

	if (key == '\x1b') {
		if (term_esc(seq) < 0)
			return VT_DEF;

		if (seq[1] == '3' && seq[2] == '~')
			return VT_DLT;

		if (seq[1] == '7' && seq[2] == '~')
			return VT_HOME;

		if (seq[1] == '8' && seq[2] == '~')
			return VT_END;

		switch (seq[1]) {
		case 'A':
			return VT_UP;
		case 'B':
			return VT_DWN;
		case 'C':
			return VT_RGHT;
		case 'D':
			return VT_LFT;
		case 'H':
			return VT_HOME;
		case 'F':
			return VT_END;
		default:
			return VT_DEF;
		}
	} else if (key == '\x7f') {
		return VT_BKSPC;
	} else if (key == '\x03' || key == '\x04') {
		return VT_EOF;
	} else if (key == '\x0a') {
		return VT_RET;
	} else {
		/* Not an escaped or control key */
		*chr = key;
		return VT_CHR;
	}
}

static size_t
key_up(char *buf, size_t size, size_t pos)
{
	const char *hist;
	size_t len;

	if (sline_history == 0)
		return pos;

	if (hist_pos > 0)
		--hist_pos;

	if ((hist = history_get(hist_pos)) == NULL)
		return pos;

	strlcpy(buf, hist, size);

	pos = key_home(pos);
	len = strlen(hist);
	write(STDOUT_FILENO, "\x1b[0K", 4);
	write(STDOUT_FILENO, hist, len);

	return len;
}

static size_t
key_down(char *buf, size_t size, size_t pos)
{
	const char *hist;
	size_t len;

	if (sline_history == 0)
		return pos;

	if (hist_pos < hist_curr)
		++hist_pos;
	else
		hist_pos = hist_curr;

	if ((hist = history_get(hist_pos)) == NULL)
		return pos;

	strlcpy(buf, hist, size);

	pos = key_home(pos);
	len = strlen(hist);
	write(STDOUT_FILENO, "\x1b[0K", 4);
	write(STDOUT_FILENO, hist, len);
	
	return len; /* len is the new pos we output */
}

static size_t
key_left(size_t pos)
{
	if (pos > 0) {
		--pos;
		write(STDOUT_FILENO, "\x1b[D", 3);
	}

	return pos;
}

static size_t
key_right(char *buf, size_t pos)
{
	if (pos < strlen(buf)) {
		++pos;
		write(STDOUT_FILENO, "\x1b[C", 3);
	}
	
	return pos;
}

static size_t
key_home(size_t pos)
{
	char cmd[CURSOR_BUF_SIZE];

	if (pos > 0) {
		snprintf(cmd, CURSOR_BUF_SIZE, "\x1b[%zdD", pos);
		write(STDOUT_FILENO, cmd, strlen(cmd));
	}

	return 0;
}


static size_t
key_end(char *buf, size_t pos)
{
	size_t len;
	char cmd[CURSOR_BUF_SIZE];

	len = strlen(buf);
	if (pos < len) {
		snprintf(cmd, CURSOR_BUF_SIZE, "\x1b[%zdC", len - pos);
		write(STDOUT_FILENO, cmd, strlen(cmd));
	}

	return len;
}

static size_t
chr_delete(char *buf, size_t pos, int bsmode)
{
	char *suff, *suff_new;
	size_t len;

	if (bsmode > 0) {
		if (pos == 0)
			return pos;
		--pos;
	}

	if ((suff = buf_slice(buf, pos)) == NULL)
		return pos;

	suff_new = suff + 1; /* Deleting character from suff; way safer */
	len = strlen(suff_new);
	strlcpy(buf + pos, suff_new, len + 1);

	if (bsmode > 0)
		write(STDOUT_FILENO, "\b", 1);

	ln_redraw(suff_new, len);

	free(suff);

	history_set(hist_curr, buf);

	return pos;
}

static size_t
chr_insert(char *buf, size_t pos, size_t size, char chr)
{
	char *suff;
	size_t len;

	if (pos >= size)
		return pos;

	if ((suff = buf_slice(buf, pos)) == NULL)
		return pos;

	len = strlen(suff);
	buf[pos] = chr;
	++pos;
	strlcpy(buf + pos, suff, len + 1);

	write(STDOUT_FILENO, &chr, 1);
	ln_redraw(suff, len);

	free(suff);

	history_set(hist_curr, buf);
	
	return pos;
}

static void
chr_return(void)
{
	write(STDOUT_FILENO, "\n", 1);
	if (sline_history > 0)
		history_next();
}

/* Public sline API subroutines follow */

int
sline(char *buf, size_t size)
{
	char chr;
	int key;
	size_t pos;

	memset(buf, 0, size);

	write(STDOUT_FILENO, sline_prompt, SLINE_PROMPT_SIZE);

	chr = 0;
	pos = 0;
	hist_pos = hist_curr;
	while ((key = term_key(&chr)) != -1) {
		switch (key) {
		case VT_BKSPC:
			pos = chr_delete(buf, pos, 1);
			hist_pos = hist_curr;
			break;
		case VT_DLT:
			pos = chr_delete(buf, pos, 0);
			hist_pos = hist_curr;
			break;
		case VT_EOF:
			write(STDOUT_FILENO, "\n", 1);
			sline_errno = SLINE_ERR_EOF;
			return -1;
		case VT_RET:
			chr_return();
			return 0;
		case VT_UP:
			pos = key_up(buf, size, pos);
			break;
		case VT_DWN:
			pos = key_down(buf, size, pos);
			break;
		case VT_LFT:
			pos = key_left(pos);
			break;
		case VT_RGHT:
			pos = key_right(buf, pos);
			break;
		case VT_HOME:
			pos = key_home(pos);
			break;
		case VT_END:
			pos = key_end(buf, pos);
			break;
		case VT_CHR:
			pos = chr_insert(buf, pos, size, chr);
			hist_pos = hist_curr;
			break;
		default:
			/* Silently ignore everything that isn't caught. */
			break;
		}

	}

	return -1;
}

void
sline_end(void)
{
	int i;

	if (sline_history == 0 || hist_curr < 0)
		goto termios;

	for (i = 0; i < HISTORY_SIZE; ++i) {
		if (history[i] != NULL)
			free(history[i]);
	}

termios:
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
}

const char *
sline_errmsg(void)
{
	switch (sline_errno) {
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

int
sline_setup(int entry_size)
{
	int i;

	sline_set_prompt(SLINE_PROMPT_DEFAULT);

	if (entry_size <= 0) {
		sline_history = 0; /* Disabling history */
		goto termios;
	}

	hist_entry_size = entry_size;
	for (i = 0; i < HISTORY_SIZE; ++i) {
		history[i] = calloc(hist_entry_size, sizeof(char));
		if (history[i] == NULL) {
			sline_errno = SLINE_ERR_MEMORY;
			return -1;
		}
	}

termios:
	if (tcgetattr(STDIN_FILENO, &old) < 0) {
		sline_errno = SLINE_ERR_TERMIOS_GET;
		return -1;
	}

	term = old;
	term.c_lflag &= ~(ICANON | ECHO | ISIG);
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) {
		sline_errno = SLINE_ERR_TERMIOS_SET;
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
