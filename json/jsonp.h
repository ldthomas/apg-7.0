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
/** \file jsonp.h
 * \brief Private JSON component header file.
 */
#ifndef JSONP_H_
#define JSONP_H_

/** @name Private Macros.
 * For internal object use only.
 */
#define TAB                 9
#define LF                  10
#define CR                  13
#define LINE_LEN            16
#define LINE_LEN4           4
#define LINE_LEN8           8
#define LINE_LEN12          12

/** \struct string_r
 * \brief This is the "relative" string developed during parsing.
 *
 * Because parsed strings are pushed on vectors, the base of the vector may change with any push.
 * Therefore, we must keep only relative indexes to the relevant items.
 * During post-parse processing these relative indexes will be converted to absolute pointers.
 * See \ref json_value_tag.
 */
typedef struct {
    aint uiCharsOffset; ///< The offset from the vector base of 32-bit character codes to the first character in the string.
    aint uiLength; ///< The number of characters in the string.
} string_r;

/** \struct value_r
 * \brief This is the "relative" value developed during parsing.
 *
 * Because parsed values are pushed on vectors, the base of the vector may change with any push.
 * Therefore, we must keep only relative indexes to the relevant items.
 * During post-parse processing these relative indexes will be converted to absolute pointers.
 * See \ref json_value_tag.
 */
typedef struct {
    aint uiId; ///< The value identifier, JSON_ID_OBJECT, etc.
    aint uiKey; ///< If uiID is JSON_ID_OBJECT, offset to the key's string. Otherwise, zero and not used.
    aint uiChildCount; ///< if uiId is JSON_ID_OBJECT or JSON_ID_ARRAY, the number of members or values.
    aint uiChildListOffset; ///< Offset from the base of the vector of child value pointers
    union{
        aint uiString; ///< \brief Offset to a string_r if JSON_ID_STRING.
        aint uiNumber; ///< \brief Offset to a number if JSON_ID_NUMBER.
    }; ///< \brief Space-saving union &ndash; only one of this is needed at a time, depending on the id.
} value_r;

/** \struct frame
 * \brief Each value is a node in the parse tree.
 *
 * A stack of these frames keeps track of the current value being parsed.
 * When the value is complete, the value remains but the frame is popped from the stack to point to the parent value.
 */
typedef struct {
    aint uiNextKey; ///< Used to keep track of the next available key offset to be used by an object member.
    aint uiValue; /**< \brief Index to the value represented by this frame. */
    aint uiString; /**< \brief Offset to the string or key string for this value. */
    void* vpVecIndexes; /** \brief A vector of child value indexes for objects and arrays. */
} frame;

/** \struct json
 * \brief The object context. For intenrnal use only.
 */
