/*  *************************************************************************************
    Copyright (c) 2021, Lowell D. Thomas
    All rights reserved.
    
    This file is part of APG Version 7.0.
    APG Version 7.0 may be used under the terms of the BSD 2-Clause License.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    
    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    
*   *************************************************************************************/
#include "./lib.h"
#include <stdarg.h>

/** \file memory.c
 * \brief The memory management object.
 *
 * This file holds the memory management class or object.
 * Almost all APG objects and applications use this memory object for control
 * of all memory allocations and frees. A primary feature of this object is that
 * its destruction automatically frees all memory allocations that have been made with it.
 * Memory allocation failures are reported with
 * an exception thrown to the parent application's catch block. This frees the application
 * of all the burdensome code for checking the return and handling an error for each
 * and every allocation.
 */

/** struct mem_cell
 * \brief A circularly-linked list structure.
 *
 * The memory object keeps track of all memory allocations in a circularly-inked list.
 * This is the structure used to implement this behavior.
 * */
typedef struct mem_cell_tag {
    struct mem_cell_tag* spPrev; ///< \brief pointer to the previous cell
    struct mem_cell_tag* spNext; ///< \brief pointer to the next cell
    aint uiSize; ///< \brief The usable size, in bytes, of this memory allocation.
    aint uiSeq;  ///< \brief The sequence number of this cell.
} mem_cell;


#if defined APG_MEM_STATS

/** \struct mem
 * \brief For internal use only. The memory object's context. Opaque to user applications.
 *
 * Note that all of the elements in this structure are only present when APG_MEM_STAT is defined.
 * Those not needed for statistics collection are absent otherwise.
 */
typedef struct {
    const void* vpValidate;///< \brief validation handle
    exception* spException; ///< \brief Pointer to the exception struct. NULL if none.
    aint uiActiveCellCount; ///< \brief number of cells on the active list
    mem_cell* spActiveList;///< \brief pointer to the first (and last) cell in a circularly, doubly linked list
    mem_stats sStats;///< \brief memory statistics
}mem;

/**@name Statistics Collection Macros used by the memory object.
 * If APG_MEM_STATS is defined, these 3 macros will be defined to call the statistics gathering functions.
 * If APG_MEM_STATS is *not* defined, these macros generate no code.
 */
///@{
/** \def STATS_ALLOC(s, i)
 * \brief Called by the memory object to count memory allocations.
 */
#define STATS_ALLOC(s, i) vStatsAlloc((s), (i))
/** \def STATS_FREE(s, i)
 * \brief Called by the memory object to count memory frees.
 */
#define STATS_FREE(s, i) vStatsFree((s), (i))
/** \def STATS_REALLOC(s, i)
 * \brief Called by the memory object to count memory re-allocations.
 */
#define STATS_REALLOC(s, i, o) vStatsRealloc((s), (i), (o))
static void vStatsAlloc(mem_stats* spStats, mem_cell* spIn);
static void vStatsFree(mem_stats* spStats, mem_cell* spIn);
static void vStatsRealloc(mem_stats* spStats, mem_cell* spIn, mem_cell* spOut);
#else
typedef struct {
    const void* vpValidate; ///< \brief validation handle
    exception* spException; ///< \brief Pointer to the exception struct. NULL if none.
    aint uiActiveCellCount;  // number of cells on the active list
    mem_cell* spActiveList; // pointer to the first (and last) cell in a circularly, doubly linked list
} mem;
#define STATS_ALLOC(s, i)
#define STATS_FREE(s, i)
#define STATS_REALLOC(s, i, o)
#endif
///@}

static const void *s_vpMagicNumber = (void*)"memory";
static const char* s_cpMemory = "memory allocation error";
static void vActivePush(mem* spCtx, mem_cell* spCellIn);
static void vActivePop(mem* spCtx, mem_cell* spCellIn);

/** \brief Construct a memory component.
 *
 * \param spException Pointer to a valid exception structure initialized with vExCtor() or \ref XCTOR().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to a memory object context on success.
 * Throws exception on memory allocation failure.
 */
void* vpMemCtor(exception* spException) {
    if(!bExValidate(spException)){
        vExContext();
    }
    mem* spCtx = NULL;
    spCtx = (mem*) malloc(sizeof(mem));
    if (!spCtx) {
        XTHROW(spException, "malloc failure");
    }
    memset((void*) spCtx, 0, sizeof(mem));
    spCtx->spException = spException;
    spCtx->vpValidate = s_vpMagicNumber;
    return (void*) spCtx;
}

