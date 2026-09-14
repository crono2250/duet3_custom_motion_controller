#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

extern char _end;
extern char _estack;

static char *heap_end;

int _close(int file)
{
    (void)file;
    errno = ENOSYS;
    return -1;
}

void _exit(int status)
{
    (void)status;
    for (;;)
    {
    }
}

int _fstat(int file, struct stat *status)
{
    (void)file;
    status->st_mode = S_IFCHR;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _kill(int pid, int signal)
{
    (void)pid;
    (void)signal;
    errno = EINVAL;
    return -1;
}

off_t _lseek(int file, off_t offset, int direction)
{
    (void)file;
    (void)offset;
    (void)direction;
    return 0;
}

ssize_t _read(int file, void *buffer, size_t length)
{
    (void)file;
    (void)buffer;
    (void)length;
    errno = ENOSYS;
    return -1;
}

void *_sbrk(ptrdiff_t increment)
{
    if (heap_end == NULL)
    {
        heap_end = &_end;
    }

    char *const previous_heap_end = heap_end;
    const uintptr_t heap_address = (uintptr_t)heap_end;
    const uintptr_t heap_start = (uintptr_t)&_end;
    const uintptr_t stack_reserve_start = (uintptr_t)&_estack - 0x800U;
    uintptr_t next_heap_address;

    if (increment >= 0)
    {
        next_heap_address = heap_address + (uintptr_t)increment;
    }
    else
    {
        const uintptr_t decrement = (uintptr_t)(-(increment + 1)) + 1U;
        if (decrement > (heap_address - heap_start))
        {
            errno = EINVAL;
            return (void *)-1;
        }
        next_heap_address = heap_address - decrement;
    }

    if (next_heap_address > stack_reserve_start)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end = (char *)next_heap_address;
    return previous_heap_end;
}

ssize_t _write(int file, const void *buffer, size_t length)
{
    (void)file;
    (void)buffer;
    return (ssize_t)length;
}
