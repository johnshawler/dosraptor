/* Minimal freestanding C library for the Amiga port (dos.library / exec.library backed). */
#include "os.h"
#include <stdarg.h>
#include <stddef.h>

void *DOSBase;
#define NULL ((void*)0)

/* ------------------------------------------------------------ memory / string */
void *memcpy(void *d, const void *s, size_t n)
{
	unsigned char *dp = d; const unsigned char *sp = s;
	if (n >= 8 && !(((unsigned)dp | (unsigned)sp) & 3)) {
		unsigned *dl = (unsigned *)dp; const unsigned *sl = (const unsigned *)sp;
		for (size_t k = n >> 2; k; k--) *dl++ = *sl++;
		dp = (unsigned char *)dl; sp = (const unsigned char *)sl; n &= 3;
	}
	while (n--) *dp++ = *sp++;
	return d;
}
void *memmove(void *d, const void *s, size_t n)
{
	unsigned char *dp = d; const unsigned char *sp = s;
	if (dp == sp) return d;
	if (dp < sp || dp >= sp + n) return memcpy(d, s, n);
	dp += n; sp += n; while (n--) *--dp = *--sp;
	return d;
}
void *memset(void *d, int c, size_t n)
{
	unsigned char *dp = d;
	if (n >= 8 && !((unsigned)dp & 3)) {
		unsigned v = (unsigned char)c; v |= v << 8; v |= v << 16;
		unsigned *dl = (unsigned *)dp;
		for (size_t k = n >> 2; k; k--) *dl++ = v;
		dp = (unsigned char *)dl; n &= 3;
	}
	while (n--) *dp++ = c;
	return d;
}
int memcmp(const void *a, const void *b, size_t n)
{
	const unsigned char *x = a, *y = b;
	for (; n; n--, x++, y++) if (*x != *y) return *x - *y;
	return 0;
}
size_t strlen(const char *s) { const char *p = s; while (*p) p++; return p - s; }
char *strcpy(char *d, const char *s) { char *r = d; while ((*d++ = *s++)); return r; }
char *strncpy(char *d, const char *s, size_t n) { char *r = d; while (n && (*d++ = *s++)) n--; while (n-- > 1) *d++ = 0; return r; }
char *strcat(char *d, const char *s) { strcpy(d + strlen(d), s); return d; }
char *strncat(char *d, const char *s, size_t n) { char *r = d; d += strlen(d); while (n-- && (*d = *s++)) d++; *d = 0; return r; }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) a++, b++; return (unsigned char)*a - (unsigned char)*b; }
int strncmp(const char *a, const char *b, size_t n) { while (n && *a && *a == *b) a++, b++, n--; return n ? (unsigned char)*a - (unsigned char)*b : 0; }
int toupper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
int tolower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
int isdigit(int c) { return c >= '0' && c <= '9'; }
int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
int isalnum(int c) { return isalpha(c) || isdigit(c); }
int isspace(int c) { return c == ' ' || (c >= 9 && c <= 13); }
int isupper(int c) { return c >= 'A' && c <= 'Z'; }
int islower(int c) { return c >= 'a' && c <= 'z'; }
int isprint(int c) { return c >= 32 && c < 127; }
int stricmp(const char *a, const char *b) { while (*a && tolower(*a) == tolower(*b)) a++, b++; return tolower((unsigned char)*a) - tolower((unsigned char)*b); }
int strcmpi(const char *a, const char *b) { return stricmp(a, b); }
int strnicmp(const char *a, const char *b, size_t n) { while (n && *a && tolower(*a) == tolower(*b)) a++, b++, n--; return n ? tolower((unsigned char)*a) - tolower((unsigned char)*b) : 0; }
char *strupr(char *s) { for (char *p = s; *p; p++) *p = toupper(*p); return s; }
char *strlwr(char *s) { for (char *p = s; *p; p++) *p = tolower(*p); return s; }
char *strchr(const char *s, int c) { for (;; s++) { if (*s == (char)c) return (char *)s; if (!*s) return NULL; } }
char *strrchr(const char *s, int c) { const char *r = NULL; for (;; s++) { if (*s == (char)c) r = s; if (!*s) return (char *)r; } }
char *strstr(const char *h, const char *n) { size_t l = strlen(n); for (; *h; h++) if (!strncmp(h, n, l)) return (char *)h; return l ? NULL : (char *)h; }
char *strtok(char *s, const char *delim)
{
	static char *save;
	if (s) save = s; if (!save) return NULL;
	while (*save && strchr(delim, *save)) save++;
	if (!*save) { save = NULL; return NULL; }
	char *start = save;
	while (*save && !strchr(delim, *save)) save++;
	if (*save) *save++ = 0; else save = NULL;
	return start;
}
long strtol(const char *s, char **end, int base)
{
	long v = 0; int neg = 0;
	while (isspace(*s)) s++;
	if (*s == '-') { neg = 1; s++; } else if (*s == '+') s++;
	if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { s += 2; base = 16; }
	else if (base == 0) base = (s[0] == '0') ? 8 : 10;
	for (;; s++) {
		int d = isdigit(*s) ? *s - '0' : isalpha(*s) ? tolower(*s) - 'a' + 10 : 99;
		if (d >= base) break; v = v * base + d;
	}
	if (end) *end = (char *)s;
	return neg ? -v : v;
}
unsigned long strtoul(const char *s, char **e, int b) { return (unsigned long)strtol(s, e, b); }
int atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }
int abs(int v) { return v < 0 ? -v : v; }
long labs(long v) { return v < 0 ? -v : v; }
static unsigned rand_state = 1;
int rand(void) { rand_state = rand_state * 1103515245u + 12345u; return (rand_state >> 16) & 0x7fff; }
void srand(unsigned s) { rand_state = s; }

