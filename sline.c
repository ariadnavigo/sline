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
#define SLINE_HIST_ENTRY_DEF_SIZE 64 /* Default size for history entries */
#define SLINE_PROMPT_DEFAULT "> "
#define SLINE_PROMPT_SIZE 32
#define UTF8_BYTES 4

enum {
	VT_DEF,
	VT_CHR,
	VT_BKSPC,
	VT_DLT,
	VT_EOF,
	VT_RET,
	VT_TAB, /* Tab is ignored by now. */
	VT_UP,
	VT_DWN,
	VT_LFT,
	VT_RGHT,
	VT_HOME,
	VT_END
};

static char *buf_slice(char *src, int pivot, size_t size);
static size_t cursor_end_pos(const char *buf);
static void ln_buf_replace(char *buf, size_t size, const char *src);
static void ln_redraw(const char *str, size_t nbytes);
static int utf8_nbytes(const char *utf8);
static int utf8_nbytes_r(const char *utf8);

static int term_key(char *utf8);
static int term_esc(char *seq);

static void key_up(char *buf, size_t size);
static void key_down(char *buf, size_t size);
static void key_left(char *buf);
static void key_right(char *buf);
static void key_home(void);
static void key_end(char *buf);

static void chr_delete(char *buf, size_t size, int bsmode);
static void chr_ins(char *buf, size_t size, const char *utf8);
static void chr_return(const char *buf);

static char sline_prompt[SLINE_PROMPT_SIZE];
static size_t pos, buf_i;
static struct termios old, term;

int sline_history = 1; /* History feature on by default */
int sline_err = SLINE_ERR_DEF;
size_t sline_hist_entry_size = SLINE_HIST_ENTRY_DEF_SIZE;

/* Auxiliary VT100 related subroutines */

static char *
buf_slice(char *src, int pivot, size_t size)
{
	char *suff, *ptr;
	size_t ptr_size;

	ptr = src + pivot;
	ptr_size = strlen(ptr) + 1;

	if ((suff = calloc(size, sizeof(char))) == NULL)
		return NULL;

	strlcpy(suff, ptr, ptr_size);
	memset(ptr, 0, ptr_size);

	return suff;
}

static size_t
cursor_end_pos(const char *buf)
{
	int i;
	size_t end_pos;

	i = 0;
	end_pos = 0;
	while (buf[i] != '\0') {
		++end_pos;
		i += utf8_nbytes(&buf[i]);
	}

	return end_pos;
}

