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
/** \dir ./xml
 * \brief The XML parser..
 */

/** \file xml/xml.h
 * \brief Public header file for the APG XML parser API..
 *
 * This file contains all of the public type definitions and the prototypes for the XML parser object.
 */

#ifndef APG_XML_H_
#define APG_XML_H_

#include <limits.h>
#include "../utilities/utilities.h"

/** \def DEFAULT_CALLBACK
 * \brief Indicator for a pre-defined, default callback function.
 *
 * For usage, see, for example, \ref vXmlSetXmlDeclCallback().
 */
#define DEFAULT_CALLBACK (void*)1

/** \struct xmldecl_info
 * \brief Information about the XML declaration.
 *
 * This structure is presented to the XML Declaration callback function, even if no XML declaration is present.
 */
typedef struct{
    const char* cpExists;       ///< \brief "yes" if the XML declaration exists, "no" otherwise.
    const char* cpVersion;      ///< \brief The value of version="1.ddd".
                                ///< Default is 1.0. Any other value is a fatal error.
    const char* cpEncoding;     ///< \brief If present must be UTF-8 or UTF-16.
                                ///< It is a fatal error if the data is not in the specified format.
                                ///< Note that UTF-16 data may be either UTF-16BE or UTF-LE. Either is acceptable.
    const char* cpStandalone;   ///< \brief The value of the standalone declaration.
} xmldecl_info;

/** \struct dtd_info
 * \brief Information about the Document Type Declaration.
 *
 * This information is passed to the DTD callback function, even if no DTD is present.
 * Only the General Entity definitions and default attribute list are used by the XML parser.
 * However, Notation and Element declarations are also noted.
 */
typedef struct{
    abool bExists; ///< \brief True if the DTD exists, false otherwise.
    abool bStandalone; ///< \brief True if standalone = "yes", false if standalone = "no".
    abool bExtSubset; ///< \brief True if an external subset is declared.
    aint uiExternalIds; ///< \brief The number of external IDs declared.
    aint uiPEDecls; ///< \brief The number of Parameter Entities declared.
    aint uiPERefs; ///< \brief The number of Parameter Entity references.
    aint uiGEDeclsDeclared; ///< \brief A count of ALL General Entities declared.
    aint uiGEDeclsUnique; ///< \brief A count of the unique and valid General Entities declared.
    ///< (Some may have been multiply declared.)
    aint uiGEDeclsNotProcessed; ///< \brief The number of General Entity declarations not processed because of condition:
    ///< unread Parameter Entity encountered previously and standalone = "no".
    aint uiAttListsDeclared; ///< \brief The number of ALL Attribute declarations.
    aint uiAttListsUnique; ///< \brief The number of unique and valid Attribute declarations.
    ///< (Some may have been multiply declared.)
    aint uiAttListsNotProcessed; ///< \brief The number of Attribute List declarations not processed because of above condition:
    ///< unread Parameter Entity encountered previously and standalone = "no".
    u32_phrase* spName; ///< \brief The DTD name (name of the root element).
    u32_phrase* spGENames; ///< \brief A list of (uiGEDeclsUnique) declared General Entity names, if any.
    u32_phrase* spGEValues; ///< \brief A list of (uiGEDeclsUnique) declared General Entity Declaration values, if any.
    u32_phrase* spAttElementNames; ///< \brief A list of (uiAttListsUnique) element names of declared attribute defaults.
    u32_phrase* spAttNames; ///< \brief A list of (uiAttListsUnique) names of declared attribute defaults.
    u32_phrase* spAttTypes; ///< \brief A list of (uiAttListsUnique) types of declared attribute defaults.
    u32_phrase* spAttValues; ///< \brief A list of (uiAttListsUnique) normalized values of declared attribute defaults.
    u32_phrase* spNotationNames; ///< \brief A list of the Notation names, if any.
    u32_phrase* spNotationValues; ///< \brief A list of the Notation values, if any.
    aint uiNotationDecls; ///< \brief The number of notation declarations found.
    aint uiElementDecls; ///< \brief The number of element declarations found.
} dtd_info;