typedef struct {
    const void* vpValidate; ///< \brief Must be the "magic number" to be a valid context.
    exception* spException; ///< \brief Pointer to the exception structure
                            /// for reporting errors to the application catch block.
    void* vpMem; ///< \brief Pointer to a memory object used for all memory allocations.
//    const char* cpTraceOut; ///< The file name to write the internal APG parser's trace to. NULL (default) results in no trace.
    void* vpVecIterators; ///< \brief A vector of iterator context pointers remembered for destruction.
    void* vpVecBuilders; ///< \brief A vector of builder context pointers remembered for destruction.

    // the JSON input (a UTF-8 encoded byte string)
    void* vpVecInput; /**< \brief The UTF-8-encoded input byte stream. BOM, if any, removed. */
    void* vpLines; ///< \brief pointer to a `lines` object context

    // used and reused by the parser
    void* vpVecChars; /**< \brief A vector of string characters. 32-bit Unicode code points. All strings are in this single vector. */
    void* vpVecAscii; /**< \brief A scratch vector for constructing ASCII strings on the fly. */
    void* vpVecValuesr; /** \brief A vector of relative values. */
    void* vpVecValues;
    json_value* spValues; /**< \brief an array of absolute values. */
    aint uiValueCount; /**< \brief The number of values in the array. */
    void* vpVecStringsr; /** \brief A vector of relative strings. */
    void* vpVecStrings; ///< \brief A vector of Unicode strings.
    u32_phrase* spStrings; /**< \brief An array of absolute strings. */
    aint uiStringCount; /**< \brief The number of strings in the array. */
    void* vpVecNumbers; /** \brief A vector of number objects. */
    void* vpVecChildIndexes; /**< \brief A single vector of relative child indexes to values. */
    void* vpVecFrames; /**< \brief Frame stack of values to keep track of the current value in the parse tree. */
    void* vpVecChildPointers; //< \brief A vector of pointers to object and array children.
    struct json_value_tag** sppChildPointers; /**< \brief An array of absolute child value pointers. */

    // working values during the parse
    frame* spCurrentFrame; /**< \brief Points to the current stack frame. */
    uint32_t uiChar; /**< \brief A working value to hold the value of a single character.
                            Higher level rules will move it to vpVecChars. Gets overwritten by each Char rule. */
    abool bHasFrac; /**< \brief A working value signaling presence of fractional value for a number value. */
    abool bHasMinus; /**< \brief A working value signaling a minus sign for a number value. */

    // pointer lists for returning values to user
    void* vpVecTreeList; ///< \brief Vector of pointers to sub_tree tree. Value pointers are in the order of a depth-first traversal.
    void* vpVecChildList; ///< \brief Vector of pointers to the children of a parent value. NULL if the parent is not an object or array.
    void* vpVecKeyList; ///< \brief Vector of pointers to the list of valued found in a key search.
    void* vpVecScratch32; ///< \brief A vector of 32-bit integer scratch space.

    // iterator helpers
    aint uiWalkCount; ///< \brief An accumulator for counting sub-tree nodes and child nodes.

    // display and writer helpers
    FILE* spIn; /**< \brief File I/O handle for the input file.
    Maintained here so that it can be closed in the destructor if necessary */
    void* vpParser; ///< \brief Pointer to the parser context if exception thrown during parsing.
    achar* acpInput; ///< \brief Buffer to hold the input converted from uint8_t to achar units.
    void* vpFmt; ///< \brief Pointer to a hexdump-style formatter object.
    void* vpVecOutput; ///< \brief Vector of 32-bit code points for generating output of value tree to JSON-text.
    void* vpConv; ///< \brief Context pointer for a conversion object.
    abool bFirstNode; ///< \brief Set to true before each call to sJsonWrite() to prevent writing a key for the first node of a sub-tree.
    aint uiCurrentDepth; ///< \brief Used to keep track of the current tree depth for display of the tree of values.
    aint uiMaxDepth; ///< \brief The maximum tree depth of values to display
} json;

/** \struct json_iterator
 * \brief A JSON interator object context.
 */
typedef struct{
    const void* vpValidate; ///< \brief Must be the "magic number" to be a valid context.
    json* spJson; ///< \brief Pointer to the parent JSON object context.
    void* vpVec; ///< \brief Work vector.
    json_value** sppValues; ///< \brief List of pointers to values.
    aint uiCount; ///< \brief The number of pointers in the list
    aint uiCurrent; ///< \brief The current iterator value index
    aint uiContextIndex; ///< \brief
} json_iterator;

/** @name Private Functions
 * Private but required across source files.*/
///@{
void vJsonGrammarRuleCallbacks(void* vpParserCtx);
json_iterator* spJsonIteratorCtor(json* spJson);
#define JSON_UTF16_MATCH  0
#define JSON_UTF16_NOMATCH  1
#define JSON_UTF16_BAD_HIGH  2
#define JSON_UTF16_BAD_LOW  3
aint uiUtf16_1(char* cpHex, uint32_t* uipChar);
aint uiUtf16_2(char* cpHex, uint32_t* uipChar);
uint32_t uiUtf8_2byte(char* cpBytes);
uint32_t uiUtf8_3byte(char* cpBytes);
uint32_t uiUtf8_4byte(char* cpBytes);
///@}

#endif /* JSONP_H_ */
