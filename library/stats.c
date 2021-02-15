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
/** \file stats.c
 * \brief Functions for collecting parsing statistics.
 *
 * As the parser traverses the syntax tree, these functions collect detailed counts of the tree node hits.
 * Hit counts are kept for each individual operator node type and each hit type (\ref ID_MATCH, \ref ID_NOMATCH).
 * For the rule name (RNM) and User-Defined Terminal (UDT) nodes, the hit counts are further refined
 * by rule/UDT name.
 */

#include "./apg.h"
#ifdef APG_STATS
#include <stdio.h>
#include <time.h>

#include "./lib.h"
#include "./parserp.h"

static const void* s_vpMagicNumber = (void*)"stats";

/** \struct node_stat
 * \brief Holds the statistics for a single node.
 */
typedef struct {
    const char* cpName; ///< \brief The node name.
    aint uiHits; ///< \brief Total number of hits.
    aint uiMatch; ///< \brief Number of matched hits.
    aint uiNomatch; ///< \brief Number of not matched hits.
} node_stat;

/** \struct stats
 * \brief The totality of all node statistics.
 */
typedef struct {
    aint uiRuleCount; ///< \brief The number of rules in the SABNF grammar.
    aint uiUdtCount; ///< \brief The number of UDTs in the SABNF grammar.
    node_stat sAlt; ///< \brief The statistics for the ALT nodes, ets.
    node_stat sCat;
    node_stat sRep;
    node_stat sRnm;
    node_stat sTrg;
    node_stat sTls;
    node_stat sTbs;
    node_stat sUdt;
    node_stat sAnd;
    node_stat sNot;
    node_stat sBkr;
    node_stat sBka;
    node_stat sBkn;
    node_stat sAbg;
    node_stat sAen;
    node_stat sTotal; ///< \brief The total statistics for all node types.
    node_stat* spRuleStats; ///< \brief An array of node statistics for each rule name.
    node_stat* spUdtStats; ///< \brief  An array of node statistics for each UDT name.
} stats;

/** \struct stats_ctx
 * \brief The statistics object context.
 */
typedef struct {
    const void* vpValidate; ///< \brief The "magic number" indicating a valid context.
    parser* spParserCtx; ///< \brief Pointer to the parent parser's context.
    exception* spException; ///< \brief Pointer to the exception structure for
                            /// reporting fatal errors back to the parser's catch block scope.
    stats sStats; ///< \brief The totality of all node statistics.
} stats_ctx;

static char* s_cpPageHeader;
static char* s_cpPageFooter;
static char s_cHits = 'h';
static char s_cAlpha = 'a';
static int compareNames(const void* l, const void* r);
static int compareHits(const void* l, const void* r);

/** \brief The statistics object constructor.
 *
 * - Note 1. There is no corresponding destructor. This object
 * is destroyed by the parent parser's destructor.
 * - Note 2. There is no reset. If the parent parser is run multiple times the statistics
 * will accumulate for the total of all runs.
 * \param vpParserCtx Pointer to the parent parser's context.
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \return Pointer to the statistics object's context.
 */
void* vpStatsCtor(void* vpParserCtx) {
    if(!bParserValidate(vpParserCtx)){
        vExContext();
    }
    parser* spParserCtx = (parser*) vpParserCtx;
    stats_ctx* spCtx;
    aint ui;
    spCtx = (stats_ctx*) vpMemAlloc(spParserCtx->vpMem, (aint) sizeof(stats_ctx));
    memset((void*) spCtx, 0, sizeof(stats_ctx));
    spCtx->spParserCtx = spParserCtx;
    spCtx->spException = spMemException(spParserCtx->vpMem);
    spCtx->sStats.uiRuleCount = spParserCtx->uiRuleCount;
    spCtx->sStats.uiUdtCount = spParserCtx->uiUdtCount;
    spCtx->sStats.spRuleStats = (node_stat*) vpMemAlloc(spParserCtx->vpMem,
            (aint) (spParserCtx->uiRuleCount * (aint) sizeof(node_stat)));
    memset((void*) spCtx->sStats.spRuleStats, 0, ((size_t) spParserCtx->uiRuleCount * sizeof(node_stat)));
    for (ui = 0; ui < spParserCtx->uiRuleCount; ui += 1) {
        spCtx->sStats.spRuleStats[ui].cpName = spParserCtx->spRules[ui].cpRuleName;
    }
    if (spParserCtx->uiUdtCount) {
        spCtx->sStats.spUdtStats = (node_stat*) vpMemAlloc(spParserCtx->vpMem,
                (aint) (spParserCtx->uiUdtCount * (aint) sizeof(node_stat)));
        memset((void*) spCtx->sStats.spUdtStats, 0, ((size_t) spParserCtx->uiUdtCount * sizeof(node_stat)));
        for (ui = 0; ui < spParserCtx->uiUdtCount; ui += 1) {
            spCtx->sStats.spUdtStats[ui].cpName = spParserCtx->spUdts[ui].cpUdtName;
        }
    }

    // success
    spParserCtx->vpStats = (void*) spCtx;
    spCtx->vpValidate = s_vpMagicNumber;
    return (void*) spCtx;
}