/** \typedef pfnXmlDeclCallback
 * \brief Defines the function type that is called after parsing the XML declaration.
 *
 * This function is called whether the XML declaration exists or not.<br>
 * Note that all pointers are valid only for the duration of the called function.
 * Applications must copy data to their own storage space if needed beyond the scope of this function call.
 * \param spInfo The XML declaration information
 * \param vpUserData The user's private data, or NULL if none.
 * This private data pointer is set in the function \ref vXmlSetXmlDeclCallback.
 */
typedef void (*pfnXmlDeclCallback)(xmldecl_info* spInfo, void* vpUserData);

/** \typedef pfnDTDCallback
 * \brief Defines the function type that is called after parsing the Document Type Declaration (DTD).
 *
 * This function is called whether the DTD exists or not.<br>
 * Note that all pointers are valid only for the duration of the call.
 * Applications must copy data to their own memory space if needed beyond the scope of this function call.
 * \param spInfo The DTD information
 * \param vpUserData The user's private data, or NULL if none.
 * This private data pointer is set in the function \ref vXmlSetDTDCallback().
 */
typedef void (*pfnDTDCallback)(dtd_info* spInfo, void* vpUserData);

/** \typedef *pfnStartTagCallback
 * \brief Defines the function type that is called after an element's start tag has been found.
 *
 * Note that all pointers are valid only for the duration of the called function.
 * Applications must copy data to their own storage space if needed beyond the scope of this function call.
 * \param spName The attribute name.
 * \param spAttNames An array of the attribute names.
 * \param spAttValues An array of the attribute values.
 * \param uiAttCount The number of attributes in the array.
 * \param vpUserData The user's private data, or NULL if none.
 * This private data pointer is set in the function \ref vXmlSetStartTagCallback().
 */
typedef void (*pfnStartTagCallback)(u32_phrase* spName, u32_phrase* spAttNames,
        u32_phrase* spAttValues, uint32_t uiAttCount, void* vpUserData);

/** \typedef *pfnEmptyTagCallback
 * \brief Defines the function type that is called after an empty tag has been found.
 *
 * Note that this is in all respects similar to \ref pfnStartTagCallback.
 * The user may assign the same function to both of these pointers.
 * They are separated to give the user the opportunity to recognize when to expect a matching end tag and not.<br>
 * Note that all pointers are valid only for the duration of the called function.
 * Applications must copy data to their own storage space if needed beyond the scope of this function call.
 * \param spName The attribute name.
 * \param spAttNames An array of the attribute names.
 * \param spAttValues An array of the attribute values.
 * \param uiAttCount The number of attributes in the array.
 * \param vpUserData The user's private data, or NULL if none.
 * This private data pointer is set in the function \ref vXmlSetEmptyTagCallback.
 */
typedef void (*pfnEmptyTagCallback)(u32_phrase* spName, u32_phrase* spAttrNames,
        u32_phrase* spAttrValues, uint32_t uiAttrCount, void* vpUserData);

/** \typedef pfnEndTagCallback
 * \brief Defines the function type that is called after an element's end tag has been found.
 *
 * Note that all pointers are valid only for the duration of the called function.
 * Applications must copy data to their own storage space if needed beyond the scope of this function call.
 * \param spName The element name.
 * \param spContent The element character data.
 * \param vpUserData The user's private data, or NULL if none.
 * This private data pointer is set in the function \ref vXmlSetEndTagCallback.
 */
typedef void (*pfnEndTagCallback)(u32_phrase* spName, u32_phrase* spContent, void* vpUserData);

/** \typedef pfnPICallback
 * \brief Defines the function type that is called after a Processing Instruction has been found.
 *
 * Note that all pointers are valid only for the duration of the called function.
 * Applications must copy data to their own storage space if needed beyond the scope of this function call.
 * \param spTarget The Processing Instruction's target or name.
 * \param spInfo The Processing Instruction's instructions or information.
 * \param vpUserData The user's private data, or NULL if none.
 * This private data pointer is set in the function \ref vXmlSetPICallback.
 */
