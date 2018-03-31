/* Support files for GNU libc.  Files in the system namespace go here.
   Files in the C namespace (ie those that do not start with an
   underscore) go in .c.  */

#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "swi.h"

// from:
//      newlib-1.17.0/newlib/libc/sys/arm/syscalls.c
//
#include "monit.h"
#if	APPLICATION_MODE
//
//
int _user_puts(char *s);
int _user_putc(int c);


/* Forward prototypes.  */
int     _system     _PARAMS ((const char *));
int     _rename     _PARAMS ((const char *, const char *));
int     _isatty		_PARAMS ((int));
clock_t _times		_PARAMS ((struct tms *));
int     _gettimeofday	_PARAMS ((struct timeval *, void *));
void    _raise 		_PARAMS ((void));
int     _unlink		_PARAMS ((const char *));
int     _link 		_PARAMS ((void));
int     _stat 		_PARAMS ((const char *, struct stat *));
int     _fstat 		_PARAMS ((int, struct stat *));
caddr_t _sbrk		_PARAMS ((int));
int     _getpid		_PARAMS ((int));
int     _kill		_PARAMS ((int, int));
void    _exit		_PARAMS ((int));
int     _close		_PARAMS ((int));
int     _swiclose	_PARAMS ((int));
int     _open		_PARAMS ((const char *, int, ...));
int     _swiopen	_PARAMS ((const char *, int));
int     _write 		_PARAMS ((int, char *, int));
int     _swiwrite	_PARAMS ((int, char *, int));
int     _lseek		_PARAMS ((int, int, int));
int     _swilseek	_PARAMS ((int, int, int));
int     _read		_PARAMS ((int, char *, int));
int     _swiread	_PARAMS ((int, char *, int));
void    initialise_monitor_handles _PARAMS ((void));

static int	wrap		_PARAMS ((int));
static int	error		_PARAMS ((int));
static int	get_errno	_PARAMS ((void));
static int	remap_handle	_PARAMS ((int));
//static int	do_AngelSWI	_PARAMS ((int, void *));
static int 	findslot	_PARAMS ((int));

/* Register name faking - works in collusion with the linker.  */
register char * stack_ptr asm ("sp");


/* following is copied from libc/stdio/local.h to check std streams */
extern void   _EXFUN(__sinit,(struct _reent *));
#define CHECK_INIT(ptr) \
  do						\
    {						\
      if ((ptr) && !(ptr)->__sdidinit)		\
	__sinit (ptr);				\
    }						\
  while (0)

/* Adjust our internal handles to stay away from std* handles.  */
#define FILE_HANDLE_OFFSET (0x20)

static int monitor_stdin =1;
static int monitor_stdout=2;
static int monitor_stderr=3;

/* Struct used to keep track of the file position, just so we
   can implement fseek(fh,x,SEEK_CUR).  */
typedef struct {
	int handle;
	int pos;
} poslog;

#define MAX_OPEN_FILES 5	// 20

static poslog openfiles [MAX_OPEN_FILES];

static int findslot (int fh)
{
	int i;
	for (i = 0; i < MAX_OPEN_FILES; i ++)
		if (openfiles[i].handle == fh)
			break;
	return i;
}

/* Function to convert std(in|out|err) handles to internal versions.  */
static int remap_handle (int fh)
{
	CHECK_INIT(_REENT);

	if (fh == STDIN_FILENO)
		return monitor_stdin;
	if (fh == STDOUT_FILENO)
		return monitor_stdout;
	if (fh == STDERR_FILENO)
		return monitor_stderr;

	return fh - FILE_HANDLE_OFFSET;
}

void initialise_monitor_handles (void)
{
	int i;

#if	0
	int fh;
	const char * name;

	name = ":tt";
	asm ("mov r0,%2; mov r1, #0; swi %a1; mov %0, r0"
	     : "=r"(fh)
	     : "i" (SWI_Open),"r"(name)
	     : "r0","r1");
	monitor_stdin = fh;

	name = ":tt";
	asm ("mov r0,%2; mov r1, #4; swi %a1; mov %0, r0"
	     : "=r"(fh)
	     : "i" (SWI_Open),"r"(name)
	     : "r0","r1");
	monitor_stdout = monitor_stderr = fh;
#endif


	for (i = 0; i < MAX_OPEN_FILES; i ++)
		openfiles[i].handle = -1;

	openfiles[0].handle = monitor_stdin;
	openfiles[0].pos = 0;
	openfiles[1].handle = monitor_stdout;
	openfiles[1].pos = 0;
}

