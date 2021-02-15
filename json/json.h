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
#ifndef _JSON_H_
#define _JSON_H_

/** \dir ,/json
 * \brief The JSON parser and builder..
 */


/** \file json.h
 * \brief Header file for the JSON component. Defines API prototypes.
 *
 */
#include "../utilities/utilities.h"

/** @name Value Identifiers
 *  Integer identifiers for each of the JSON value types.
 */
///@{
#define JSON_ID_OBJECT          11 ///< \brief Object value
#define JSON_ID_ARRAY           13 ///< \brief Array value
#define JSON_ID_STRING          14 ///< \brief String value
#define JSON_ID_NUMBER          15 ///< \brief Number value
#define JSON_ID_TRUE            16 ///< \brief Literal value is true
#define JSON_ID_FALSE           17 ///< \brief Literal value is false
#define JSON_ID_NULL            18 ///< \brief Literal value is null
///@}

/** @name Number Type Identifiers
 *  APG specifies numbers as one of three types &ndash; floating point, unsigned integer and signed integer.
 *  These identifiers are used to indicate which.
 */
///@{
#define JSON_ID_FLOAT           19 ///< \brief Number value is a double floating point number
#define JSON_ID_SIGNED          20 ///< \brief Number value is a 64-bit signed integer
#define JSON_ID_UNSIGNED        21 ///< \brief Number value is a 64-bit unsigned integer
///@}

/** \struct json_number
 * \brief The structure of a JSON number value.
 *
 * The `json` object differentiates between three different number types &ndash;
 * floating point, signed and unsigned integer values.
 */
typedef struct {
    aint uiType; /**< \brief Identifies the number type. One of
 *  - JSON_ID_FLOAT
 *  - JSON_ID_SIGNED
 *  - JSON_ID_UNSIGNED
 */
    union {
        double dFloat; /**< \brief If uiType = JSON_ID_FLOAT, the floating point value. */
        uint64_t uiUnsigned; /**< \brief If uiType = JSON_ID_UNSIGNED, the unsigned int value. */
        int64_t iSigned; /**< \brief If uiType = JSON_ID_SIGNED, the signed int value. */
    };    /**< \brief double floating point, unsigned integer, signed integer
     *
     * Only one is needed at a time from this space-saving union.
     */

} json_number;

/** \struct json_value_tag
 * \brief The structure of a JSON value.
 *
 * The `json` specification defines seven different value types &ndash;
 * object, array, string, number, true, false and null.
 * This APG JSON parser equalizes the handling of them all with this structure/union.
 * Any one of these value types can be represented with this structure,
 * including object members, which are treated as values with the addition of a key string.
 */
typedef struct json_value_tag {
    aint uiId; /**< \brief The type of value. One of
     - JSON_ID_OBJECT
     - JSON_ID_ARRAY
     - JSON_ID_STRING
     - JSON_ID_NUMBER
     - JSON_ID_TRUE
     - JSON_ID_FALSE
     - JSON_ID_NULL, */
    u32_phrase* spKey; /**< \brief Points to the associated key string if this is a member of a JSON object. Otherwise, NULL. */
    union{
        u32_phrase* spString; ///< \brief Pointer to the string value if uiId = JSON_ID_STRING
        json_number* spNumber; ///< \brief Pointer to the number value if uiId = JSON_ID_NUMBER
        struct{
            struct json_value_tag** sppChildren; /**< \brief Points to a list of child value pointers if uiId is JSON_ID_OBJECT or JSON_ID_ARRAY. */

            aint uiChildCount; /**< \brief The number of child values if uiId is JSON_ID_OBJECT or JSON_ID_ARRAY. */
        };
    };
} json_value;

// constructor/destructor
/** @name Construction and Destruction*/
///@{
void* vpJsonCtor(exception* spEx);
void vJsonDtor(void *vpCtx);
abool bJsonValidate(void* vpCtx);
///@}