typedef void (*pfnPICallback)(u32_phrase* spTarget, u32_phrase* spInfo, void* vpUserData);

/** \typedef pfnCommentCallback
 * \brief Defines the function type that is called after a comment is found.
 *
 * Note that all pointers are valid only for the duration of the called function.
 * Applications must copy data to their own storage space if needed beyond the scope of this function call.
 * \param spComment The comment characters.
 * \param vpUserData The user's private data, or NULL if none.
 * This private data pointer is set in the function \ref vXmlSetCommentCallback.
 */
typedef void (*pfnCommentCallback)(u32_phrase* spComment, void* vpUserData);

// constructor/destructor
void* vpXmlCtor(exception* spEx);
void vXmlDtor(void* vpCtx);
abool bXmlValidate(void* vpCtx);

// the XML input
void vXmlGetFile(void* vpCtx, const char* cpFileName);
void vXmlGetArray(void* vpCtx, uint8_t* ucpData, aint uiDataLen);

// parsing
void vXmlParse(void* vpCtx);
void vXmlSetXmlDeclCallback(void* vpCtx, pfnXmlDeclCallback pfnCallback, void* vpUserData);
void vXmlSetDTDCallback(void* vpCtx, pfnDTDCallback pfnCallback, void* vpUserData);
void vXmlSetEmptyTagCallback(void* vpCtx, pfnEmptyTagCallback pfnCallback, void* vpUserData);
void vXmlSetStartTagCallback(void* vpCtx, pfnStartTagCallback pfnCallback, void* vpUserData);
void vXmlSetEndTagCallback(void* vpCtx, pfnEndTagCallback pfnCallback, void* vpUserData);
void vXmlSetPICallback(void* vpCtx, pfnPICallback pfnCallback, void* vpUserData);
void vXmlSetCommentCallback(void* vpCtx, pfnCommentCallback pfnCallback, void* vpUserData);

// display
void vXmlDisplayInput(void* vpCtx, abool bShowLines);
void vXmlDisplayMsgs(void* vpCtx);
void* vpXmlGetMsgs(void* vpCtx);

/** \page xml An XML Parser
 *
 * This library contains an API for creating, using and destroying a
 * [standards](https://www.w3.org/TR/REC-xml/)-compliant, non-validating XML parser.
 * This serves both as an example of a relatively complex APG-generated parser
 * and as a practical, useful, non-validating XML parser.
 * It will parse both the XML declaration and the Document Type Declaration (DTD) if present.
 * As a non-validating XML parser it will:
 * - check the document for well-formedness
 * - parse the XML declaration, if any
 * - parse the DTD internal subset, if any
 * - record any defined entity values
 * - record any defined default attribute names and (normalized) values
 * - recognize and honor defined entity and default attribute values while parsing the body of the XML document
 *
 * This is an event-based parser as opposed to a
 * [Document Object Model](https://en.wikipedia.org/wiki/Document_Object_Model) (DOM) type parser.
 * It provides an API that exposes parsed information at well-defined document events.
 * The document events are handled through user-written callback functions.
 * Be aware that data presented to the callback function is transient.
 * It is valid only for the duration of the call.
 * The application will need to make copies into it's own memory space of any data that needs to be retained for later use.
 * Events may be ignored simply by not providing a callback function for the event.
 * The document events are:
 * - the XML declaration information
 * - the DTD information
 * - the element name, attribute names and values in an empty element
 * - the element name, attribute names and values in a start tag
 * - the element name and content at the end tag
 * - the target name and instructions of a Processing Instruction
 * - the comment text of parsed comments
 *
 * Full documentation can be found in xml.h, xmlp.h and xml.c.
 * */

#endif /* APG_XML_H_ */
