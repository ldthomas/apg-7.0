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

/** \file vector.c
 *  \brief The vector object. Provides a dynamic memory array.
 *
 *  This suite of member functions provides tools for managing a dynamic array of arbitrary-sized data elements.
 *  It is based on a last-in-first-out (LIFO), push/pop, stack model with additional element access operations.<br><br>
 *  CAVEAT: Care must be taken when using data pointers returned from the member functions.
 *  It must always be assumed that they are only valid until the next member function call.
 *  If a data location needs to be retained as application state data, save its vector element index as the state
 *  data and convert it to a pointer (vpVecAt() or similar location techniques) only when needed.
 */

static const void* s_vpMagicNumber = (void*)"vector";

#if defined APG_VEC_STATS
/** \struct vector
 * \brief Private for internal use only. Defines the vector's state. Opaque to applications.
 *
 * Note that all of the elements in this structure are only present when APG_VEC_STAT is defined.
 * Those not needed for statistics collection are absent otherwise.
 */
typedef struct {
    const void* vpValidate; ///< \brief must be equal to s_vpMagicNumber
    exception* spException;
    void* vpMem;         ///< \brief context to the underlying memory component
    char* cpData;           ///< \brief pointer to the vector's data buffer
    aint uiElementSize;     ///< \brief number of bytes in one element
    aint uiReserved;        ///< \brief number of elements that have been reserved on the buffer
    aint uiUsed;            ///< \brief number of the reserved elements that have been used
    aint uiGrownCount;      ///< \brief number times the vector automatically grew in size
    aint uiGrownElements;   ///< \brief number elements vector has grown by
    aint uiPushed;          ///< \brief number of elements pushed
    aint uiPopped;          ///< \brief number of elements popped
    aint uiMaxUsed;         ///< \brief maximum number of elements used;
} vector;
static void vStatsPush(vector* spCtx, aint uiPushed);
static void vStatsPop(vector* spCtx, aint uiPopped);
static void vStatsGrow(vector* spCtx, aint uiAddedElements);
/**@name Statistics Collection Macros used by the vector object.
 * If APG_VEC_STATS is defined, these 3 macros will be defined to call the statistics gathering functions.
 * If APG_VEC_STATS is *not* defined, these macros generate no code.
 */
///@{
/** \def STATS_PUSH(s, p)
 * \brief Called by the vector object to count the number of pushed (added) elements.
 */
#define STATS_PUSH(s, p) vStatsPush((s), (p))
/** \def STATS_POP(s, p)
 * \brief Called by the vector object to count the number of popped (removed) elements.
 */
#define STATS_POP(s, p) vStatsPop((s), (p))
/** \def STATS_GROW(s, n)
 * \brief Called by the vector object to count the number of times the size of the vector was automatically extended.
 */
#define STATS_GROW(s, n) vStatsGrow((s), (n))
///@}
#else
typedef struct {
    const void* vpValidate;       // must be equal to s_vpMagicNumber
    exception* spException;
    void* vpMem;         // context to the underlying memory component
    char* cpData;           // pointer to the vector's data buffer
    aint uiElementSize;     // number of bytes in one element
    aint uiReserved;        // number of elements that have been reserved on the buffer
    aint uiUsed;            // number of the reserved elements that have been used
} vector;
#define STATS_PUSH(s, p)
#define STATS_POP(s, p)
#define STATS_GROW(s, n)
#endif

static void vGrow(vector* spCtx, aint uiElements);

/** \brief The vector object constructor.
 *
 * The vector object is one of the few APG objects that does not require an exception pointer for construction.
 * Rather it takes a memory object pointer. The memory object is the vector's parent.
 * All memory allocations are done with the parent object and all exceptions are reported
 * on the parent's exception object.
 * Calling the parent memory object's destructor will free all memory associated with this vector object.
 *
 * \param vpMem Pointer to a valid memory context, previously returned from \ref vpMemCtor()
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param uiElementSize Size, in bytes, of each array element. Must be greater than 0.
 * \param uiInitialAlloc Number of elements to initially allocate. Must be greater than 0.
 * \return Returns a vector context pointer. Throws exception on input or memory allocation error.
 */