/** @name Read and Write
 * Reads and writes UTF-8 byte streams.
 * JSON files are parsed as they are read.
 * JSON files are converted to UTF-8 as they are written.
 * */
///@{
void* vpJsonReadFile(void *vpCtx, const char *cpFileName);
void* vpJsonReadArray(void *vpCtx, uint8_t *ucpData, aint uiDataLen);
uint8_t* ucpJsonWrite(void* vpCtx, json_value* spValue, aint* uipCount);
///@}

/** @name Accessing JSON Tree Nodes
 * Find trees, sub-trees, node siblings and specified key names.
 */
///@{
void* vpJsonFindKeyA(void *vpCtx, const char *cpKey, json_value* spValue);
void* vpJsonFindKeyU(void *vpCtx, const uint32_t *uipKey, aint uiLength, json_value* spValue);
void* vpJsonTree(void* vpCtx, json_value* spValue);
void* vpJsonChildren(void* vpCtx, json_value* spValue);
///@}

/** @name Value Iterators
 * \anchor anchor_json_iterator*/
///@{
void vJsonIteratorDtor(void* vpIteratorCtx);
json_value* spJsonIteratorFirst(void* vpIteratorCtx);
json_value* spJsonIteratorLast(void* vpIteratorCtx);
json_value* spJsonIteratorNext(void* vpIteratorCtx);
json_value* spJsonIteratorPrev(void* vpIteratorCtx);
aint uiJsonIteratorCount(void* vpIteratorCtx);
///@}

/** @name JSON Builder
 * For simple ASCII characters, one can easily just type a JSON file into a word processor.
 * The syntax is that simple.
 * However, for general 32- or even 64-bit alphabet characters this is not possible.
 * This suite of functions &ndash; implemented as a sub-object &ndash;
 * can build Unicode strings and arbitrary-width numbers into JSON
 */
///@{
void* vpJsonBuildCtor(void* vpJsonCtx);
void vJsonBuildDtor(void* vpBuildCtx);
void vJsonBuildClear(void* vpBuildCtx);
aint uiJsonBuildMakeStringA(void* vpBuildCtx, const char* cpString);
aint uiJsonBuildMakeStringU(void* vpBuildCtx, const uint32_t* uipData, aint uiLength);
aint uiJsonBuildMakeNumberF(void* vpBuildCtx, double dNumber);
aint uiJsonBuildMakeNumberS(void* vpBuildCtx, int64_t iNumber);
aint uiJsonBuildMakeNumberU(void* vpBuildCtx, uint64_t uiNumber);
aint uiJsonBuildMakeTrue(void* vpBuildCtx);
aint uiJsonBuildMakeFalse(void* vpBuildCtx);
aint uiJsonBuildMakeNull(void* vpBuildCtx);
aint uiJsonBuildMakeObject(void* vpBuildCtx);
aint uiJsonBuildMakeArray(void* vpBuildCtx);
aint uiJsonBuildAddToObject(void* vpBuildCtx, aint uiObject, aint uiKey, aint uiAdd);
aint uiJsonBuildAddToArray(void* vpBuildCtx, aint uiArray, aint uiAdd);
void* vpJsonBuild(void* vpBuildCtx, aint uiRoot);
///@}