/** \brief Destroys a Memory component. Frees all memory allocated.
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * Silently ignored if NULL.
 * However, if non-NULL it must be a valid memory context pointer.
 */
void vMemDtor(void* vpCtx) {
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx) {
        // if the context pointer is non-NULL it must be a valid memory object
        if (spCtx->vpValidate == s_vpMagicNumber) {
            vMemClear(vpCtx);
            memset(vpCtx, 0, sizeof(mem));
            free(vpCtx);
        }else{
            vExContext();
        }
    }
}


/** \brief Validates a memory context.
 * \param vpCtx A pointer to a memory context.
 * \return APG_TRUE if the handle is valid, APG_FALSE otherwise.
 */
abool bMemValidate(void* vpCtx){
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Get a pointer to this memory objects's exception handler.
 *
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 \return Pointer to this memory object's exception structure
 */
exception* spMemException(void* vpCtx){
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        return spCtx->spException;
    }else{
        vExContext();
    }
    return NULL;
}

/** \brief Allocates memory.
 *
 * Note that uiBytes + sizeof(mem_cell) is actually allocated on the heap.
 * - The data is | cell struct | ... user data ... |
 * - The actual malloc() heap allocation is at cell struct
 * - The user data area is at heap allocation address + sizeof(cell struct)
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param uiBytes the number of bytes of memory to allocate
 * \return a pointer to the user data portion of the allocated memory.
 * Exception thrown on memory allocation failure.
 */
void* vpMemAlloc(void* vpCtx, aint uiBytes) {
    mem* spCtx = (mem*) vpCtx;
    if (!vpCtx || (spCtx->vpValidate != s_vpMagicNumber)) {
        vExContext();
        return NULL;
    }
    mem_cell* spCell;
    spCell = (mem_cell*) malloc(uiBytes + sizeof(mem_cell));
    if (spCell) {
        spCell->uiSize = uiBytes;

        // push it on the active list
        vActivePush(spCtx, spCell);
        STATS_ALLOC(&spCtx->sStats, spCell);

        // success - return pointer to user data, not the actual heap block
        return (void*) (spCell + 1);
    }
    // malloc failed
    XTHROW(spCtx->spException, s_cpMemory);
    return NULL;
}
/** \brief Free memory previously allocated with vpMemAlloc().
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param vpData Pointer to the data to free. A NULL pointer is silently ignored.
 * However, an invalid pointer - a pointer not previously returned from \ref vpMemAlloc() or \ref vpMemRealloc() -
 * will result in a thrown exception.
 * vExContext() exception thrown if context pointer is invalid.
 */
void vMemFree(void* vpCtx, const void* vpData) {
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx && spCtx->vpValidate == s_vpMagicNumber) {
        if (vpData) {
            mem* spCtx = (mem*) vpCtx;
            mem_cell* spCell;
            spCell = (mem_cell*) vpData;
            --spCell; // this backs off from the user data to the actual heap allocation address

            // validate the data (must be a valid cell)
            aint ui;
            mem_cell* spThis = spCtx->spActiveList;
            for(ui = 0; ui < spCtx->uiActiveCellCount; ui++){
                if(spThis == spCell){
                    goto found;
                }
                spThis = spThis->spNext;
            }
            XTHROW(spCtx->spException, "attempt to free an unallocated memory address");

            // pop from active list
            found:;
            STATS_FREE(&spCtx->sStats, spCell);
            vActivePop(spCtx, spCell);
        }
    }else{
        vExContext();
    }
}
/** \brief Re-allocates memory previously allocated with vpMemAlloc().
 *
 * Can be used to up-size or down-size a memory allocation.
 Any previous data in the memory allocation is copied to the re-allocation.
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param vpData Pointer to the data to free.
 * An invalid pointer - a pointer not previously returned from \ref vpMemAlloc() or \ref vpMemRealloc() -
 * will result in a thrown exception.
 \param uiBytes number of re-allocated bytes of memory. Must be > 0;
 \return pointer to the re-allocated memory.
 Throws exception on memory allocation error, invalid data pointer error or uiBytes=0.
 */