void* vpVecCtor(void* vpMem, aint uiElementSize, aint uiInitialAlloc) {
    vector* spCtx;
    exception* spEx = NULL;
    while(APG_TRUE){
        if (!bMemValidate(vpMem)) {
            vExContext();
            break;
        }
        spEx = spMemException(vpMem);
        if (uiElementSize == 0) {
            XTHROW(spEx, "element size cannot be zero");
            break;
        }
        if (uiInitialAlloc == 0) {
            XTHROW(spEx, "initial allocation cannot be zero");
            break;
        }
        spCtx = (vector*) vpMemAlloc(vpMem, sizeof(vector));
        memset((void*) spCtx, 0, sizeof(*spCtx));
        spCtx->cpData = (char*) vpMemAlloc(vpMem, (uiElementSize * uiInitialAlloc));
        spCtx->uiElementSize = uiElementSize;
        spCtx->uiReserved = uiInitialAlloc;
        spCtx->uiUsed = 0;

        // success
        spCtx->vpMem = vpMem;
        spCtx->spException = spEx;
        spCtx->vpValidate = s_vpMagicNumber;
        return (void*) spCtx;
    }
    return NULL;
}

/** \brief The vector component destructor.
 *
 * Frees all memory allocated to this vector object.
 *
 * Note that all memory is also freed when the parent memory object is destroyed.
 *
 * \param vpCtx A vector context pointer previously returned from vpVecCtor().
 * Silently ignores NULL.
 * However, if non-NULL must be valid or the application will exit with a \ref BAD_CONTEXT exit code.
 */
void vVecDtor(void* vpCtx) {
    vector* spCtx = (vector*) vpCtx;
    if (vpCtx) {
        if (spCtx->vpValidate == s_vpMagicNumber) {
            void* vpMem = spCtx->vpMem;
            vMemFree(vpMem, (void*) spCtx->cpData);
            memset(vpCtx, 0, sizeof(vector));
            vMemFree(vpMem, vpCtx);
        }else{
            vExContext();
        }
    }
}

/** \brief Validates a vector component context.
 \param vpCtx Pointer to vector context.
 \return True if the context is valid, false otherwise.
 */
abool bVecValidate(void* vpCtx){
    vector* spCtx = (vector*) vpCtx;
    if (vpCtx && (spCtx->vpValidate == s_vpMagicNumber)) {
        return APG_TRUE;
    }
    return APG_FALSE;
}

/** \brief Adds one element to the end of the array.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param vpElement Pointer to the element to add. If NULL, space for a new element is added but no data is copied to it.
 * \return A pointer to the new element in the array. Exception thrown on memory allocation error, if any.
 */
void* vpVecPush(void* vpCtx, void* vpElement) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        void* vpReturn = NULL;
        if (spCtx->uiUsed >= spCtx->uiReserved) {
            vGrow(spCtx, 1);
        }
        vpReturn = (void*) (spCtx->cpData + (spCtx->uiUsed * spCtx->uiElementSize));
        if (vpElement) {
            // copy new element to vector
            memcpy(vpReturn, vpElement, spCtx->uiElementSize);
        }
        spCtx->uiUsed += 1;
        STATS_PUSH(spCtx, 1);
        return vpReturn;
    }
    vExContext();
    return NULL;
}

/** \brief Adds one or more elements to the end of the array.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param vpElement Pointer to the first element to add. If NULL, space for the new elements is added but no data is copied to it.
 * \param uiCount The number of elements to add.
 * \return A pointer to the new element in the array.
 Exception thrown if uiCount = 0 or on memory allocation failure.
 */
void* vpVecPushn(void* vpCtx, void* vpElement, aint uiCount) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if (uiCount) {
            void* vpReturn = NULL;
            if ((spCtx->uiUsed + uiCount) > spCtx->uiReserved) {
                vGrow(spCtx, uiCount);
            }
            vpReturn = (void*) (spCtx->cpData + (spCtx->uiUsed * spCtx->uiElementSize));
            if (vpElement) {
                memcpy(vpReturn, vpElement, uiCount * spCtx->uiElementSize);
            }
            spCtx->uiUsed += uiCount;
            STATS_PUSH(spCtx, uiCount);
            return vpReturn;
        }
        XTHROW(spCtx->spException, "attempt to push 0 elements on the vector");
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Pops one element from the end of the array.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the popped element. NULL if the vector is empty.
 NOTE: the popped element data remains valid until the next call to vpVecPush() or vpVecPushn();
 */