static void
ln_buf_replace(char *buf, size_t size, const char *src)
{
	memset(buf, 0, size);
	strlcpy(buf, src, size);

	key_home();
	write(STDOUT_FILENO, "\x1b[0K", 4);
	write(STDOUT_FILENO, src, strlen(src));

	buf_i = strlen(buf);
	pos = cursor_end_pos(buf);
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
utf8_nbytes(const char *utf8)
{
	if (utf8[0] >= '\xc0' && utf8[0] <= '\xdf')
		return 2;
	else if (utf8[0] >= '\xe0' && utf8[0] <= '\xef')
		return 3;
	else if (utf8[0] >= '\xf0' && utf8[0] <= '\xf7')
		return 4;

	return 1;
}

static int
utf8_nbytes_r(const char *utf8)
{
	const char *ptr;
	int nbytes;

	for (ptr = utf8; (nbytes = utf8_nbytes(ptr)) == 1; --ptr) {
		/* We stop if we're an ASCII char */
		if ((unsigned char)ptr[0] <= '\x7f')
			return 1;
	}

	return nbytes;
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
term_key(char *utf8)
{
	char key;
	char seq[3];
	int nread, nbytes;

	while ((nread = read(STDIN_FILENO, &key, 1)) != 1) {
		if (nread == -1)
			return -1;
	}

	if (key == '\x1b') {
		if (term_esc(seq) < 0)
			return VT_DEF;

		if (seq[1] == '3' && seq[2] == '~')
			return VT_DLT;

		if ((seq[1] == '1' || seq[1] == '7') && seq[2] == '~')
			return VT_HOME;

		if ((seq[1] == '4' || seq[1] == '8') && seq[2] == '~')
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
	} else if (key == '\x04') {
		return VT_EOF;
	} else if (key == '\x0a') {
		return VT_RET;
	} else if (key == '\t') {
		return VT_TAB;
	} else if ((nbytes = utf8_nbytes(&key)) > 1) {
		utf8[0] = key;
		read(STDIN_FILENO, utf8 + 1, nbytes - 1);
		return VT_CHR;
	} else {
		utf8[0] = key;
		return VT_CHR;
	}
}

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

	ln_buf_replace(buf, size, hist);
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

	ln_buf_replace(buf, size, hist);
}

static void
key_left(char *buf)
{
	int nbytes;

	if (pos == 0)
		return;

	nbytes = utf8_nbytes_r(&buf[buf_i - 1]);
	buf_i -= nbytes;

	write(STDOUT_FILENO, "\x1b[D", 3);
	--pos;
}

static void
key_right(char *buf)
{
	int nbytes;

	if (pos == cursor_end_pos(buf))
		return;

	nbytes = utf8_nbytes(&buf[buf_i]);
	buf_i += nbytes;

	write(STDOUT_FILENO, "\x1b[C", 3);
	++pos;
}

static void
key_home(void)
{
	char cmd[CURSOR_BUF_SIZE];

	if (pos == 0)
		return;

	buf_i = 0;

	snprintf(cmd, CURSOR_BUF_SIZE, "\x1b[%zdD", pos);
	write(STDOUT_FILENO, cmd, strlen(cmd));
	pos = 0;
}


static void
key_end(char *buf)
{
	int i;
	size_t end_pos;
	char cmd[CURSOR_BUF_SIZE];

	if (buf[buf_i] == '\0')
		return;

	i = 0;
	end_pos = 0;
	while (buf[i] != '\0') {
		++end_pos;
		i += utf8_nbytes(&buf[i]);
	}

	buf_i = strlen(buf);

	snprintf(cmd, CURSOR_BUF_SIZE, "\x1b[%zdC", end_pos - pos);
	write(STDOUT_FILENO, cmd, strlen(cmd));
	pos = end_pos;
}

static void
chr_delete(char *buf, size_t size, int bsmode)
{
	char *suff, *suff_new;
	int nbytes;
	size_t len;

	if (bsmode > 0) {
		if (buf_i == 0)
			return;

		nbytes = utf8_nbytes_r(&buf[buf_i - 1]);
		buf_i -= nbytes;
	} else {
		nbytes = utf8_nbytes_r(&buf[buf_i]);
	}

	if ((suff = buf_slice(buf, buf_i, size)) == NULL)
		return;

	suff_new = suff + nbytes; /* Deleting character from suff; way safer */
	len = strlen(suff_new);
	strlcpy(&buf[strlen(buf)], suff_new, len + 1);

	if (bsmode > 0) {
		write(STDOUT_FILENO, "\b", 1);
		--pos;
	}
	ln_redraw(suff_new, len);

	free(suff);

	history_set(hist_top, buf);
}

static void
chr_ins(char *buf, size_t size, const char *utf8)
{
	char *suff;
	size_t len;
	int i, nbytes;

	if (pos >= size)
		return;

	if ((suff = buf_slice(buf, buf_i, size)) == NULL)
		return;

	len = strlen(suff);
	nbytes = utf8_nbytes(utf8);
	for (i = 0; i < nbytes; ++i)
		buf[strlen(buf)] = utf8[i];

	strlcpy(&buf[strlen(buf)], suff, len + 1);
	buf_i += nbytes;

	write(STDOUT_FILENO, utf8, nbytes);
	++pos;
	ln_redraw(suff, len);

	free(suff);

	if (strlen(buf) > 0)
		history_set(hist_top, buf);
}

static void
chr_return(const char *buf)
{
	write(STDOUT_FILENO, "\n", 1);
	if (sline_history > 0) {
		history_set(hist_pos, buf);
		history_next();
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
	pos = 0;
	buf_i = 0;
	if (init != NULL) {
		/*
		 * Using size instead of wsize because we're already given a
		 * null terminated string.
		 */
		strlcpy(buf, init, size);
		write(STDOUT_FILENO, buf, size);
		pos = cursor_end_pos(buf);
		buf_i = strlen(buf);
	}

	memset(utf8, 0, UTF8_BYTES);
	hist_pos = hist_top;
	while ((key = term_key(utf8)) != -1) {
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
			key_home();
			break;
		case VT_END:
			key_end(buf);
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

	return history[pos];
}

int
sline_setup(void)
{
	sline_set_prompt(SLINE_PROMPT_DEFAULT);

	if (sline_history > 0) 
		history_setup();

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
