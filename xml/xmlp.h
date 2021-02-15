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
/** \file xml/xmlp.h
 * \brief Private header for the APG XML parser's component context. Not needed by application programs.
 */
#ifndef APG_XMLP_H_
#define APG_XMLP_H_

/** \struct cdata_id
 * \brief Parsed character data definition.
 *
 * All parsed, 32-bit unicode code point data is pushed into the vecto vpVec32.
 * This structure defines the offset and length of a given parsed datum.
 */
typedef struct {
    uint32_t uiOffset; /**< \brief The offset into vpVec32 array for the start of the data. */
    uint32_t uiLength; /**< \brief The number of 32-bit data characters. */
} cdata_id;

/** \struct named_value
 * \brief Provides offsets and lengths in the vpVec32 array for a name and value pair.
 */
typedef struct{
    cdata_id sName; ///< \brief Points to offset and length of the datum's name.
    cdata_id sValue; ///< \brief Points to offset and length of the datum's value.
} named_value;

/** \struct att_decl
 * \brief Identifies the element name, attribute name and default attribute value of attribute list declarations.
 *
 * All offsets and lengths refer to the vpVecEntities vector of 32-bit character code points.
 */
typedef struct{
    struct xml_tag* spXml; ///< \brief A copy of the XML object context pointer. Needed for quick sort and quick look up.
    cdata_id sElementName; ///< \brief The element name, offset and length.
    cdata_id sAttName; ///< \brief The attribute name, offset and length.
    cdata_id sAttType; ///< \brief attribute type, offset and length.
    cdata_id sAttValue; ///< \brief The attribute normalized value, offset and length.
    aint uiAttCount; ///< \brief The number of different attribute names associated with this element name.
    abool bIsCDATA; ///< \brief True if the attribute typ is CDATA
    abool bHasData; ///< \brief True if the attribute declaration defines default data.
    abool bInvalidValue; ///< \brief True if the attribute value is invalid.
} att_decl;

/** \struct entity_decl
 * \brief Provides the offset into the general 32-bit vector and length of a name and value pair.
 *
 * Both the name and value are strings of 32-bit integers. That is, Unicode code points.
 * A single vector (vpVec32) holds all of the Unicode code points. Individual datum are simply
 * demarked by their offset and length. Used primarily for General Entities
 * but also for Notation Declarations.
 */
typedef struct {
    struct xml_tag* spXml; ///< \brief A copy of the XML object context pointer. Needed for quick sort and quick look up.
    cdata_id sName; ///< \brief The offset (into vpVec32) and length of the name.
    cdata_id sValue; ///< \brief The offset (into vpVec32) and length of the value
    aint uiInputOffset; ///< \brief Offset to the first input character of the "<!ENTITY" declaration.
    abool bGEPERef; ///< \brief True if the General Entity declaration contains a Parameter Entity.
    abool bGEDefEx; ///< \brief True if the General Entity declaration contains an external ID.
    abool bEntityDeclaredError; ///< \brief True if the General Entity declaration contains a logged error.
    abool bExpanded; ///< \brief True if this entity value has been expanded.
} entity_decl;

/** \struct element_frame
 * \brief A stack is used to track which element is currently being parsed.
 * This frame struct contains all of the information needed to track the data.
 */
typedef struct {
    aint uiElementOffset; /**< \brief Parser offset to beginning of this element. For error reporting. */
    aint uiAttributeOffset; /**< \brief Parser offset to beginning of the current attribute. For error reporting. */
    aint uiEntityOffset; /**< \brief Parser offset to beginning of the current entity reference. For error reporting. */
    aint uiBase32; /**< \brief Base index in vpVec32 for all data for this element. Pop to uiBase32 at end of element. */
    aint uiBaseAtt; /**< \brief Base index in vpVecAttList for this element. Pop to uiBaseAtt at end of element. */
    aint uiAttCount;/**< \brief The number of attributes found in the start tag. */
    cdata_id sSName; /**< \brief Identifies the location of the start tag name. */
    cdata_id sEName; /**< \brief Identifies the location of the end tag name. */
    cdata_id sContent; /**< \brief Identifies the location of the element's character data. */
} element_frame;

typedef struct{
    uint32_t uiNameOffset; ///< \brief This is a unique identifier for the name. Used to check for entities that refer to themselves indirectly.
} entity_frame;

/** \struct xml
 * \brief This is the encapsulated data for the xml component. The component context or handle is an opaque pointer to this data.
 */
