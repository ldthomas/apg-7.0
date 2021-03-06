//
// This C-language parser header was generated by APG Version 7.0.
// User modifications invalidate the license agreement and may cause unpredictable results.
//
/*  *************************************************************************************
    Copyright (c) 2021, Lowell D. Thomas
    All rights reserved.

    This file was generated by and is part of APG Version 7.0.
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

#ifndef _SIP_0_H_
#define _SIP_0_H_

// rule ids
#define SIP_0_ABS_PATH 53
#define SIP_0_ABSOLUTEURI 50
#define SIP_0_ACCEPT 70
#define SIP_0_ACCEPT_ENCODING 77
#define SIP_0_ACCEPT_LANGUAGE 81
#define SIP_0_ACCEPT_PARAM 73
#define SIP_0_ACCEPT_RANGE 71
#define SIP_0_ACKM 6
#define SIP_0_ADDR_SPEC 119
#define SIP_0_AINFO 107
#define SIP_0_ALERT_INFO 84
#define SIP_0_ALERT_PARAM 85
#define SIP_0_ALGORITHM 187
#define SIP_0_ALLOW 86
#define SIP_0_ALPHA 303
#define SIP_0_ALPHANUM 269
#define SIP_0_AREA_SPECIFIER 247
#define SIP_0_AUTH_PARAM 102
#define SIP_0_AUTH_PARAM_NAME 103
#define SIP_0_AUTH_SCHEME 105
#define SIP_0_AUTHENTICATION_INFO 106
#define SIP_0_AUTHORITY 62
#define SIP_0_AUTHORIZATION 87
#define SIP_0_BASE_PHONE_NUMBER 243
#define SIP_0_BLANKSPACE 1
#define SIP_0_BYEM 8
#define SIP_0_C_P_EXPIRES 124
#define SIP_0_C_P_Q 123
#define SIP_0_CALL_ID 111
#define SIP_0_CALL_INFO 113
#define SIP_0_CALLID 112
#define SIP_0_CANCELM 9
#define SIP_0_CHALLENGE 176
#define SIP_0_CHAR 304
#define SIP_0_CNONCE 96
#define SIP_0_CNONCE_VALUE 97
#define SIP_0_CODINGS 79
#define SIP_0_COLON 291
#define SIP_0_COMMA 289
#define SIP_0_COMMENT 294
#define SIP_0_COMPOSITE_TYPE 143
#define SIP_0_CONTACT 116
#define SIP_0_CONTACT_EXTENSION 125
#define SIP_0_CONTACT_PARAM 117
#define SIP_0_CONTACT_PARAMS 122
#define SIP_0_CONTENT_CODING 80
#define SIP_0_CONTENT_DISPOSITION 127
#define SIP_0_CONTENT_ENCODING 133
#define SIP_0_CONTENT_LANGUAGE 134
#define SIP_0_CONTENT_LENGTH 138
#define SIP_0_CONTENT_TYPE 139
#define SIP_0_CR 305
#define SIP_0_CREDENTIALS 88
#define SIP_0_CRLF 306
#define SIP_0_CSEQ 152
#define SIP_0_CTEXT 295
#define SIP_0_D_NAME 121
#define SIP_0_DATE 153
#define SIP_0_DATE1 156
#define SIP_0_DELAY 211
#define SIP_0_DELTA_SECONDS 126
#define SIP_0_DIG_RESP 90
#define SIP_0_DIGEST_CLN 178
#define SIP_0_DIGEST_RESPONSE 89
#define SIP_0_DIGEST_URI 93
#define SIP_0_DIGEST_URI_VALUE 94
#define SIP_0_DIGIT 307
#define SIP_0_DISCRETE_TYPE 142
#define SIP_0_DISP_EXTENSION_TOKEN 132
#define SIP_0_DISP_PARAM 129
#define SIP_0_DISP_TYPE 128
#define SIP_0_DISPLAY_NAME 120
#define SIP_0_DOMAIN 181
#define SIP_0_DOMAINLABEL 26
#define SIP_0_DQUOTE 308
#define SIP_0_DRESPONSE 100
#define SIP_0_DTMF_DIGIT 265
#define SIP_0_ENCODING 78
#define SIP_0_EQUAL 284
#define SIP_0_ERROR_INFO 160
#define SIP_0_ERROR_URI 161
#define SIP_0_ESCAPED 273
#define SIP_0_EXPIRES 162
#define SIP_0_EXTENSION_HEADER 237
#define SIP_0_EXTENSION_METHOD 12
#define SIP_0_EXTENSION_TOKEN 144
#define SIP_0_FROM 163
#define SIP_0_FROM_PARAM 165
#define SIP_0_FROM_SPEC 164
#define SIP_0_FUTURE_EXTENSION 257
#define SIP_0_GEN_VALUE 76
#define SIP_0_GENERIC_PARAM 75
#define SIP_0_GLOBAL_NETWORK_PREFIX 251
#define SIP_0_GLOBAL_PHONE_NUMBER 242
#define SIP_0_HANDLING_PARAM 130
#define SIP_0_HCOLON 280
#define SIP_0_HEADER 45
#define SIP_0_HEADER_NAME 238
#define SIP_0_HEADER_VALUE 239
#define SIP_0_HEADERS 44
#define SIP_0_HEXDIG 309
#define SIP_0_HIER_PART 51
#define SIP_0_HNAME 46
#define SIP_0_HNV_UNRESERVED 48
#define SIP_0_HOST 24
#define SIP_0_HOSTNAME 25
#define SIP_0_HOSTPORT 23
#define SIP_0_HVALUE 47
#define SIP_0_IANA_TOKEN 148
#define SIP_0_IETF_TOKEN 145
#define SIP_0_IN_REPLY_TO 167
#define SIP_0_INFO 114
#define SIP_0_INFO_PARAM 115
#define SIP_0_INVITEM 5
#define SIP_0_IPV4ADDRESS 66
#define SIP_0_IPV6ADDRESS 68
#define SIP_0_IPV6REFERENCE 67
#define SIP_0_ISDN_SUBADDRESS 245
#define SIP_0_LANGUAGE 82
#define SIP_0_LANGUAGE_RANGE 83
#define SIP_0_LANGUAGE_TAG 135
#define SIP_0_LAQUOT 288
#define SIP_0_LDQUOT 292
#define SIP_0_LF 310
#define SIP_0_LHEX 281
#define SIP_0_LOCAL_NETWORK_PREFIX 252
#define SIP_0_LOCAL_PHONE_NUMBER 244
#define SIP_0_LPAREN 285
#define SIP_0_LR_PARAM 38
#define SIP_0_LWS 300
#define SIP_0_M_ATTRIBUTE 150
#define SIP_0_M_PARAMETER 149
#define SIP_0_M_SUBTYPE 147
#define SIP_0_M_TYPE 141
#define SIP_0_M_VALUE 151
#define SIP_0_MADDR_PARAM 37
#define SIP_0_MARK 272
#define SIP_0_MAX_FORWARDS 168
#define SIP_0_MEDIA_RANGE 72
#define SIP_0_MEDIA_TYPE 140
#define SIP_0_MESSAGE_BODY 240
#define SIP_0_MESSAGE_HEADER 69
#define SIP_0_MESSAGE_QOP 95
#define SIP_0_METHOD 11
#define SIP_0_METHOD_PARAM 35
#define SIP_0_MIME_VERSION 169
#define SIP_0_MIN_EXPIRES 170
#define SIP_0_MONTH 159
#define SIP_0_NAME_ADDR 118
#define SIP_0_NC_VALUE 99
#define SIP_0_NET_PATH 52
#define SIP_0_NETWORK_PREFIX 250
#define SIP_0_NEXTNONCE 108
#define SIP_0_NONCE 183
#define SIP_0_NONCE_COUNT 98
#define SIP_0_NONCE_VALUE 184
#define SIP_0_OCTET 311
#define SIP_0_ONE_SECOND_PAUSE 263
#define SIP_0_OPAQUE 185
#define SIP_0_OPAQUE_PART 54
#define SIP_0_OPTION_TAG 192
#define SIP_0_OPTIONSM 7
#define SIP_0_ORGANIZATION 171
#define SIP_0_OTHER_CHALLENGE 177
#define SIP_0_OTHER_HANDLING 131
#define SIP_0_OTHER_PARAM 39
#define SIP_0_OTHER_PRIORITY 174
#define SIP_0_OTHER_RESPONSE 104
#define SIP_0_OTHER_TRANSPORT 32
#define SIP_0_OTHER_USER 34
#define SIP_0_PARAM 59
#define SIP_0_PARAM_UNRESERVED 43
#define SIP_0_PARAMCHAR 42
#define SIP_0_PASSWORD 22
#define SIP_0_PATH_SEGMENTS 57
#define SIP_0_PAUSE_CHARACTER 262
#define SIP_0_PCHAR 60
#define SIP_0_PHONE_CONTEXT_IDENT 249
#define SIP_0_PHONE_CONTEXT_TAG 248
#define SIP_0_PHONEDIGIT 260
#define SIP_0_PNAME 40
#define SIP_0_PORT 28
#define SIP_0_POST_DIAL 246
#define SIP_0_PRIMARY_TAG 136
#define SIP_0_PRIORITY 172
#define SIP_0_PRIORITY_VALUE 173
#define SIP_0_PRIVATE_PREFIX 253
#define SIP_0_PRODUCT 206
#define SIP_0_PRODUCT_VERSION 207
#define SIP_0_PROTOCOL_NAME 225
#define SIP_0_PROTOCOL_VERSION 226
#define SIP_0_PROVIDER_HOSTNAME 256
#define SIP_0_PROVIDER_TAG 255
#define SIP_0_PROXY_AUTHENTICATE 175
#define SIP_0_PROXY_AUTHORIZATION 190
#define SIP_0_PROXY_REQUIRE 191
#define SIP_0_PSEUDONYM 235
#define SIP_0_PVALUE 41
#define SIP_0_Q_STRING 297
#define SIP_0_QDTEXT 298
#define SIP_0_QOP_OPTIONS 188
#define SIP_0_QOP_VALUE 189
#define SIP_0_QUERY 65
#define SIP_0_QUOTED_PAIR 299
#define SIP_0_QUOTED_STRING 296
#define SIP_0_QVALUE 74
#define SIP_0_RAQUOT 287
#define SIP_0_RDQUOT 293
#define SIP_0_REALM 179
#define SIP_0_REALM_VALUE 180
#define SIP_0_REASON_PHRASE 16
#define SIP_0_REC_ROUTE 194
#define SIP_0_RECORD_ROUTE 193
#define SIP_0_REG_NAME 64
#define SIP_0_REGISTERM 10
#define SIP_0_REPLY_TO 196
#define SIP_0_REQUEST 2
#define SIP_0_REQUEST_DIGEST 101
#define SIP_0_REQUEST_LINE 3
#define SIP_0_REQUEST_URI 49
#define SIP_0_REQUIRE 199
#define SIP_0_RESERVED 270
#define SIP_0_RESPONSE 13
#define SIP_0_RESPONSE_AUTH 109
#define SIP_0_RESPONSE_DIGEST 110
#define SIP_0_RETRY_AFTER 200
#define SIP_0_RETRY_PARAM 201
#define SIP_0_RFC1035DOMAIN 266
#define SIP_0_RFC1035LABEL 268
#define SIP_0_RFC1035SUBDOMAIN 267
#define SIP_0_RFC1123_DATE 155
#define SIP_0_ROUTE 202
#define SIP_0_ROUTE_PARAM 203
#define SIP_0_RPAREN 286
#define SIP_0_RPLYTO_PARAM 198
#define SIP_0_RPLYTO_SPEC 197
#define SIP_0_RR_PARAM 195
#define SIP_0_SCHEME 61
#define SIP_0_SEGMENT 58
#define SIP_0_SEMI 290
#define SIP_0_SENT_BY 228
#define SIP_0_SENT_PROTOCOL 224
#define SIP_0_SERVER 204
#define SIP_0_SERVER_VAL 205
#define SIP_0_SERVICE_PROVIDER 254
#define SIP_0_SIP_DATE 154
#define SIP_0_SIP_MESSAGE 0
#define SIP_0_SIP_URI 17
#define SIP_0_SIP_VERSION 4
#define SIP_0_SIPS_URI 18
#define SIP_0_SLASH 283
#define SIP_0_SP 312
#define SIP_0_SRVR 63
#define SIP_0_STALE 186
#define SIP_0_STAR 282
#define SIP_0_STATUS_CODE 15
#define SIP_0_STATUS_LINE 14
#define SIP_0_SUBJECT 208
#define SIP_0_SUBTAG 137
#define SIP_0_SUPPORTED 209
#define SIP_0_SWS 301
#define SIP_0_TAG_PARAM 166
#define SIP_0_TEL_QUOTED_STRING 259
#define SIP_0_TELEPHONE_SUBSCRIBER 241
#define SIP_0_TEXT_UTF8_TRIM 274
#define SIP_0_TEXT_UTF8CHAR 275
#define SIP_0_TIME 157
#define SIP_0_TIMESTAMP 210
#define SIP_0_TO 212
#define SIP_0_TO_PARAM 213
#define SIP_0_TOKEN 278
#define SIP_0_TOKEN_CHAR 258
#define SIP_0_TOPLABEL 27
#define SIP_0_TRANSPORT 227
#define SIP_0_TRANSPORT_PARAM 31
#define SIP_0_TTL 229
#define SIP_0_TTL_PARAM 36
#define SIP_0_UNRESERVED 271
#define SIP_0_UNSUPPORTED 214
#define SIP_0_URI 182
#define SIP_0_URI_PARAMETER 30
#define SIP_0_URI_PARAMETERS 29
#define SIP_0_URIC 55
#define SIP_0_URIC_NO_SLASH 56
#define SIP_0_USER 20
#define SIP_0_USER_AGENT 215
#define SIP_0_USER_PARAM 33
#define SIP_0_USER_UNRESERVED 21
#define SIP_0_USERINFO 19
#define SIP_0_USERNAME 91
#define SIP_0_USERNAME_VALUE 92
#define SIP_0_UTF8_CONT 277
#define SIP_0_UTF8_NONASCII 276
#define SIP_0_VIA 216
#define SIP_0_VIA_BRANCH 222
#define SIP_0_VIA_EXTENSION 223
#define SIP_0_VIA_MADDR 220
#define SIP_0_VIA_PARAMS 218
#define SIP_0_VIA_PARM 217
#define SIP_0_VIA_RECEIVED 221
#define SIP_0_VIA_TTL 219
#define SIP_0_VISUAL_SEPARATOR 261
#define SIP_0_WAIT_FOR_DIAL_TONE 264
#define SIP_0_WARN_AGENT 233
#define SIP_0_WARN_CODE 232
#define SIP_0_WARN_TEXT 234
#define SIP_0_WARNING 230
#define SIP_0_WARNING_VALUE 231
#define SIP_0_WKDAY 158
#define SIP_0_WORD 279
#define SIP_0_WSP 302
#define SIP_0_WWW_AUTHENTICATE 236
#define SIP_0_X_TOKEN 146
#define RULE_COUNT_SIP_0 313

// pointer to parser initialization data
extern void* vpSip0Init;

// Helper function(s) for setting rule/UDT name callbacks.
// Un-comment and replace named NULL with pointer to the appropriate callback function.
//  NOTE: This can easily be modified for setting AST callback functions:
//        Replace parser_callback with ast_callback and
//        vParserSetRuleCallback(vpParserCtx) with vAstSetRuleCallback(vpAstCtx) and
//        vParserSetUdtCallback(vpParserCtx) with vAstSetUdtCallback(vpAstCtx).
/****************************************************************
void vSip0RuleCallbacks(void* vpParserCtx){
    aint ui;
    parser_callback cb[RULE_COUNT_SIP_0];
    cb[SIP_0_ABS_PATH] = NULL;
    cb[SIP_0_ABSOLUTEURI] = NULL;
    cb[SIP_0_ACCEPT] = NULL;
    cb[SIP_0_ACCEPT_ENCODING] = NULL;
    cb[SIP_0_ACCEPT_LANGUAGE] = NULL;
    cb[SIP_0_ACCEPT_PARAM] = NULL;
    cb[SIP_0_ACCEPT_RANGE] = NULL;
    cb[SIP_0_ACKM] = NULL;
    cb[SIP_0_ADDR_SPEC] = NULL;
    cb[SIP_0_AINFO] = NULL;
    cb[SIP_0_ALERT_INFO] = NULL;
    cb[SIP_0_ALERT_PARAM] = NULL;
    cb[SIP_0_ALGORITHM] = NULL;
    cb[SIP_0_ALLOW] = NULL;
    cb[SIP_0_ALPHA] = NULL;
    cb[SIP_0_ALPHANUM] = NULL;
    cb[SIP_0_AREA_SPECIFIER] = NULL;
    cb[SIP_0_AUTH_PARAM] = NULL;
    cb[SIP_0_AUTH_PARAM_NAME] = NULL;
    cb[SIP_0_AUTH_SCHEME] = NULL;
    cb[SIP_0_AUTHENTICATION_INFO] = NULL;
    cb[SIP_0_AUTHORITY] = NULL;
    cb[SIP_0_AUTHORIZATION] = NULL;
    cb[SIP_0_BASE_PHONE_NUMBER] = NULL;
    cb[SIP_0_BLANKSPACE] = NULL;
    cb[SIP_0_BYEM] = NULL;
    cb[SIP_0_C_P_EXPIRES] = NULL;
    cb[SIP_0_C_P_Q] = NULL;
    cb[SIP_0_CALL_ID] = NULL;
    cb[SIP_0_CALL_INFO] = NULL;
    cb[SIP_0_CALLID] = NULL;
    cb[SIP_0_CANCELM] = NULL;
    cb[SIP_0_CHALLENGE] = NULL;
    cb[SIP_0_CHAR] = NULL;
    cb[SIP_0_CNONCE] = NULL;
    cb[SIP_0_CNONCE_VALUE] = NULL;
    cb[SIP_0_CODINGS] = NULL;
    cb[SIP_0_COLON] = NULL;
    cb[SIP_0_COMMA] = NULL;
    cb[SIP_0_COMMENT] = NULL;
    cb[SIP_0_COMPOSITE_TYPE] = NULL;
    cb[SIP_0_CONTACT] = NULL;
    cb[SIP_0_CONTACT_EXTENSION] = NULL;
    cb[SIP_0_CONTACT_PARAM] = NULL;
    cb[SIP_0_CONTACT_PARAMS] = NULL;
    cb[SIP_0_CONTENT_CODING] = NULL;
    cb[SIP_0_CONTENT_DISPOSITION] = NULL;
    cb[SIP_0_CONTENT_ENCODING] = NULL;
    cb[SIP_0_CONTENT_LANGUAGE] = NULL;
    cb[SIP_0_CONTENT_LENGTH] = NULL;
    cb[SIP_0_CONTENT_TYPE] = NULL;
    cb[SIP_0_CR] = NULL;
    cb[SIP_0_CREDENTIALS] = NULL;
    cb[SIP_0_CRLF] = NULL;
    cb[SIP_0_CSEQ] = NULL;
    cb[SIP_0_CTEXT] = NULL;
    cb[SIP_0_D_NAME] = NULL;
    cb[SIP_0_DATE] = NULL;
    cb[SIP_0_DATE1] = NULL;
    cb[SIP_0_DELAY] = NULL;
    cb[SIP_0_DELTA_SECONDS] = NULL;
    cb[SIP_0_DIG_RESP] = NULL;
    cb[SIP_0_DIGEST_CLN] = NULL;
    cb[SIP_0_DIGEST_RESPONSE] = NULL;
    cb[SIP_0_DIGEST_URI] = NULL;
    cb[SIP_0_DIGEST_URI_VALUE] = NULL;
    cb[SIP_0_DIGIT] = NULL;
    cb[SIP_0_DISCRETE_TYPE] = NULL;
    cb[SIP_0_DISP_EXTENSION_TOKEN] = NULL;
    cb[SIP_0_DISP_PARAM] = NULL;
    cb[SIP_0_DISP_TYPE] = NULL;
    cb[SIP_0_DISPLAY_NAME] = NULL;
    cb[SIP_0_DOMAIN] = NULL;
    cb[SIP_0_DOMAINLABEL] = NULL;
    cb[SIP_0_DQUOTE] = NULL;
    cb[SIP_0_DRESPONSE] = NULL;
    cb[SIP_0_DTMF_DIGIT] = NULL;
    cb[SIP_0_ENCODING] = NULL;
    cb[SIP_0_EQUAL] = NULL;
    cb[SIP_0_ERROR_INFO] = NULL;
    cb[SIP_0_ERROR_URI] = NULL;
    cb[SIP_0_ESCAPED] = NULL;
    cb[SIP_0_EXPIRES] = NULL;
    cb[SIP_0_EXTENSION_HEADER] = NULL;
    cb[SIP_0_EXTENSION_METHOD] = NULL;
    cb[SIP_0_EXTENSION_TOKEN] = NULL;
    cb[SIP_0_FROM] = NULL;
    cb[SIP_0_FROM_PARAM] = NULL;
    cb[SIP_0_FROM_SPEC] = NULL;
    cb[SIP_0_FUTURE_EXTENSION] = NULL;
    cb[SIP_0_GEN_VALUE] = NULL;
    cb[SIP_0_GENERIC_PARAM] = NULL;
    cb[SIP_0_GLOBAL_NETWORK_PREFIX] = NULL;
    cb[SIP_0_GLOBAL_PHONE_NUMBER] = NULL;
    cb[SIP_0_HANDLING_PARAM] = NULL;
    cb[SIP_0_HCOLON] = NULL;
    cb[SIP_0_HEADER] = NULL;
    cb[SIP_0_HEADER_NAME] = NULL;
    cb[SIP_0_HEADER_VALUE] = NULL;
    cb[SIP_0_HEADERS] = NULL;
    cb[SIP_0_HEXDIG] = NULL;
    cb[SIP_0_HIER_PART] = NULL;
    cb[SIP_0_HNAME] = NULL;
    cb[SIP_0_HNV_UNRESERVED] = NULL;
    cb[SIP_0_HOST] = NULL;
    cb[SIP_0_HOSTNAME] = NULL;
    cb[SIP_0_HOSTPORT] = NULL;
    cb[SIP_0_HVALUE] = NULL;
    cb[SIP_0_IANA_TOKEN] = NULL;
    cb[SIP_0_IETF_TOKEN] = NULL;
    cb[SIP_0_IN_REPLY_TO] = NULL;
    cb[SIP_0_INFO] = NULL;
    cb[SIP_0_INFO_PARAM] = NULL;
    cb[SIP_0_INVITEM] = NULL;
    cb[SIP_0_IPV4ADDRESS] = NULL;
    cb[SIP_0_IPV6ADDRESS] = NULL;
    cb[SIP_0_IPV6REFERENCE] = NULL;
    cb[SIP_0_ISDN_SUBADDRESS] = NULL;
    cb[SIP_0_LANGUAGE] = NULL;
    cb[SIP_0_LANGUAGE_RANGE] = NULL;
    cb[SIP_0_LANGUAGE_TAG] = NULL;
    cb[SIP_0_LAQUOT] = NULL;
    cb[SIP_0_LDQUOT] = NULL;
    cb[SIP_0_LF] = NULL;
    cb[SIP_0_LHEX] = NULL;
    cb[SIP_0_LOCAL_NETWORK_PREFIX] = NULL;
    cb[SIP_0_LOCAL_PHONE_NUMBER] = NULL;
    cb[SIP_0_LPAREN] = NULL;
    cb[SIP_0_LR_PARAM] = NULL;
    cb[SIP_0_LWS] = NULL;
    cb[SIP_0_M_ATTRIBUTE] = NULL;
    cb[SIP_0_M_PARAMETER] = NULL;
    cb[SIP_0_M_SUBTYPE] = NULL;
    cb[SIP_0_M_TYPE] = NULL;
    cb[SIP_0_M_VALUE] = NULL;
    cb[SIP_0_MADDR_PARAM] = NULL;
    cb[SIP_0_MARK] = NULL;
    cb[SIP_0_MAX_FORWARDS] = NULL;
    cb[SIP_0_MEDIA_RANGE] = NULL;
    cb[SIP_0_MEDIA_TYPE] = NULL;
    cb[SIP_0_MESSAGE_BODY] = NULL;
    cb[SIP_0_MESSAGE_HEADER] = NULL;
    cb[SIP_0_MESSAGE_QOP] = NULL;
    cb[SIP_0_METHOD] = NULL;
    cb[SIP_0_METHOD_PARAM] = NULL;
    cb[SIP_0_MIME_VERSION] = NULL;
    cb[SIP_0_MIN_EXPIRES] = NULL;
    cb[SIP_0_MONTH] = NULL;
    cb[SIP_0_NAME_ADDR] = NULL;
    cb[SIP_0_NC_VALUE] = NULL;
    cb[SIP_0_NET_PATH] = NULL;
    cb[SIP_0_NETWORK_PREFIX] = NULL;
    cb[SIP_0_NEXTNONCE] = NULL;
    cb[SIP_0_NONCE] = NULL;
    cb[SIP_0_NONCE_COUNT] = NULL;
    cb[SIP_0_NONCE_VALUE] = NULL;
    cb[SIP_0_OCTET] = NULL;
    cb[SIP_0_ONE_SECOND_PAUSE] = NULL;
    cb[SIP_0_OPAQUE] = NULL;
    cb[SIP_0_OPAQUE_PART] = NULL;
    cb[SIP_0_OPTION_TAG] = NULL;
    cb[SIP_0_OPTIONSM] = NULL;
    cb[SIP_0_ORGANIZATION] = NULL;
    cb[SIP_0_OTHER_CHALLENGE] = NULL;
    cb[SIP_0_OTHER_HANDLING] = NULL;
    cb[SIP_0_OTHER_PARAM] = NULL;
    cb[SIP_0_OTHER_PRIORITY] = NULL;
    cb[SIP_0_OTHER_RESPONSE] = NULL;
    cb[SIP_0_OTHER_TRANSPORT] = NULL;
    cb[SIP_0_OTHER_USER] = NULL;
    cb[SIP_0_PARAM] = NULL;
    cb[SIP_0_PARAM_UNRESERVED] = NULL;
    cb[SIP_0_PARAMCHAR] = NULL;
    cb[SIP_0_PASSWORD] = NULL;
    cb[SIP_0_PATH_SEGMENTS] = NULL;
    cb[SIP_0_PAUSE_CHARACTER] = NULL;
    cb[SIP_0_PCHAR] = NULL;
    cb[SIP_0_PHONE_CONTEXT_IDENT] = NULL;
    cb[SIP_0_PHONE_CONTEXT_TAG] = NULL;
    cb[SIP_0_PHONEDIGIT] = NULL;
    cb[SIP_0_PNAME] = NULL;
    cb[SIP_0_PORT] = NULL;
    cb[SIP_0_POST_DIAL] = NULL;
    cb[SIP_0_PRIMARY_TAG] = NULL;
    cb[SIP_0_PRIORITY] = NULL;
    cb[SIP_0_PRIORITY_VALUE] = NULL;
    cb[SIP_0_PRIVATE_PREFIX] = NULL;
    cb[SIP_0_PRODUCT] = NULL;
    cb[SIP_0_PRODUCT_VERSION] = NULL;
    cb[SIP_0_PROTOCOL_NAME] = NULL;
    cb[SIP_0_PROTOCOL_VERSION] = NULL;
    cb[SIP_0_PROVIDER_HOSTNAME] = NULL;
    cb[SIP_0_PROVIDER_TAG] = NULL;
    cb[SIP_0_PROXY_AUTHENTICATE] = NULL;
    cb[SIP_0_PROXY_AUTHORIZATION] = NULL;
    cb[SIP_0_PROXY_REQUIRE] = NULL;
    cb[SIP_0_PSEUDONYM] = NULL;
    cb[SIP_0_PVALUE] = NULL;
    cb[SIP_0_Q_STRING] = NULL;
    cb[SIP_0_QDTEXT] = NULL;
    cb[SIP_0_QOP_OPTIONS] = NULL;
    cb[SIP_0_QOP_VALUE] = NULL;
    cb[SIP_0_QUERY] = NULL;
    cb[SIP_0_QUOTED_PAIR] = NULL;
    cb[SIP_0_QUOTED_STRING] = NULL;
    cb[SIP_0_QVALUE] = NULL;
    cb[SIP_0_RAQUOT] = NULL;
    cb[SIP_0_RDQUOT] = NULL;
    cb[SIP_0_REALM] = NULL;
    cb[SIP_0_REALM_VALUE] = NULL;
    cb[SIP_0_REASON_PHRASE] = NULL;
    cb[SIP_0_REC_ROUTE] = NULL;
    cb[SIP_0_RECORD_ROUTE] = NULL;
    cb[SIP_0_REG_NAME] = NULL;
    cb[SIP_0_REGISTERM] = NULL;
    cb[SIP_0_REPLY_TO] = NULL;
    cb[SIP_0_REQUEST] = NULL;
    cb[SIP_0_REQUEST_DIGEST] = NULL;
    cb[SIP_0_REQUEST_LINE] = NULL;
    cb[SIP_0_REQUEST_URI] = NULL;
    cb[SIP_0_REQUIRE] = NULL;
    cb[SIP_0_RESERVED] = NULL;
    cb[SIP_0_RESPONSE] = NULL;
    cb[SIP_0_RESPONSE_AUTH] = NULL;
    cb[SIP_0_RESPONSE_DIGEST] = NULL;
    cb[SIP_0_RETRY_AFTER] = NULL;
    cb[SIP_0_RETRY_PARAM] = NULL;
    cb[SIP_0_RFC1035DOMAIN] = NULL;
    cb[SIP_0_RFC1035LABEL] = NULL;
    cb[SIP_0_RFC1035SUBDOMAIN] = NULL;
    cb[SIP_0_RFC1123_DATE] = NULL;
    cb[SIP_0_ROUTE] = NULL;
    cb[SIP_0_ROUTE_PARAM] = NULL;
    cb[SIP_0_RPAREN] = NULL;
    cb[SIP_0_RPLYTO_PARAM] = NULL;
    cb[SIP_0_RPLYTO_SPEC] = NULL;
    cb[SIP_0_RR_PARAM] = NULL;
    cb[SIP_0_SCHEME] = NULL;
    cb[SIP_0_SEGMENT] = NULL;
    cb[SIP_0_SEMI] = NULL;
    cb[SIP_0_SENT_BY] = NULL;
    cb[SIP_0_SENT_PROTOCOL] = NULL;
    cb[SIP_0_SERVER] = NULL;
    cb[SIP_0_SERVER_VAL] = NULL;
    cb[SIP_0_SERVICE_PROVIDER] = NULL;
    cb[SIP_0_SIP_DATE] = NULL;
    cb[SIP_0_SIP_MESSAGE] = NULL;
    cb[SIP_0_SIP_URI] = NULL;
    cb[SIP_0_SIP_VERSION] = NULL;
    cb[SIP_0_SIPS_URI] = NULL;
    cb[SIP_0_SLASH] = NULL;
    cb[SIP_0_SP] = NULL;
    cb[SIP_0_SRVR] = NULL;
    cb[SIP_0_STALE] = NULL;
    cb[SIP_0_STAR] = NULL;
    cb[SIP_0_STATUS_CODE] = NULL;
    cb[SIP_0_STATUS_LINE] = NULL;
    cb[SIP_0_SUBJECT] = NULL;
    cb[SIP_0_SUBTAG] = NULL;
    cb[SIP_0_SUPPORTED] = NULL;
    cb[SIP_0_SWS] = NULL;
    cb[SIP_0_TAG_PARAM] = NULL;
    cb[SIP_0_TEL_QUOTED_STRING] = NULL;
    cb[SIP_0_TELEPHONE_SUBSCRIBER] = NULL;
    cb[SIP_0_TEXT_UTF8_TRIM] = NULL;
    cb[SIP_0_TEXT_UTF8CHAR] = NULL;
    cb[SIP_0_TIME] = NULL;
    cb[SIP_0_TIMESTAMP] = NULL;
    cb[SIP_0_TO] = NULL;
    cb[SIP_0_TO_PARAM] = NULL;
    cb[SIP_0_TOKEN] = NULL;
    cb[SIP_0_TOKEN_CHAR] = NULL;
    cb[SIP_0_TOPLABEL] = NULL;
    cb[SIP_0_TRANSPORT] = NULL;
    cb[SIP_0_TRANSPORT_PARAM] = NULL;
    cb[SIP_0_TTL] = NULL;
    cb[SIP_0_TTL_PARAM] = NULL;
    cb[SIP_0_UNRESERVED] = NULL;
    cb[SIP_0_UNSUPPORTED] = NULL;
    cb[SIP_0_URI] = NULL;
    cb[SIP_0_URI_PARAMETER] = NULL;
    cb[SIP_0_URI_PARAMETERS] = NULL;
    cb[SIP_0_URIC] = NULL;
    cb[SIP_0_URIC_NO_SLASH] = NULL;
    cb[SIP_0_USER] = NULL;
    cb[SIP_0_USER_AGENT] = NULL;
    cb[SIP_0_USER_PARAM] = NULL;
    cb[SIP_0_USER_UNRESERVED] = NULL;
    cb[SIP_0_USERINFO] = NULL;
    cb[SIP_0_USERNAME] = NULL;
    cb[SIP_0_USERNAME_VALUE] = NULL;
    cb[SIP_0_UTF8_CONT] = NULL;
    cb[SIP_0_UTF8_NONASCII] = NULL;
    cb[SIP_0_VIA] = NULL;
    cb[SIP_0_VIA_BRANCH] = NULL;
    cb[SIP_0_VIA_EXTENSION] = NULL;
    cb[SIP_0_VIA_MADDR] = NULL;
    cb[SIP_0_VIA_PARAMS] = NULL;
    cb[SIP_0_VIA_PARM] = NULL;
    cb[SIP_0_VIA_RECEIVED] = NULL;
    cb[SIP_0_VIA_TTL] = NULL;
    cb[SIP_0_VISUAL_SEPARATOR] = NULL;
    cb[SIP_0_WAIT_FOR_DIAL_TONE] = NULL;
    cb[SIP_0_WARN_AGENT] = NULL;
    cb[SIP_0_WARN_CODE] = NULL;
    cb[SIP_0_WARN_TEXT] = NULL;
    cb[SIP_0_WARNING] = NULL;
    cb[SIP_0_WARNING_VALUE] = NULL;
    cb[SIP_0_WKDAY] = NULL;
    cb[SIP_0_WORD] = NULL;
    cb[SIP_0_WSP] = NULL;
    cb[SIP_0_WWW_AUTHENTICATE] = NULL;
    cb[SIP_0_X_TOKEN] = NULL;
    for(ui = 0; ui < (aint)RULE_COUNT_SIP_0; ui++){
        vParserSetRuleCallback(vpParserCtx, ui, cb[ui]);
    }
}
****************************************************************/

#endif /* _SIP_0_H_ */
