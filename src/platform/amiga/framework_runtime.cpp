#include <exec/memory.h>
#include <proto/exec.h>

void *operator new(unsigned long size)
{
    unsigned long *allocation = (unsigned long *)AllocMem(
        size + sizeof(unsigned long), MEMF_ANY | MEMF_CLEAR);
    if (!allocation)
        return 0;
    *allocation = size + sizeof(unsigned long);
    return allocation + 1;
}

void *operator new[](unsigned long size)
{
    return operator new(size);
}

void operator delete(void *pointer)
{
    unsigned long *allocation;
    if (!pointer)
        return;
    allocation = (unsigned long *)pointer - 1;
    FreeMem(allocation, *allocation);
}

void operator delete[](void *pointer)
{
    operator delete(pointer);
}

void operator delete(void *pointer, unsigned long)
{
    operator delete(pointer);
}

void operator delete[](void *pointer, unsigned long)
{
    operator delete(pointer);
}

extern "C" void __cxa_pure_virtual(void)
{
    for (;;)
        ;
}