static int get_errno (void)
{
#if	0
	asm ("swi %a0" :: "i" (SWI_GetErrno));
#endif
	return 0;	//������.
}

static int error (int result)
{
	errno = get_errno ();
	return result;
}

static int wrap (int result)
{
	if (result == -1)
		return error (-1);
	return result;
}

/* Returns # chars not! written.  */
int _swiread (int file,
              char * ptr,
              int len)
{
#if	0
	int fh = remap_handle (file);
	asm ("mov r0, %1; mov r1, %2;mov r2, %3; swi %a0"
	     : /* No outputs */
	     : "i"(SWI_Read), "r"(fh), "r"(ptr), "r"(len)
	     : "r0","r1","r2");
#endif

	return len;		//������.
}

int _read (int file,
           char * ptr,
           int len)
{
	int slot = findslot (remap_handle (file));
	int x = _swiread (file, ptr, len);

	if (x < 0)
		return error (-1);

	if (slot != MAX_OPEN_FILES)
		openfiles [slot].pos += len - x;

	/* x == len is not an error, at least if we want feof() to work.  */
	return len - x;
}

int _swilseek (int file,
               int ptr,
               int dir)
{
	int res=0;
	int fh = remap_handle (file);
	int slot = findslot (fh);

	if (dir == SEEK_CUR) {
		if (slot == MAX_OPEN_FILES)
			return -1;
		ptr = openfiles[slot].pos + ptr;
		dir = SEEK_SET;
	}

	if (dir == SEEK_END) {
#if	0
		asm ("mov r0, %2; swi %a1; mov %0, r0"
		     : "=r" (res)
		     : "i" (SWI_Flen), "r" (fh)
		     : "r0");
#endif
		ptr += res;
	}

#if	0
	/* This code only does absolute seeks.  */
	asm ("mov r0, %2; mov r1, %3; swi %a1; mov %0, r0"
	     : "=r" (res)
	     : "i" (SWI_Seek), "r" (fh), "r" (ptr)
	     : "r0", "r1");
#endif

	if (slot != MAX_OPEN_FILES && res == 0)
		openfiles[slot].pos = ptr;

	/* This is expected to return the position in the file.  */
	return res == 0 ? ptr : -1;
}

int _lseek (int file,
            int ptr,
            int dir)
{
	return wrap (_swilseek (file, ptr, dir));
}

/* Returns #chars not! written.  */
int _swiwrite (
    int    file,
    char * ptr,
    int    len)
{

#if	0
	char buf[64];
	sprintf(buf,"_swiwrite(%d,0x%x,%x)\n",file,ptr[0],len);
	_user_puts(buf);
#endif

	//������.
	if(file<3) {
		while(len) {
			_user_putc(*ptr++);
			len--;
		}
	}
	return len;	

#if	0
	int fh = remap_handle (file);
	asm ("mov r0, %1; mov r1, %2;mov r2, %3; swi %a0"
	     : /* No outputs */
	     : "i"(SWI_Write), "r"(fh), "r"(ptr), "r"(len)
	     : "r0","r1","r2");
#endif

}

int
_write (int    file,
        char * ptr,
        int    len)
{
	int slot = findslot (remap_handle (file));
	int x = _swiwrite (file, ptr,len);

	if (x == -1 || x == len)
		return error (-1);

	if (slot != MAX_OPEN_FILES)
		openfiles[slot].pos += len - x;

	return len - x;
}

//extern int strlen (const char *);


#if	0

int
_swiopen (const char * path,
          int          flags)
{
	int aflags = 0, fh;

	int i = findslot (-1);

	if (i == MAX_OPEN_FILES)
		return -1;

	/* The flags are Unix-style, so we need to convert them.  */
#ifdef O_BINARY
	if (flags & O_BINARY)
		aflags |= 1;
#endif

	if (flags & O_RDWR)
		aflags |= 2;

	if (flags & O_CREAT)
		aflags |= 4;

	if (flags & O_TRUNC)
		aflags |= 4;

	if (flags & O_APPEND) {
		aflags &= ~4;     /* Can't ask for w AND a; means just 'a'.  */
		aflags |= 8;
	}

	asm ("mov r0,%2; mov r1, %3; swi %a1; mov %0, r0"
	     : "=r"(fh)
	     : "i" (SWI_Open),"r"(path),"r"(aflags)
	     : "r0","r1");

	if (fh >= 0) {
		openfiles[i].handle = fh;
		openfiles[i].pos = 0;
	}

	return fh >= 0 ? fh + FILE_HANDLE_OFFSET : error (fh);
}

