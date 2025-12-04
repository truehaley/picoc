/* picoc heap memory allocation. */

/* stack grows up from the bottom and heap grows down from
    the top of heap space */
#include "interpreter.h"

#ifdef DEBUG_HEAP
void ShowBigList(Picoc *picoc)
{
    struct AllocNode *LPos;

    printf("Heap: bottom=0x%lx 0x%lx-0x%lx, big freelist=", (long)picoc->HeapBottom,
        (long)&(picoc->HeapMemory)[0], (long)&(picoc->HeapMemory)[HEAP_SIZE]);
    for (LPos = picoc->FreeListBig; LPos != NULL; LPos = LPos->NextFree)
        printf("0x%lx:%d ", (long)LPos, LPos->Size);

    printf("\n");
}
#endif

/* initialize the stack and heap storage */
void HeapInit(Picoc *picoc, int StackOrHeapSize)
{
    int Count;
    int AlignOffset = 0;

    picoc->HeapMemory = malloc(StackOrHeapSize);
    picoc->HeapBottom = NULL;  /* the bottom of the (downward-growing) heap */
    picoc->StackFrame = NULL;  /* the current stack frame */
    picoc->HeapStackTop = NULL;  /* the top of the stack */

    while (((unsigned long)&picoc->HeapMemory[AlignOffset] & (sizeof(ALIGN_TYPE)-1)) != 0)
        AlignOffset++;

    picoc->StackFrame = &(picoc->HeapMemory)[AlignOffset];
    picoc->HeapStackTop = &(picoc->HeapMemory)[AlignOffset];
    *(void**)(picoc->StackFrame) = NULL;
    picoc->HeapBottom =
        &(picoc->HeapMemory)[StackOrHeapSize-sizeof(ALIGN_TYPE)+AlignOffset];
    picoc->FreeListBig = NULL;
    for (Count = 0; Count < FREELIST_BUCKETS; Count++)
        picoc->FreeListBucket[Count] = NULL;
}

void HeapCleanup(Picoc *picoc)
{
    free(picoc->HeapMemory);
}

/* allocate some space on the stack, in the current stack frame
 * clears memory. can return NULL if out of stack space */
void *HeapAllocStack(Picoc *picoc, int Size)
{
    char *NewMem = picoc->HeapStackTop;
    char *NewTop = (char*)picoc->HeapStackTop + MEM_ALIGN(Size);
#ifdef DEBUG_HEAP
    printf("HeapAllocStack(%ld) at 0x%lx\n", (unsigned long)MEM_ALIGN(Size),
        (unsigned long)picoc->HeapStackTop);
#endif
    if (NewTop > (char*)picoc->HeapBottom)
        return NULL;

    picoc->HeapStackTop = (void*)NewTop;
    memset((void*)NewMem, '\0', Size);
    return NewMem;
}

/* allocate some space on the stack, in the current stack frame */
void HeapUnpopStack(Picoc *picoc, int Size)
{
#ifdef DEBUG_HEAP
    printf("HeapUnpopStack(%ld) at 0x%lx\n", (unsigned long)MEM_ALIGN(Size),
        (unsigned long)picoc->HeapStackTop);
#endif
    picoc->HeapStackTop = (void*)((char*)picoc->HeapStackTop + MEM_ALIGN(Size));
}

/* free some space at the top of the stack */
int HeapPopStack(Picoc *picoc, void *Addr, int Size)
{
    int ToLose = MEM_ALIGN(Size);
    if (ToLose > ((char*)picoc->HeapStackTop - (char*)&(picoc->HeapMemory)[0]))
        return false;

#ifdef DEBUG_HEAP
    printf("HeapPopStack(0x%lx, %ld) back to 0x%lx\n", (unsigned long)Addr,
        (unsigned long)MEM_ALIGN(Size), (unsigned long)picoc->HeapStackTop - ToLose);
#endif
    picoc->HeapStackTop = (void*)((char*)picoc->HeapStackTop - ToLose);
    assert(Addr == NULL || picoc->HeapStackTop == Addr);

    return true;
}

/* push a new stack frame on to the stack */
void HeapPushStackFrame(Picoc *picoc)
{
#ifdef DEBUG_HEAP
    printf("Adding stack frame at 0x%lx\n", (unsigned long)picoc->HeapStackTop);
#endif
    *(void**)picoc->HeapStackTop = picoc->StackFrame;
    picoc->StackFrame = picoc->HeapStackTop;
    picoc->HeapStackTop = (void*)((char*)picoc->HeapStackTop +
        MEM_ALIGN(sizeof(ALIGN_TYPE)));
}

/* pop the current stack frame, freeing all memory in the
    frame. can return NULL */
int HeapPopStackFrame(Picoc *picoc)
{
    if (*(void**)picoc->StackFrame != NULL) {
        picoc->HeapStackTop = picoc->StackFrame;
        picoc->StackFrame = *(void**)picoc->StackFrame;
#ifdef DEBUG_HEAP
        printf("Popping stack frame back to 0x%lx\n",
            (unsigned long)picoc->HeapStackTop);
#endif
        return true;
    } else
        return false;
}

/* allocate some dynamically allocated memory. memory is cleared.
    can return NULL if out of memory */
void *HeapAllocMem(Picoc *picoc, int Size)
{
    return calloc(Size, 1);
}

/* free some dynamically allocated memory */
void HeapFreeMem(Picoc *picoc, void *Mem)
{
    free(Mem);
}