typedef struct xml_tag{
    const void* vpValidate; /**< \brief Set to the context handle as a "magic number" for validating component member function calls. */
    exception* spException;
    void* vpMem; /**< \brief Handle to the memory component which keeps track of all memory allocations by this xml component*/
    void* vpFmt; ///< \brief Context pointer to a format object used to display printing and non-printing code points.
    void* vpParser; ///< \brief APG parser context pointer. Kept here for memory clean up use.
    void* vpConv; ///< \brief A UTF conversion component. Reused in several places.
    void* vpMsgs; /**< \brief During DTD parsing, this collects errors, all to be reported at the close of the DTD.
    During element processing (the body of the XML document) it is used to collect warning messages presented to the user
    at the end of the document. */
    void* vpLines; ///< \brief A lines object context pointer. Used to report the line number location of errors.

    // input
    aint uiTrueType; /**< \brief Used to record the actual data type of the XML input (UTF-8, UTF-16BE or UTF-16LE) */
    void* vpVecChars; /**< \brief Vector of the XML input characters */
    achar* acpChars; /**< \brief Pointer to input characters converted to achar, which in general is not 8-bit characters. */
    uint8_t* ucpData; ///< \brief Points to the data read from an input file, if any. Otherwise, NULL. Needed for vXmlDtor().

    // working memory for parsed values
    uint32_t uiChar; /**< \brief A working value for a single character. Gets overwritten by each Char rule. */
    void* vpVecName; /**< \brief Hold the name from the Name rule. Gets overwritten for each new Name instance. */
    void* vpVec32; /**< \brief A 32-bit work vector of all most parsed data, names, attributes and content. */
    void* vpVec8; /**< \brief A work vector for byte-stream data. */
    void* vpVecString; /**< \brief A work vector for representing ASCII-only character data as a null-terminated string. */
    void* vpVecCData; ///< \brief Temp vector of u32_phrase info for presentation to the caller.
    aint uiSavedOffset; ///< \brief Offset to the beginning of the declaration being processed. Used for error reporting.

    // XML declarations info
    xmldecl_info sXmlDecl; ///< \brief For collecting and reporting the XML declaration info.

    // DTD declarations info
    abool bStandalone; ///< \brief True if standalone = "yes", false if standalone = "no".
    abool bExtSubset; ///< \brief True if an external subset is found, False otherwise.
    aint uiDTDOffset; ///< \brief Parser offset to the beginning of the DTD. Used for error reporting.
    cdata_id sDtdName; ///< \brief Offset & length of the DTD name.
    void* vpVecGEDefs; ///< \brief Vector of named_values for the General Entities declared.
    void* vpVecEntityFrames; ///< \brief Vector of General Entities stack frames for walking the tree of entity replacement values.
    void* vpVecNotationDecls; ///< \brief Vector of Notations declared.
    aint uiExternalIds; ///< \brief The number of external IDs found.
    aint uiPEDecls; ///< \brief The number of Parameter Entity declarations found.
    aint uiPERefs; ///< \brief The number of Parameter Entity references found.
    aint uiGEDeclsTotal; /**< \brief The total number of General Entity declarations.
     This includes declarations with multiply-defined names and ignored due to existence of Parameter Entities. */
    aint uiGEDeclsNotProcessed; /**< \brief The number of General Entity declarations not processed because of conditionals.
        Condition: Unread Parameter Entity encountered previously and bStandalone is false (standalone = "no"). */
    aint uiElementDecls; ///< \brief The number of element declarations.
    entity_decl sCurrentEntity; ///< \brief Holds the current entity declaration information.
    ///< May be saved or discarded, depending on final processing.

    // element handling
    void* vpVecFrame; /**< \brief A vector for a stack of frames.
    A frame is pushed for each new element encountered and popped when the element ends. */
    element_frame* spCurrentFrame; /**< \brief Holds a pointer to the current frame for the element being parsed. */

    // attribute handling
    att_decl sCurrentAttList; ///< \brief Holds the current attribute list information.
    ///< Depending on its uniqueness and correctness it may be discarded or added to the list at the close of the declaration.
    void* vpVecAttWork; ///< \brief A vector of work space for normalization of attribute balues.
    void* vpVecAttDecls; /**< \brief The list of attribute declarations. */
    void* vpVecAttList; /**< \brief A vector of attribute named values. */
    aint uiAttListsNotProcessed; ///< \brief The number of Attribute List declarations not processed because of the PE conditionals.
    aint uiAttListsDeclared; ///< \brief The number of Attribute List declarations including empty and not processed.

    // user call functions
    pfnEmptyTagCallback pfnEmptyTagCallback; /**< \brief Pointer to the user's callback function for empty tag porcessing.
    May be the same as the start tag callback function or may be NULL for no callback. */
    void* vpEmptyTagData; ///< \brief An opaque pointer available for user's use.
    ///< It is presented to the callback function for the caller's use.
    pfnStartTagCallback pfnStartTagCallback; /**< \brief Pointer to the user's callback function for start tag processing.
    May be the same as the empty tag callback function or may be NULL for no callback. */
    void* vpStartTagData; ///< \brief An opaque pointer available for user's use.
    ///< It is presented to the callback function for the caller's use.
    pfnEndTagCallback pfnEndTagCallback; /**< \brief Pointer to the user's callback function for end tag processing. */
    void* vpEndTagData; ///< \brief An opaque pointer available for user's use.
    ///< It is presented to the callback function for the caller's use.
    pfnPICallback pfnPICallback; /**< \brief Pointer to the user's function for Processing Instruction processing. */
    void* vpPIData; ///< \brief An opaque pointer available for user's use.
    ///< It is presented to the callback function for the caller's use.
    pfnXmlDeclCallback pfnXmlDeclCallback; /**< \brief Pointer to the user's function for the XML declaration information. */
    void* vpXmlDeclData; ///< \brief An opaque pointer available for user's use.
    ///< It is presented to the callback function for the caller's use.
    pfnDTDCallback pfnDTDCallback; /**< \brief Pointer to the user's function for the DTD information. */
    void* vpCommentData; ///< \brief An opaque pointer available for user's use.
    ///< It is presented to the callback function for the caller's use.
    pfnCommentCallback pfnCommentCallback; /**< \brief Pointer to the user's function for the DTD information. */
    void* vpDTDData; ///< \brief An opaque pointer available for user's use.
} xml;


void vXmlgrammarRuleCallbacks(void* vpParser);
//void vXmlgrammarUdtCallbacks(void* vpParserCtx);

#endif /* APG_XMLP_H_ */
