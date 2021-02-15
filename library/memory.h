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
#ifndef LIB_MEMORY_H_
#define LIB_MEMORY_H_

/** \file memory.h
 * \brief Memory management header.
 */

/** \struct mem_stats
 * \brief Available to the user for display of memory statistics.
 *
 * Note that to generate and use the memory statistics the library
 * must be compiled with the macro APG_MEM_STATS defined.
 * e.g.
 * <pre>gcc -DAPG_MEM_STATS ...</pre>
 */
typedef struct {
    aint uiAllocations; /**< The number of memory allocations. */
    aint uiReAllocations; /**< The number of re-allocations.*/
    aint uiFrees; /**< The number of memory allocations freed.*/
    aint uiCells; /**< The current number of memory cells (allocations).*/
    aint uiMaxCells; /**< The maximum number of memory cells allocated.*/
    aint uiHeapBytes; /**< The current number of heap bytes allocated.*/
    aint uiMaxHeapBytes; /**< The maximum number of heap bytes allocated.*/
} mem_stats;

void* vpMemCtor(exception* spException);
void vMemDtor(void* vpCtx);
abool bMemValidate(void* vpCtx);
void* vpMemAlloc(void* vpCtx, aint uiBytes);
void vMemFree(void* vpCtx, const void* vpData);
void* vpMemRealloc(void* vpCtx, const void* vpData, aint uiBytes);
aint uiMemCount(void* vpCtx);
void vMemClear(void* vpCtx);
void vMemStats(void* vpCtx, mem_stats* spStats);
exception* spMemException(void* vpCtx);

#endif /* LIB_MEMORY_H_ */