/* ------------------------------------------------------------------ printf */
static int put_num(char *o, char *end, unsigned long v, int base, int neg, int width, int left, int zero, int upper)
{
	char tmp[24]; int n = 0, len = 0;
	do { int d = v % base; tmp[n++] = d < 10 ? '0' + d : (upper ? 'A' : 'a') + d - 10; v /= base; } while (v);
	if (neg) tmp[n++] = '-';
	int pad = width > n ? width - n : 0;
	#define PUT(c) do { if (o < end) *o = (c); o++; len++; } while (0)
	if (!left && !zero) while (pad--) PUT(' ');
	if (neg && zero && !left) { PUT('-'); n--; while (pad--) PUT('0'); }
	else if (zero && !left) while (pad--) PUT('0');
	while (n) PUT(tmp[--n]);
	if (left) while (pad--) PUT(' ');
	return len;
}
int vsnprintf(char *buf, size_t size, const char *f, va_list ap)
{
	char *o = buf, *end = buf + (size ? size - 1 : 0);
	for (; *f; f++) {
		if (*f != '%') { if (o < end) *o = *f; o++; continue; }
		f++; int left = 0, zero = 0, width = 0, lng = 0, prec = -1;
		for (;; f++) { if (*f == '-') left = 1; else if (*f == '0') zero = 1; else if (*f == '+' || *f == ' ' || *f == '#') ; else break; }
		if (*f == '*') { width = va_arg(ap, int); f++; } else while (isdigit(*f)) width = width * 10 + *f++ - '0';
		if (*f == '.') { f++; prec = 0; if (*f == '*') { prec = va_arg(ap, int); f++; } else while (isdigit(*f)) prec = prec * 10 + *f++ - '0'; }
		while (*f == 'l' || *f == 'h') { if (*f == 'l') lng = 1; f++; }
		switch (*f) {
		case 'd': case 'i': { long v = lng ? va_arg(ap, long) : va_arg(ap, int); int neg = v < 0; o += put_num(o, end, neg ? -v : v, 10, neg, width, left, zero, 0); break; }
		case 'u': { unsigned long v = va_arg(ap, unsigned long); o += put_num(o, end, v, 10, 0, width, left, zero, 0); break; }
		case 'x': case 'X': { unsigned long v = va_arg(ap, unsigned long); o += put_num(o, end, v, 16, 0, width, left, zero, *f == 'X'); break; }
		case 'p': { unsigned long v = (unsigned long)va_arg(ap, void *); o += put_num(o, end, v, 16, 0, 8, 0, 1, 0); break; }
		case 'c': { int c = va_arg(ap, int); if (o < end) *o = c; o++; break; }
		case 's': { const char *s = va_arg(ap, const char *); if (!s) s = "(null)"; int l = strlen(s); if (prec >= 0 && l > prec) l = prec; int pad = width > l ? width - l : 0;
			if (!left) while (pad--) { if (o < end) *o = ' '; o++; }
			while (l--) { if (o < end) *o = *s; o++; s++; }
			if (left) while (pad--) { if (o < end) *o = ' '; o++; } break; }
		case '%': if (o < end) *o = '%'; o++; break;
		default: if (o < end) *o = *f; o++; break;
		}
	}
	if (size) *(o < end ? o : end) = 0;
	return o - buf;
}
int vsprintf(char *buf, const char *f, va_list ap) { return vsnprintf(buf, 0x7fffffff, f, ap); }
int sprintf(char *buf, const char *f, ...) { va_list ap; va_start(ap, f); int r = vsprintf(buf, f, ap); va_end(ap); return r; }
int snprintf(char *buf, size_t n, const char *f, ...) { va_list ap; va_start(ap, f); int r = vsnprintf(buf, n, f, ap); va_end(ap); return r; }
char *ltoa(long v, char *buf, int base) { if (base == 10) sprintf(buf, "%ld", v); else sprintf(buf, "%lx", v); return buf; }