/** @name Display Helpers */
///@{
void vJsonDisplayValue(void *vpCtx, json_value *spValue, aint uiDepth);
void vJsonDisplayInput(void *vpCtx, abool bShowLines);
//void vJsonSetTraceFile(void *vpCtx, const char *cpTraceOutput);
///@}
/**
\page json A JSON Parser
## Introduction

<i>"JavaScript Object Notation (JSON) is a lightweight, text-based, language-independent data interchange format."</i>
<sup>[[1]](https://tools.ietf.org/html/rfc8259)</sup>
The motivation for including a JSON parser in this application suite is the need for a convenient and
general means of transmitting input arrays of character codes to APG parsers.
APG parsers may require simple 7-bit ASCII characters, a 32-bit array of Unicode code points or in general,
an arbitrary array of 32-bit or even 64-bit integers.
JSON is a suitable format that encompasses all of these data types,
while providing for transmission as a simple byte stream in all cases.

Why not [XML](https://www.w3.org/TR/REC-xml/)?
XML is a well-developed system for the mark up of text data.
However, it is not designed for and is severely limited as mark up for an arbitrary array of
[ABNF](https://tools.ietf.org/html/rfc5234) alphabet character codes.
For one, it delivers only Unicode.
And, for some reason, it doesn't deliver complete Unicode.
It specifically forbids the ASCII control characters other than 0x09, 0x0A, 0x0D and 0x7F, even as escaped characters.
On the other hand, several, probably many,
important Internet standards are defined in ABNF that specifies a character set of “octets”.
That is, all bytes from 0-255. This data would require overlaying a secondary data conversion,
[base64](https://tools.ietf.org/html/rfc4648#section-5) for example, for transmission via XML

JSON, fills the needs of transmitting any and all types of data that an APG parser might require.
JSON strings carry all allowed Unicode code points, including all ASCII control characters.
If non-Unicode characters are required, JSON provides arrays of numbers.
Therefore, even an array of 64-bit integer character codes can be represented in JSON format and,
because the standard requires that JSON files be UTF-8 encoded,
they can be transmitted as a simple byte stream with no byte-order ambiguities.

With these points in mind, a general, [RFC8259](https://tools.ietf.org/html/rfc8259)-compliant
JSON parser has been developed and included in this application suite.
The full documentation can be found in json/json.h, json/json.c and json/builder.c

## Implementation
This JSON parser follows the [object](\ref objects) model of other APG component objects.
That is, a constructor generates a context &ndash; an opaque state structure &ndash;
and fatal errors are reported with a simple exception handling technique.
See vpJsonCtor() and vJsonDtor().

In addition, value iterators and JSON text builders are sub-objects. That is, their constructors
require a valid JSON object as parent. While they do have their own separate destructors
they share a memory object with their parent.
Therefore, the destruction of the parent object, vJsonDtor(),
will clean up all memory from all child iterators and builders as well.

### Design
From the [ABNF grammar](https://tools.ietf.org/html/rfc8259#page-5) for a JSON text it is apparent that
the grammar defines a tree of values.

\dot
digraph example {
  size = "5,5";
  ratio = "fill";
      node [fontsize=14];
      label="JSON ABNF as a tree of values.";
      root [ label="value"];
      alt [ label="alternation"];
      string [label="string"];
      number [label="number"];
      object [label="object"];
      array[label="array"];
      true[ label="true"];
      false [label="false"];
      null [label="null"];
      cat1 [label="concatenation"];
      cat2 [label="concatenation"];
      mv1 [ label="member\nvalue"];
      mv2 [ label="member\nvalue"];
      mv3 [ label="member\nvalue"];
      v1 [ label="value"];
      v2 [ label="value"];
      v3 [ label="value"];
      root->alt
      alt->{string number object array true false null}
      object->cat1
      array->cat2
      cat1->{mv1 mv2 mv3}
      cat2->{v1 v2 v3}
  }
  \enddot
In this APG implementation, all nodes of this tree,
except for the alternation and concatenations, which do not appear in the concrete syntax tree, are "values".
Values are defined sufficiently generally that one structure, \ref json_value_tag, describes fully any node of the tree.

The parser will return an iterator which will walk the tree from value to value in a
[depth-first](https://en.wikipedia.org/wiki/Tree_traversal#Depth-first_search_of_binary_tree) fashion.
Additionally, iterators can be generated which will walk any value node as the root of a sub-tree,
or walk horizontally across all siblings of an object or array parent value node.
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
<br>
*/

#endif /* _JSON_H_ */