/** \brief Collects the statistics for a single node hit.
 *
 * This function is called only by the parent parser via the macro \ref STATS_HIT.
 * \param vpCtx Pointer to a valid statistics context returned from vpStatsCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param spOp Pointer to the opcode of the current node.
 * \param uiState The node state (\ref ID_MATCH or \ref ID_NOMATCH)
 * \return void
 */
void vStatsHit(void* vpCtx, const opcode* spOp, aint uiState) {
    stats_ctx* spCtx = (stats_ctx*) vpCtx;
    stats* spStats = &spCtx->sStats;
    node_stat* spNodeStat = NULL;
    node_stat* spRuleStat = NULL;

    switch (spOp->sGen.uiId) {
    case ID_ALT:
        spNodeStat = &spStats->sAlt;
        break;
    case ID_CAT:
        spNodeStat = &spStats->sCat;
        break;
    case ID_REP:
        spNodeStat = &spStats->sRep;
        break;
    case ID_RNM:
        spNodeStat = &spStats->sRnm;
        spRuleStat = &spStats->spRuleStats[spOp->sRnm.spRule->uiRuleIndex];
        break;
    case ID_TRG:
        spNodeStat = &spStats->sTrg;
        break;
    case ID_TBS:
        spNodeStat = &spStats->sTbs;
        break;
    case ID_TLS:
        spNodeStat = &spStats->sTls;
        break;
    case ID_UDT:
        spNodeStat = &spStats->sUdt;
        spRuleStat = &spStats->spUdtStats[spOp->sUdt.spUdt->uiUdtIndex];
        break;
    case ID_AND:
        spNodeStat = &spStats->sAnd;
        break;
    case ID_NOT:
        spNodeStat = &spStats->sNot;
        break;
    case ID_BKR:
        spNodeStat = &spStats->sBkr;
        break;
    case ID_BKA:
        spNodeStat = &spStats->sBka;
        break;
    case ID_BKN:
        spNodeStat = &spStats->sBkn;
        break;
    case ID_ABG:
        spNodeStat = &spStats->sAbg;
        break;
    case ID_AEN:
        spNodeStat = &spStats->sAen;
        break;
    default:
        XTHROW(spCtx->spException, "unrecognized operator ID");
        break;
    }
    spNodeStat->uiHits++;
    if (uiState == ID_MATCH) {
        spNodeStat->uiMatch++;
    } else {
        spNodeStat->uiNomatch++;
    }
    spNodeStat = &spStats->sTotal;
    spNodeStat->uiHits++;
    if (uiState == ID_MATCH) {
        spNodeStat->uiMatch++;
    } else {
        spNodeStat->uiNomatch++;
    }
    if (spRuleStat) {
        spRuleStat->uiHits++;
        if (uiState == ID_MATCH) {
            spRuleStat->uiMatch++;
        } else {
            spRuleStat->uiNomatch++;
        }
    }
}

/** \brief Generates an HTML page displaying the node hit statistics.
 * \param vpCtx Pointer to a valid statistics context returned from vpStatsCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpMode Name of the display mode.
 * - "alphabetical" (or any string beginning with "a" or "A"), rule/udt nodes arranged alphabetically
 * - "hit count" (or any string beginning with "h" or "H"), rule/udt nodes arranged descending on hit count
 * - defaults to "hit count" if mode is NULL or unrecognized as either of above.
 * \param cpFileName Name of the file to write the HTML page to. If NULL, writes to stdout.
 */