/* ---------------------------------------------------------- allocation (exec) */
typedef struct { unsigned size, pad; } Hdr;
void *malloc(size_t n)
{
	Hdr *h = AllocMem(n + sizeof(Hdr), MEMF_ANY);
	if (!h) return NULL;
	h->size = n + sizeof(Hdr); return h + 1;
}
void *calloc(size_t a, size_t b)
{
	Hdr *h = AllocMem(a * b + sizeof(Hdr), MEMF_CLEAR);
	if (!h) return NULL;
	h->size = a * b + sizeof(Hdr); return h + 1;
}
void free(void *p) { if (p) { Hdr *h = (Hdr *)p - 1; FreeMem(h, h->size); } }
void *realloc(void *p, size_t n)
{
	if (!p) return malloc(n);
	Hdr *h = (Hdr *)p - 1; size_t old = h->size - sizeof(Hdr);
	void *q = malloc(n); if (!q) return NULL;
	memcpy(q, p, old < n ? old : n); free(p); return q;
}

/* --------------------------------------------------------------- low-level io */
#define MAXFD 32
static LONG fdtab[MAXFD];
int open(const char *name, int flags, ...)
{
	int fd; for (fd = 3; fd < MAXFD; fd++) if (!fdtab[fd]) break;
	if (fd == MAXFD) return -1;
	LONG fh = Open(name, (flags & 3) || (flags & 0x40) ? ((flags & 0x40) ? MODE_NEWFILE : 1004 /*MODE_READWRITE*/) : MODE_OLDFILE);
	if (!fh) return -1;
	fdtab[fd] = fh; return fd;
}
int open64(const char *n, int f, ...) { return open(n, f); }
int close(int fd) { if (fd < 3 || !fdtab[fd]) return -1; Close(fdtab[fd]); fdtab[fd] = 0; return 0; }
int read(int fd, void *b, unsigned n) { return Read(fdtab[fd], b, n); }
int write(int fd, const void *b, unsigned n) { if (fd < 3) return n; return Write(fdtab[fd], b, n); }
long lseek(int fd, long off, int whence) { long old = Seek(fdtab[fd], 0, OFFSET_CURRENT); if (old < 0) return -1; if (Seek(fdtab[fd], off, whence == 0 ? OFFSET_BEGINNING : whence == 1 ? OFFSET_CURRENT : OFFSET_END) < 0) return -1; return Seek(fdtab[fd], 0, OFFSET_CURRENT); }
long long lseek64(int fd, long long off, int whence) { return lseek(fd, (long)off, whence); }
long filelength(int fd) { long cur = lseek(fd, 0, 1); long end = lseek(fd, 0, 2); lseek(fd, cur, 0); return end; }
int access(const char *name, int mode) { LONG fh = Open(name, MODE_OLDFILE); if (!fh) return -1; Close(fh); return 0; }
int remove(const char *name) { register void *a6 __asm("a6") = DOSBase; register ULONG d1 __asm("d1") = (ULONG)name; register LONG d0 __asm("d0"); __asm volatile("jsr -72(%%a6)" : "=r"(d0), "+r"(d1) : "r"(a6) : "a0", "a1", "memory", "cc"); return d0 ? 0 : -1; }
char *getenv(const char *n) { return NULL; }
int chsize(int fd, long sz) { return 0; }
int fileno(void *f) { return *(int *)f; }