#endif

int
_open (const char * path,
       int          flags,
       ...)
{
	return 2;	//������.

#if	0
	return wrap (_swiopen (path, flags));
#endif
}

int
_swiclose (int file)
{
	int myhan = remap_handle (file);
	int slot = findslot (myhan);

	if (slot != MAX_OPEN_FILES)
		openfiles[slot].handle = -1;

#if	0
	asm ("mov r0, %1; swi %a0" :: "i" (SWI_Close),"r"(myhan):"r0");
#endif
	return 0;
}

int _close (int file)
{
	return wrap (_swiclose (file));
}

int _kill (int pid, int sig)
{
	(void)pid;
	(void)sig;

	return 0;

#if	0
	asm ("swi %a0" :: "i" (SWI_Exit));
#endif
}

void _exit(int status)
{
	/* There is only one SWI for both _exit and _kill. For _exit, call
	   the SWI with the second argument set to -1, an invalid value for
	   signum, so that the SWI handler can distinguish the two calls.
	   Note: The RDI implementation of _kill throws away both its
	   arguments.  */
	_kill(status, -1);
	while(1);
}

int _getpid (int n)
{
	return 1;
	n = n;
}

extern char   end[];		// == .bss �Z�N�V�����̍Ō�����x����
#define	ResvSizeStk	0x120	//    .userstk �Z�N�V�����̑傫����������Heap�Ƃ��Ďg�p.

caddr_t _sbrk (int incr)
{
	static char  *sbrk_heap_end=0;
	char *        prev_sbrk_heap_end;

	if (sbrk_heap_end == NULL) {
		sbrk_heap_end = &end[ResvSizeStk];
	}
	prev_sbrk_heap_end = sbrk_heap_end;

	if (sbrk_heap_end + incr > stack_ptr) {
		/* Some of the libstdc++-v3 tests rely upon detecting
			 out of memory errors, so do not abort here.  */
		errno = ENOMEM;

		return (caddr_t) -1;
	}
	sbrk_heap_end += incr;
	return (caddr_t) prev_sbrk_heap_end;
}

//extern void memset (struct stat *, int, unsigned int);

int _fstat (int file, struct stat * st)
{
	memset (st, 0, sizeof (* st));
	st->st_mode = S_IFCHR;
	st->st_blksize = 1024;
	return 0;
	file = file;
}

int _stat (const char *fname, struct stat *st)
{
	int file;

	/* The best we can do is try to open the file readonly.  If it exists,
	   then we can guess a few things about it.  */
	if ((file = _open (fname, O_RDONLY)) < 0)
		return -1;

	memset (st, 0, sizeof (* st));
	st->st_mode = S_IFREG | S_IREAD;
	st->st_blksize = 1024;
	_swiclose (file); /* Not interested in the error.  */
	return 0;
}

int _link (void)
{
	return -1;
}

int _unlink (const char *path)
{
	return -1;
}

void _raise (void)
{
	return;
}

int _gettimeofday (struct timeval * tp, void * tzvp)
{
	struct timezone *tzp = tzvp;
	if (tp) {
		/* Ask the host for the seconds since the Unix epoch.  */
		{
			int value;
			asm ("swi %a1; mov %0, r0" : "=r" (value): "i" (SWI_Time) : "r0");
			tp->tv_sec = value;
		}
		tp->tv_usec = 0;
	}

	/* Return fixed data for the timezone.  */
	if (tzp) {
		tzp->tz_minuteswest = 0;
		tzp->tz_dsttime = 0;
	}

	return 0;
}

/* Return a clock that ticks at 100Hz.  */
clock_t _times (struct tms * tp)
{
	clock_t timeval;

	asm ("swi %a1; mov %0, r0" : "=r" (timeval): "i" (SWI_Clock) : "r0");

	if (tp) {
		tp->tms_utime  = timeval;	/* user time */
		tp->tms_stime  = 0;	/* system time */
		tp->tms_cutime = 0;	/* user time, children */
		tp->tms_cstime = 0;	/* system time, children */
	}

	return timeval;
};


int _isatty (int fd)
{
	return (fd <= 2) ? 1 : 0;  /* one of stdin, stdout, stderr */
}

int _system (const char *s)
{
	if (s == NULL)
		return 0;
	errno = ENOSYS;
	return -1;
}

int _rename (const char * oldpath, const char * newpath)
{
	errno = ENOSYS;
	return -1;
}


#endif	//APPLICATION_MODE