void* vpVecPop(void* vpCtx) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if (spCtx->uiUsed) {
            spCtx->uiUsed -= 1;
            STATS_POP(spCtx, 1);
            return ((void*) (spCtx->cpData + (spCtx->uiUsed * spCtx->uiElementSize)));
        }
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Pops one or more elements from the end of the array.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param uiCount The number of elements to pop. If uiCount > number of elements, all remaining elements are popped.
 * \return A pointer to the first (lowest index) popped element. NULL if the vector is empty or uiCount == 0.
 NOTE: the popped element data remains valid until the next call to vpVecPush() or vpVecPushn();
 */
void* vpVecPopn(void* vpCtx, aint uiCount) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if ((uiCount > 0) && (spCtx->uiUsed > 0)) {
            if (spCtx->uiUsed > uiCount) {
                // pop uiCount elements
                spCtx->uiUsed -= uiCount;
                STATS_POP(spCtx, uiCount);
                return (void*) (spCtx->cpData + (spCtx->uiUsed * spCtx->uiElementSize));
            }
            // pop all remaining elements
            STATS_POP(spCtx, spCtx->uiUsed);
            spCtx->uiUsed = 0;
            return spCtx->cpData;
        }
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Pops the element at the given zero-based index and all higher indexes.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param uiIndex Index of the first element to pop. Element uiIndex and all higher indexed elements are popped.
 * \return A pointer to the first popped element, the element with index uiIndex.
 NULL if the vector is empty or no elements are popped. i.e. if uiIndex is >= uiVecLen(vpCtx).
 NOTE: the popped element data remains valid until the next call to vpVecPush() or vpVecPushn().<br>
 Example: A common usage is to restore a vector to its previous state. Suppose a vector, ctx, has 10 elements.<br>
 <pre>
 uiCount = uiVecLen(ctx);           // uiCount = 10, the number of elements on the vector
 vpNew = vpVecPushn(ctx, NULL, 5);  // vpNew points to element 11 (index 10), there are now 15 elements on the vector
 vpOld = vpVecPopi(ctx, uiCount);   // vpOld now points to the popped element 11 (index = 10) and the vector is restored to its previous condition
 </pre>
 */
void* vpVecPopi(void* vpCtx, aint uiIndex) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(uiIndex >= spCtx->uiUsed){
            return NULL;
        }
        aint uiCount = spCtx->uiUsed - uiIndex; // will always be positive
        spCtx->uiUsed -= uiCount;
        STATS_POP(spCtx, uiCount);
        return ((void*) (spCtx->cpData + (uiIndex * spCtx->uiElementSize)));
    }
    vExContext();
    return NULL;
}

/** \brief Get the first element one the vector. The vector is not altered.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the first element in the array. NULL if the array is empty.
 */
void* vpVecFirst(void* vpCtx) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if (spCtx->uiUsed) {
            return ((void*) spCtx->cpData);
        }
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Get the last element one the vector. The vector is not altered.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \return A pointer to the last element in the array. NULL if the array is empty.
 */
void* vpVecLast(void* vpCtx) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if (spCtx->uiUsed) {
            return ((void*) (spCtx->cpData + (spCtx->uiUsed - 1) * spCtx->uiElementSize));
        }
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Get a the indexed vector element. The vector is not altered.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param uiIndex The index of the element to get (0-based).
 * \return A pointer to the indexed element in the array.
 NULL if the array is empty or the index is out of range.
 */
void* vpVecAt(void* vpCtx, aint uiIndex) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        if(spCtx->uiUsed == 0){
            return NULL;
        }
        if(uiIndex == 0){
            return ((void*) spCtx->cpData);
        }
        if(uiIndex < spCtx->uiUsed){
            return ((void*) (spCtx->cpData + (uiIndex * spCtx->uiElementSize)));
        }
        return NULL;
    }
    vExContext();
    return NULL;
}

/** \brief Get the vector length. That is, the number of elements on the vector
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \return The number of elements currently in the vector.
 */
aint uiVecLen(void* vpCtx){
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        return spCtx->uiUsed;
    }
    vExContext();
    return 0;
}

/** \brief Get a pointer to the vector buffer.
 *
 * This is different from vpVecFirst() in that it will always point to the buffer
 * even if there are no elements currently in it.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \return Pointer beginning of the current buffer
 */