void* vpMemRealloc(void* vpCtx, const void* vpData, aint uiBytes) {
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx && spCtx->vpValidate == s_vpMagicNumber) {
        mem_cell* spOldCell;
        mem_cell* spNewCell;
        void* vpDst;
        void* vpSrc;
        aint uiCopy;
        if(!vpData){
            XTHROW(spCtx->spException, "data pointer cannot be NULL");
        }
        if(!uiBytes){
            XTHROW(spCtx->spException, "byte-size for re-allocation cannot be 0");
        }
        spOldCell = (mem_cell*) vpData;
        --spOldCell;

        // validate the data (must be a valid cell)
        aint ui;
        mem_cell* spThis = spCtx->spActiveList;
        for(ui = 0; ui < spCtx->uiActiveCellCount; ui++){
            if(spThis == spOldCell){
                goto found;
            }
            spThis = spThis->spNext;
        }
        XTHROW(spCtx->spException, "attempt to re-allocate an unallocated memory address");

        found:;
        if(spOldCell->uiSize == uiBytes){
            // no need to reallocate
            return (void*)(spOldCell + 1);
        }

        // get the new allocation
        spNewCell = (mem_cell*) malloc(uiBytes + sizeof(mem_cell));
        if (spNewCell) {
            spNewCell->spNext = spOldCell->spNext;
            spNewCell->spPrev = spOldCell->spPrev;
            spOldCell->spNext->spPrev = (struct mem_cell_tag*) spNewCell;
            spOldCell->spPrev->spNext = (struct mem_cell_tag*) spNewCell;
            spNewCell->uiSeq = spOldCell->uiSeq;
            spNewCell->uiSize = uiBytes;

            // copy the data from old allocation to new
            uiCopy = uiBytes < spOldCell->uiSize ? uiBytes : spOldCell->uiSize;
            vpDst = (void*) (spNewCell + 1);
            vpSrc = (void*) (spOldCell + 1);
            memcpy(vpDst, vpSrc, uiCopy);
            if(spCtx->spActiveList == spOldCell){
                // deleting the first cell, reassign the active list
                spCtx->spActiveList = spNewCell;
            }

            // free the old data
            STATS_REALLOC(&spCtx->sStats, spOldCell, spNewCell);
            free((void*) spOldCell);
            return (void*) (spNewCell + 1);
        }
        // malloc failed
        XTHROW(spCtx->spException, s_cpMemory);
    }else{
        vExContext();
    }
    return NULL;
}

/** \brief Returns the number of memory allocations.
 *
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 \return Current number of allocations.
 * vExContext() exception thrown if context pointer is invalid.
 */
aint uiMemCount(void* vpCtx) {
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx && spCtx->vpValidate == s_vpMagicNumber) {
        mem_cell* spLast;
        if (spCtx->spActiveList) {
            spLast = (mem_cell*) spCtx->spActiveList->spPrev;
            return (spLast->uiSeq + 1);
        }
        return (aint) 0;
    }else{
        vExContext();
    }
    return (aint) 0;
}

/** \brief Frees all memory allocations.
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 */
void vMemClear(void* vpCtx) {
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        mem_cell* spLast;
        while (spCtx->spActiveList) {
            spLast = (mem_cell*) spCtx->spActiveList->spPrev;
            STATS_FREE(&spCtx->sStats, spLast);
            vActivePop(spCtx, spLast);
        }
    }else{
        vExContext();
    }
}

/** \brief Push a new memory cell on the linked list.
 * \param spCtx - pointer to the memory context previously returned from vpMemCtor()
 * \param spCellIn - the new cell to be pushed
 * \return void
 */
static void vActivePush(mem* spCtx, mem_cell* spCellIn) {
    struct mem_cell_tag* spLast;
    struct mem_cell_tag* spFirst;
    struct mem_cell_tag* spCell = (struct mem_cell_tag*) spCellIn;

    // sequence number roll over is a fatal error
    // link the cell
    switch (spCtx->uiActiveCellCount) {
    case 0:
        spCtx->spActiveList = spCell;
        spCell->spNext = spCell;
        spCell->spPrev = spCell;
        spCell->uiSeq = 0;
        break;
    case 1:
        spLast = spCtx->spActiveList;
        spLast->spNext = spCell;
        spLast->spPrev = spCell;
        spCell->spNext = spLast;
        spCell->spPrev = spLast;
        spCell->uiSeq = spLast->uiSeq + 1;
        break;
    default:
        spFirst = spCtx->spActiveList;
        spLast = spFirst->spPrev;
        spFirst->spPrev = spCell;
        spLast->spNext = spCell;
        spCell->spNext = spFirst;
        spCell->spPrev = spLast;
        spCell->uiSeq = spLast->uiSeq + 1;
        break;
    }

    ++spCtx->uiActiveCellCount;
}