/* stdio on top of fds (FILE = {fd, ungot}) */
typedef struct { int fd; int err; int eof; } MFILE;
static MFILE mstd[3] = { { 0 }, { 1 }, { 2 } };
void *stdin = &mstd[0], *stdout = &mstd[1], *stderr = &mstd[2];
void *fopen(const char *name, const char *mode)
{
	int fl = 0; if (mode[0] == 'w') fl = 0x40 | 2; else if (mode[0] == 'a') fl = 2; else if (strchr(mode, '+')) fl = 2;
	int fd = open(name, fl); if (fd < 0) return NULL;
	MFILE *f = malloc(sizeof(MFILE)); f->fd = fd; f->err = f->eof = 0; return f;
}
void *fopen64(const char *n, const char *m) { return fopen(n, m); }
int fclose(void *fp) { MFILE *f = fp; int r = close(f->fd); free(f); return r; }
size_t fread(void *b, size_t sz, size_t n, void *fp) { MFILE *f = fp; int r = read(f->fd, b, sz * n); if (r <= 0) { f->eof = 1; return 0; } return r / sz; }
size_t fwrite(const void *b, size_t sz, size_t n, void *fp) { MFILE *f = fp; if (f->fd == 1 || f->fd == 2) { char tmp[512]; size_t l = sz * n; if (l > 511) l = 511; memcpy(tmp, b, l); tmp[l] = 0; dbg_write(tmp); return n; } int r = write(f->fd, b, sz * n); return r > 0 ? r / sz : 0; }
int fseek(void *fp, long off, int wh) { return lseek(((MFILE *)fp)->fd, off, wh) < 0 ? -1 : 0; }
long ftell(void *fp) { return lseek(((MFILE *)fp)->fd, 0, 1); }
int fflush(void *fp) { return 0; }
int fgetc(void *fp) { unsigned char c; return fread(&c, 1, 1, fp) == 1 ? c : -1; }
char *fgets(char *b, int n, void *fp) { int i = 0; while (i < n - 1) { int c = fgetc(fp); if (c < 0) break; b[i++] = c; if (c == '\n') break; } if (!i) return NULL; b[i] = 0; return b; }
int fputc(int c, void *fp) { char ch = c; fwrite(&ch, 1, 1, fp); return c; }
int fputs(const char *s, void *fp) { fwrite(s, 1, strlen(s), fp); return 0; }
int fprintf(void *fp, const char *f, ...) { char b[512]; va_list ap; va_start(ap, f); int r = vsnprintf(b, sizeof b, f, ap); va_end(ap); fwrite(b, 1, r, fp); return r; }
int vfprintf(void *fp, const char *f, va_list ap) { char b[512]; int r = vsnprintf(b, sizeof b, f, ap); fwrite(b, 1, r, fp); return r; }
/* debug output from interrupt context: collected in a ring buffer, written out by the next dbg_write()/dbg_flush() from the main task */
char isr_ring[4096]; volatile unsigned isr_head, isr_tail;
void isr_log(const char *f, ...)
{
	char b[160]; va_list ap; va_start(ap, f); int n = vsnprintf(b, sizeof b, f, ap); va_end(ap);
	for (int i = 0; i < n; i++) { unsigned nh = (isr_head + 1) & 4095; if (nh == isr_tail) break; isr_ring[isr_head] = b[i]; isr_head = nh; }
}
int dbgfd;
extern void inject_log(const char *s);
void dbg_flush(void);
void dbg_write(const char *s)
{
	inject_log(s);
	if (!dbgfd) { LONG fh = Open("SYS:dbg.log", MODE_NEWFILE); dbgfd = fh ? fh : -1; }
	if (dbgfd > 0) { extern volatile unsigned plat_vbl_count; char pre[16]; int n = sprintf(pre, "[%u] ", plat_vbl_count); Write(dbgfd, pre, n); Write(dbgfd, s, strlen(s)); }
}
void dbg_flush(void)
{
	char b[256]; int n = 0;
	while (isr_tail != isr_head && n < 255) { b[n++] = isr_ring[isr_tail]; isr_tail = (isr_tail + 1) & 4095; }
	if (n) { b[n] = 0; if (!dbgfd) dbg_write(""); if (dbgfd > 0) Write(dbgfd, b, n); }
}
int printf(const char *f, ...) { char b[512]; va_list ap; va_start(ap, f); int r = vsnprintf(b, sizeof b, f, ap); va_end(ap); dbg_write(b); return r; }
int vprintf(const char *f, va_list ap) { char b[512]; int r = vsnprintf(b, sizeof b, f, ap); dbg_write(b); return r; }
int puts(const char *s) { dbg_write(s); dbg_write("\n"); return 0; }
int sscanf(const char *s, const char *f, ...)
{
	va_list ap; va_start(ap, f); int n = 0;
	for (; *f; f++) {
		if (*f != '%') { if (isspace(*f)) { while (isspace(*s)) s++; } else if (*s == *f) s++; else break; continue; }
		f++; while (isdigit(*f) || *f == 'l') f++;
		while (isspace(*s)) s++;
		char *e; long v;
		if (*f == 'x' || *f == 'X') v = strtol(s, &e, 16); else if (*f == 'd' || *f == 'u' || *f == 'i') v = strtol(s, &e, 10); else break;
		if (e == s) break; s = e; *va_arg(ap, int *) = v; n++;
	}
	va_end(ap); return n;
}
int __isoc99_sscanf(const char *s, const char *f, ...) __attribute__((alias("sscanf")));
int *__errno_location(void) { static int e; return &e; }

/* exit: return to the OS through the crt0 exit path */
extern void _exit_to_os(int rc) __attribute__((noreturn));
void exit(int rc) { if (dbgfd > 0) Close(dbgfd); _exit_to_os(rc); }
void abort(void) { exit(20); }
char *strerror(int e) { return "error"; }