void* vpVecBuffer(void* vpCtx){
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx && (spCtx->vpValidate == s_vpMagicNumber)){
        return (void*)spCtx->cpData;
    }
    vExContext();
    return NULL;
}

/** \brief Clears all used elements in a vector component.
 *
 This simply resets the current element pointer to zero.
 No data is actually deleted.
 No memory is released or re-allocated.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 If NULL, the call is silently ignored. However, if non-NULL it must be a valid context pointer.
 */
void vVecClear(void* vpCtx) {
    vector* spCtx = (vector*) vpCtx;
    if(vpCtx){
        if(spCtx->vpValidate == s_vpMagicNumber){
            STATS_POP(spCtx, spCtx->uiUsed);
            spCtx->uiUsed = 0;
        }else{
            vExContext();
        }
    }
}

/** \brief Doubles the size of the vector buffer.
 *
 * Old data is preserved.
 * * \param spCtx - pointer to a vector context
 * * \param uiElements - minimum number of additional elements to make room for
 * \return void. Note that an exception is thrown if there is a memory reallocation error.
 */
static void vGrow(vector* spCtx, aint uiElements) {
    aint uiNewReserved;
    uiNewReserved = 2 * (spCtx->uiReserved + uiElements);
    spCtx->cpData = (char*)vpMemRealloc(spCtx->vpMem, (void*) spCtx->cpData, (spCtx->uiElementSize * uiNewReserved));

//    XTHROW(spCtx->spException, "pretending that the memory reallocation failed while growing the vector");

    STATS_GROW(spCtx, (uiNewReserved - spCtx->uiReserved));
    spCtx->uiReserved = uiNewReserved;
}

#if defined APG_VEC_STATS
/** Copy the vector statistics in the user's buffer.
 *
 * Note that APG_VEC_STATS must be defined for statistics collection.
 * If not defined, this function will return an empty \ref vec_stats structure.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param spStats - pointer to the user's stats buffer
 */
void vVecStats(void* vpCtx, vec_stats* spStats){
    vector* spCtx = (vector*) vpCtx;
    if(spStats){
        spStats->uiElementSize = spCtx->uiElementSize;
        spStats->uiReserved = spCtx->uiReserved;
        spStats->uiUsed = spCtx->uiUsed;
        spStats->uiMaxUsed = spCtx->uiMaxUsed;
        spStats->uiPopped = spCtx->uiPopped;
        spStats->uiPushed = spCtx->uiPushed;
        spStats->uiGrownCount = spCtx->uiGrownCount;
        spStats->uiGrownElements = spCtx->uiGrownElements;
        spStats->uiGrownBytes = spCtx->uiElementSize * spCtx->uiGrownElements;
        spStats->uiReservedBytes = spCtx->uiElementSize * spCtx->uiReserved;
        spStats->uiOriginalBytes = spStats->uiReservedBytes - spStats->uiGrownBytes;
        spStats->uiOriginalElements = spStats->uiOriginalBytes / spCtx->uiElementSize;
        spStats->uiUsedBytes = spCtx->uiElementSize * spCtx->uiUsed;
        spStats->uiMaxUsedBytes = spCtx->uiElementSize * spCtx->uiMaxUsed;
    }
}
void vStatsPush(vector* spCtx, aint uiCount){
    spCtx->uiPushed += uiCount;
    if(spCtx->uiUsed > spCtx->uiMaxUsed){
        spCtx->uiMaxUsed = spCtx->uiUsed;
    }
}
void vStatsPop(vector* spCtx, aint uiCount){
    spCtx->uiPopped += uiCount;
}
void vStatsGrow(vector* spCtx, aint uiAddedElements){
    spCtx->uiGrownCount += 1;
    spCtx->uiGrownElements += uiAddedElements;
}
#else
/** In release build, simply zero the user's buffer.
 * \param vpCtx A valid vector context pointer previously returned from vpVecCtor().
 * If invalid, silently exits with a \ref BAD_CONTEXT exit code.
 * \param spStats - pointer to the user's stats buffer
 */
void vVecStats(void* vpCtx, vec_stats* spStats){
    if(spStats){
        memset((void*) spStats, 0, sizeof(vec_stats));
    }
}
#endif