/** \brief Removes (pops) a memory cell from the linked list.
 *
 * Throws an exception if the data pointer has not been a return from vpMemAlloc().
 * \param spCtx - pointer to the memory context previously returned from vpMemCtor()
 * \param spCellIn - the memory cell to be removed (popped)
 * \return void
 */
static void vActivePop(mem* spCtx, mem_cell* spCellIn) {
    struct mem_cell_tag* spPrev;
    struct mem_cell_tag* spNext;
    struct mem_cell_tag* spCell = (struct mem_cell_tag*) spCellIn;
    if (spCtx->spActiveList == spCell) {
        // update the pointer to the first cell
        if (spCtx->uiActiveCellCount == 1) {
            spCtx->spActiveList = NULL;
        } else {
            spCtx->spActiveList = spCtx->spActiveList->spNext;
        }
    }

    // remove the cell from the active list
    spPrev = spCell->spPrev;
    spNext = spCell->spNext;
    spPrev->spNext = spCell->spNext;
    spNext->spPrev = spCell->spPrev;

    --spCtx->uiActiveCellCount;

    // free the data
    free((void*) spCell);
}

#if defined APG_MEM_STATS

/** \brief Returns a copy of the Memory component's current statistics.
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spStats pointer to a user-supplied statistics buffer.
 * \return void
 */
void vMemStats(void* vpCtx, mem_stats* spStats) {
    mem* spCtx = (mem*) vpCtx;
    if (vpCtx && spCtx->vpValidate == s_vpMagicNumber) {
        if (spStats) {
            mem* spMemCtx = (mem*) vpCtx;
            memcpy((void*) spStats, (void*) &spMemCtx->sStats, sizeof(mem_stats));
        }
    }
}

/* static helper functions */
/** Updates memory statistics on allocation
 * \param spStats - pointer to a statistics structure
 * \param spIn - pointer to the memory cell being modified
 * \return void
 */
static void vStatsAlloc(mem_stats* spStats, mem_cell* spIn) {
    spStats->uiAllocations += 1;
    spStats->uiCells += 1;
    spStats->uiHeapBytes += spIn->uiSize + sizeof(mem_cell);
    if (spStats->uiCells > spStats->uiMaxCells) {
        spStats->uiMaxCells = spStats->uiCells;
    }
    if (spStats->uiHeapBytes > spStats->uiMaxHeapBytes) {
        spStats->uiMaxHeapBytes = spStats->uiHeapBytes;
    }
}

/** Updates memory statistics after free
 * \param spStats - pointer to a statistics structure
 * \param spIn - pointer to the memory cell being modified
 * \return void
 */
static void vStatsFree(mem_stats* spStats, mem_cell* spIn) {
    spStats->uiFrees += 1;
    spStats->uiCells -= 1;
    spStats->uiHeapBytes -= spIn->uiSize + sizeof(mem_cell);
}
/** Updates memory statistics after reallocation
 * \param spStats - pointer to a statistics structure
 * \param spIn - pointer to the memory cell being modified
 * \param spOut - pointer to the new, reallocated memory cell
 * \return void
 */
static void vStatsRealloc(mem_stats* spStats, mem_cell* spIn, mem_cell* spOut) {
    spStats->uiReAllocations += 1;
    if(spOut->uiSize > spIn->uiSize){
        spStats->uiHeapBytes += spOut->uiSize - spIn->uiSize;
    }else{
        spStats->uiHeapBytes -= spIn->uiSize - spOut->uiSize;
    }
    if (spStats->uiHeapBytes > spStats->uiMaxHeapBytes) {
        spStats->uiMaxHeapBytes = spStats->uiHeapBytes;
    }
}
#else
/** Just return an empty struct.
 * \param vpCtx A pointer to a valid memory context previously returned from vpMemCtor().
 * If invalid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spStats pointer to a user-supplied statistics buffer. May be NULL, in which case only the required size of the statistics buffer is returned.
 * \return void
 */
void vMemStats(void* vpCtx, mem_stats* spStats) {
    if (spStats) {
        memset((void*) spStats, 0, sizeof(mem_stats));
    }
}
#endif