void vStatsToHtml(void* vpCtx, const char* cpMode, const char* cpFileName) {
    stats_ctx* spCtx = (stats_ctx*) vpCtx;
    if(spCtx->vpValidate != s_vpMagicNumber){
        vExContext();
        return; // should never return
    }
    stats* spStats = &spCtx->sStats;
    node_stat* spNode;
    FILE* spOut = stdout;
    char* cpModeName;
    char cMode;
    aint ui;

    // determine the mode
    cMode = s_cHits;
    cpModeName = "hit count"; // default to hit count mode
    if(cpMode){
        if (*cpMode == 'a' || *cpMode == 'A') {
            cpModeName = "alphabetical";
            cMode = s_cAlpha;
        }
    }
    // open the output file (default is stdout)
    if (cpFileName) {
        spOut = fopen(cpFileName, "wb");
        if(!spOut){
            XTHROW(spCtx->spException, "stats to HTML unable to open output file");
        }
    }
    // output the page header
    fprintf(spOut, "%s", s_cpPageHeader);
    fprintf(spOut, "<h3>Node Statistics<h3/>\n");

    // open the operators table
    fprintf(spOut, "%s", "<table class=\"apg-stats\">\n");
    fprintf(spOut, "<caption>Operators<caption/>\n");
    fprintf(spOut, "<tr><th>name</th><th>hits</th><th>match</th><th>no match</th><tr>\n");
    spNode = &spStats->sAlt;
    fprintf(spOut,
            "<tr><td><span class=\"apg-remainder\">non-<br>terminals</span></td><td></td><td></td><td></td><tr>\n");
    fprintf(spOut, "<tr><td>ALT</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sCat;
    fprintf(spOut, "<tr><td>CAT</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sRep;
    fprintf(spOut, "<tr><td>REP</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sRnm;
    fprintf(spOut, "<tr><td>RNM</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sAnd;
    fprintf(spOut, "<tr><td>AND</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sNot;
    fprintf(spOut, "<tr><td>NOT</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sBka;
    fprintf(spOut, "<tr><td>BKA</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sBkn;
    fprintf(spOut, "<tr><td>BKN</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    fprintf(spOut, "<tr><td><span class=\"apg-remainder\">terminals</span></td><td></td><td></td><td></td><tr>\n");
    spNode = &spStats->sTls;
    fprintf(spOut, "<tr><td>TLS</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sTbs;
    fprintf(spOut, "<tr><td>TBS</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sTrg;
    fprintf(spOut, "<tr><td>TRG</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sUdt;
    fprintf(spOut, "<tr><td>UDT</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sBkr;
    fprintf(spOut, "<tr><td>BKR</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sAbg;
    fprintf(spOut, "<tr><td>ABG</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sAen;
    fprintf(spOut, "<tr><td>AEN</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
    spNode = &spStats->sTotal;
    fprintf(spOut, "<tr><td>total</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch);

    // close the operators table
    fprintf(spOut, "%s", "</table>\n");

    // sort the rules
    node_stat sRuleStats[spStats->uiRuleCount];
    for (ui = 0; ui < spStats->uiRuleCount; ui += 1) {
        sRuleStats[ui] = spStats->spRuleStats[ui];
    }
    qsort((void*) sRuleStats, (size_t) spStats->uiRuleCount, sizeof(node_stat), compareNames);
    if (cMode == s_cHits) {
        qsort((void*) sRuleStats, (size_t) spStats->uiRuleCount, sizeof(node_stat), compareHits);
    }
    // open the rules table
    fprintf(spOut, "<br/>\n");
    fprintf(spOut, "%s", "<table class=\"apg-stats\">\n");
    fprintf(spOut, "<caption>Rules: %s<caption/>\n", cpModeName);
    fprintf(spOut, "<tr><th>name</th><th>hits</th><th>match</th><th>no match</th><tr>\n");
    for (ui = 0; ui < spStats->uiRuleCount; ui += 1) {
        spNode = &sRuleStats[ui];
        if (spNode->uiHits) {
            fprintf(spOut, "<tr><td>%s</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", spNode->cpName,
                    (luint) spNode->uiHits, (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
        }
    }

    // close the rules table
    fprintf(spOut, "%s", "</table>\n");

    if (spStats->uiUdtCount) {
        // sort the udts
        node_stat sUdtStats[spStats->uiUdtCount];
        for (ui = 0; ui < spStats->uiUdtCount; ui += 1) {
            sUdtStats[ui] = spStats->spUdtStats[ui];
        }
        qsort((void*) sUdtStats, (size_t) spStats->uiUdtCount, sizeof(node_stat), compareNames);
        if (cMode == s_cHits) {
            qsort((void*) sUdtStats, (size_t) spStats->uiUdtCount, sizeof(node_stat), compareHits);
        }
        // open the UDT table
        fprintf(spOut, "<br/>\n");
        fprintf(spOut, "%s", "<table class=\"apg-stats\">\n");
        fprintf(spOut, "<caption>UDTs: %s<caption/>\n", cpModeName);
        fprintf(spOut, "<tr><th>name</th><th>hits</th><th>match</th><th>no match</th><tr>\n");
        for (ui = 0; ui < spStats->uiUdtCount; ui += 1) {
            spNode = &sUdtStats[ui];
            if (spNode->uiHits) {
                fprintf(spOut, "<tr><td>%s</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><td>%"PRIuMAX"</td><tr>\n", spNode->cpName,
                        (luint) spNode->uiHits, (luint) spNode->uiMatch, (luint) spNode->uiNomatch);
            }
        }

        // close the UDT table
        fprintf(spOut, "%s", "</table>\n");

    }

    // output the page footer
    time_t tTime = time(NULL);
    if (tTime != (time_t)-1) {
        fprintf(spOut, "<h5>%s</h5>\n", asctime(gmtime(&tTime)));
    }
    fprintf(spOut, "%s", s_cpPageFooter);
    fflush(spOut);
    if (spOut != stdout) {
        fclose(spOut);
    }
}

/** \brief Display the statistics in ASCII format.
 * \param vpCtx Pointer to a valid statistics context returned from vpStatsCtor()
 * If not valid the application will silently exit with a \ref BAD_CONTEXT exit code.
 * \param cpMode Name of the display mode.
 * - "alphabetical" (or any string beginning with "a" or "A"), rule/udt nodes arranged alphabetically
 * - "hit count" (or any string beginning with "h" or "H"), rule/udt nodes arranged descending on hit count
 * - defaults to "hit count" if mode is NULL or unrecognized as either of above.
 * \param cpFileName Name of the file to write the HTML page to. If NULL, writes to stdout.
 */
void vStatsToAscii(void* vpCtx, const char* cpMode, const char* cpFileName) {
    stats_ctx* spCtx = (stats_ctx*) vpCtx;
    if(spCtx->vpValidate != s_vpMagicNumber){
        vExContext();
        return; // should never return
    }
    stats* spStats = &spCtx->sStats;
    node_stat* spNode;
    FILE* spOut = stdout;
    char* cpModeName;
    char cMode;
    aint ui;

    // determine the mode
    cMode = s_cHits;
    cpModeName = "hit count"; // default to hit count mode
    if(cpMode){
        if (*cpMode == 'a' || *cpMode == 'A') {
            cpModeName = "alphabetical";
            cMode = s_cAlpha;
        }
    }
    // open the output file (default is stdout)
    if (cpFileName) {
        spOut = fopen(cpFileName, "wb");
        if(!spOut){
            XTHROW(spCtx->spException, "stats to ASCII, can't open output file");
        }
    }
    fprintf(spOut, "NODE STATISTICS\n");

    // open the operators table
    fprintf(spOut, "Operators: non-terminals\n");
    spNode = &spStats->sAlt;
    char* cpFormats = "| %7s | %7s | %7s | %s\n";
    char* cpFormat = "| %7"PRIuMAX" | %7"PRIuMAX" | %7"PRIuMAX" | %s\n";
    fprintf(spOut, cpFormats, "hits", "match", "nomatch", "name");
    spNode = &spStats->sAlt;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "ALT");
    spNode = &spStats->sCat;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "CAT");
    spNode = &spStats->sRep;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "REP");
    spNode = &spStats->sRnm;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "RNM");
    spNode = &spStats->sAnd;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "AND");
    spNode = &spStats->sNot;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "NOT");
    spNode = &spStats->sBka;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "BKA");
    spNode = &spStats->sBkn;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "BKN");

    fprintf(spOut, "\n");
    fprintf(spOut, "Operators: terminals\n");
    fprintf(spOut, cpFormats, "hits", "match", "nomatch", "name");
    spNode = &spStats->sTls;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "TLS");
    spNode = &spStats->sTbs;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "TBS");
    spNode = &spStats->sTrg;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "TRG");
    spNode = &spStats->sUdt;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "UDT");
    spNode = &spStats->sBkr;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "BKR");
    spNode = &spStats->sAbg;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "ABG");
    spNode = &spStats->sAen;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "AEN");

    fprintf(spOut, "\n");
    fprintf(spOut, "Operators: total\n");
    spNode = &spStats->sTotal;
    fprintf(spOut, cpFormat, (luint) spNode->uiHits,
            (luint) spNode->uiMatch, (luint) spNode->uiNomatch, "TOTAL");

    // sort the rules
    qsort((void*) spStats->spRuleStats, (size_t) spStats->uiRuleCount, sizeof(node_stat), compareNames);
    if (cMode == s_cHits) {
        qsort((void*) spStats->spRuleStats, (size_t) spStats->uiRuleCount, sizeof(node_stat), compareHits);
    }
    // open the rules table
    fprintf(spOut, "\n");
    fprintf(spOut, "Rules: %s\n", cpModeName);
    fprintf(spOut, cpFormats, "hits", "match", "nomatch", "name");
    for (ui = 0; ui < spStats->uiRuleCount; ui += 1) {
        spNode = &spStats->spRuleStats[ui];
        if (spNode->uiHits) {
            fprintf(spOut, cpFormat, (luint) spNode->uiHits, (luint) spNode->uiMatch, (luint) spNode->uiNomatch, spNode->cpName);
        }
    }

    if (spStats->uiUdtCount) {
        // sort the udts
        qsort((void*) spStats->spUdtStats, (size_t) spStats->uiUdtCount, sizeof(node_stat), compareNames);
        if (cMode == s_cHits) {
            qsort((void*) spStats->spUdtStats, (size_t) spStats->uiUdtCount, sizeof(node_stat), compareHits);
        }
        // open the UDT table
        fprintf(spOut, "\n");
        fprintf(spOut, "UDTs: %s\n", cpModeName);
        fprintf(spOut, cpFormats, "hits", "match", "nomatch", "name");
        for (ui = 0; ui < spStats->uiUdtCount; ui += 1) {
            spNode = &spStats->spUdtStats[ui];
            if (spNode->uiHits) {
                fprintf(spOut, cpFormat,
                        (luint) spNode->uiHits, (luint) spNode->uiMatch, (luint) spNode->uiNomatch, spNode->cpName);
            }
        }
    }
    time_t tTime = time(NULL);
    if (tTime != (time_t)-1) {
        fprintf(spOut, "\n");
        fprintf(spOut, "%s\n", asctime(gmtime(&tTime)));
    }
    if (spOut != stdout) {
        fclose(spOut);
    }
}

static int compareNames(const void* l, const void* r) {
    node_stat* spL = (node_stat*) l;
    node_stat* spR = (node_stat*) r;
    aint uiLenL = (aint)strlen(spL->cpName);
    aint uiLenR = (aint)strlen(spR->cpName);
    aint uiLen = uiLenL < uiLenR ? uiLenL : uiLenR;
    aint ui;
    char cL, cR;
    for(ui = 0; ui < uiLen; ui++){
        cL = spL->cpName[ui];
        cR = spR->cpName[ui];
        if (cL >= 65 && cL <= 90) {
            cL += 32;
        }
        if (cR >= 65 && cR <= 90) {
            cR += 32;
        }
        if (cL < cR) {
            return -1;
        }
        if (cL > cR) {
            return 1;
        }
    }
    if(uiLenL < uiLenR){
        return -1;
    }
    if(uiLenL > uiLenR){
        return 1;
    }
    return 0;
}
static int compareHits(const void* l, const void* r) {
    node_stat* spL = (node_stat*) l;
    node_stat* spR = (node_stat*) r;
    if (spL->uiHits < spR->uiHits) {
        return 1;
    }
    if (spL->uiHits > spR->uiHits) {
        return -1;
    }
    return 0;
}

static char* s_cpPageHeader = "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<title>stats</title>\n"
        "<style>\n"
        ".apg-mono {\n"
        "  font-family: monospace;\n"
        "}\n"
        ".apg-active {\n"
        "  font-weight: bold;\n"
        "  color: #000000;\n"
        "}\n"
        ".apg-match {\n"
        "  font-weight: bold;\n"
        "  color: #264BFF;\n"
        "}\n"
        ".apg-empty {\n"
        "  font-weight: bold;\n"
        "  color: #0fbd0f;\n"
        "}\n"
        ".apg-nomatch {\n"
        "  font-weight: bold;\n"
        "  color: #FF4000;\n"
        "}\n"
        ".apg-lh-match {\n"
        "  font-weight: bold;\n"
        "  color: #1A97BA;\n"
        "}\n"
        ".apg-lb-match {\n"
        "  font-weight: bold;\n"
        "  color: #5F1687;\n"
        "}\n"
        ".apg-remainder {\n"
        "  font-weight: bold;\n"
        "  color: #999999;\n"
        "}\n"
        ".apg-ctrl-char {\n"
        "  font-weight: bolder;\n"
        "  font-style: italic;\n"
        "  font-size: .6em;\n"
        "}\n"
        ".apg-line-end {\n"
        "  font-weight: bold;\n"
        "  color: #000000;\n"
        "}\n"
        ".apg-error {\n"
        "  font-weight: bold;\n"
        "  color: #FF4000;\n"
        "}\n"
        ".apg-phrase {\n"
        "  color: #000000;\n"
        "  background-color: #8caae6;\n"
        "}\n"
        ".apg-empty-phrase {\n"
        "  color: #0fbd0f;\n"
        "}\n"
        "table.apg-state {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: left;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-state th,\n"
        "table.apg-state td {\n"
        "  text-align: left;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-state th:nth-last-child(2),\n"
        "table.apg-state td:nth-last-child(2) {\n"
        "  text-align: right;\n"
        "}\n"
        "table.apg-state caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-stats {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-stats th,\n"
        "table.apg-stats td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-stats caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-trace {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-trace caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-trace th,\n"
        "table.apg-trace td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-trace th:last-child,\n"
        "table.apg-trace th:nth-last-child(2),\n"
        "table.apg-trace td:last-child,\n"
        "table.apg-trace td:nth-last-child(2) {\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-grammar {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-grammar caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-grammar th,\n"
        "table.apg-grammar td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-grammar th:last-child,\n"
        "table.apg-grammar td:last-child {\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-rules {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-rules caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-rules th,\n"
        "table.apg-rules td {\n"
        "  text-align: right;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-rules a {\n"
        "  color: #003399 !important;\n"
        "}\n"
        "table.apg-rules a:hover {\n"
        "  color: #8caae6 !important;\n"
        "}\n"
        "table.apg-attrs {\n"
        "  font-family: monospace;\n"
        "  margin-top: 5px;\n"
        "  font-size: 11px;\n"
        "  line-height: 130%;\n"
        "  text-align: center;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-attrs caption {\n"
        "  font-size: 125%;\n"
        "  line-height: 130%;\n"
        "  font-weight: bold;\n"
        "  text-align: left;\n"
        "}\n"
        "table.apg-attrs th,\n"
        "table.apg-attrs td {\n"
        "  text-align: center;\n"
        "  border: 1px solid black;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "table.apg-attrs th:nth-child(1),\n"
        "table.apg-attrs th:nth-child(2),\n"
        "table.apg-attrs th:nth-child(3) {\n"
        "  text-align: right;\n"
        "}\n"
        "table.apg-attrs td:nth-child(1),\n"
        "table.apg-attrs td:nth-child(2),\n"
        "table.apg-attrs td:nth-child(3) {\n"
        "  text-align: right;\n"
        "}\n"
        "table.apg-attrs a {\n"
        "  color: #003399 !important;\n"
        "}\n"
        "table.apg-attrs a:hover {\n"
        "  color: #8caae6 !important;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n";
static char* s_cpPageFooter = "</body>\n"
        "</html>\n";

#endif /* APG_STATS */
