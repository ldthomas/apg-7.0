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
/// \file attributes.h
/// \brief Header file for the attributes functions.

#ifndef ATTRIBUTES_H_
#define ATTRIBUTES_H_

#define BOOL_UNDEFINED ((abool)-1)

/** \brief The API will construct an attributes object. This is the attribute object's context.
 *
 */
typedef struct{
    const void* vpValidate; ///< \brief True if the context is valid.
    exception* spException; ///< \brief Pointer to the exception context inherited from the parent API.
    void* vpMem; ///< \brief Pointer to the memory context inherited from the parent API.
    api* spApi; ///< \brief Pointer to the parent API context.
    api_attr_w* spWorkingAttrs; ///< \brief An array of private attribute structures.
    ///< Construction requires iterations with space to hold intermediate values.
    api_attr_w* spAttrs; ///< \brief An array of private attribute structures used in their construction.
    api_attr* spPublicAttrs; ///< \brief When attributes a complete, the public version strips some of the unneeded variables used only in construction.
    api_attr* spErrorAttrs; ///< \brief An array of all rule attributes that have errors. (i.e. left recursive)
    aint uiStartRule; ///< \brief The grammar start rule.
    aint uiErrorCount; ///< \brief The number of rules that have attribute errors.
    void* vpVecGroupNumbers; ///< \brief A vector for the discovery of groups of mutually recursive rules.
} attrs_ctx;

// private prototypes
void vAttrsDtor(void* vpCtx);
void vRuleDependencies(attrs_ctx* spAtt);
void vRuleAttributes(attrs_ctx* spAtt);
void vAttrsByName(api_attr* spAttrs, aint uiCount, FILE* spStream);
void vAttrsByIndex(api_attr* spAttrs, aint uiCount, FILE* spStream);
void vAttrsByType(api_attr* spAttrs, aint uiCount, FILE* spStream);
const char * cpType(aint uiId);

/**
﻿\page attrs Attributes Validation

\htmlonly<a id="top"></a>\endhtmlonly
[What are Rule Attributes?](#what)<br>
[Some Attributes Basics](#basics)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[Mutually-Recursive Rules](#mutually)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[The Role of Empty Strings](#empty-strings)<br>
[How are Attributes Determined?](#how)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[Rule Types](#types)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[The Opcode Rules for Attributes](#opcode-rules)<br>
\htmlonly<a id="what"></a>\endhtmlonly
## What are Rule Attributes?
Attributes are aspects of the rules of a grammar that exist beyond the syntax and semantics of the grammar.
It is entirely possible to write a well-formed ABNF grammar that is unparsable due to problems with its recursiveness.
Identifying and computing these attributes is one of the most complex aspects of generating a working parser from an ABNF or SABNF grammar.
Therefore, I'll try to describe attributes and their computation some in detail here.

It is well known that left-recursive rules are unparsable. Consider
> S = S "a" / "b"

The first step in matching any input string is to expand the rule `S`. It is also the second and all future steps.
In other words, the parser will simply recurs the rule `S` until there is a stack overflow exception.
Left-recursion is therefore an attribute (or aspect or quality or characteristic) of the rule `S` that is important to know.
APG regards left recursion as a fatal attribute of the rule `S` and scans each input SABNF grammar to
determine in advance whether any rule is left-recrusive.

But there is more. APG actually identifies six different attributes and evaluates them for each rule in the grammar,
three of which are fatal.
These are the six attribute definitions with a simple example of each.

[top](#top)

**Left Recursion**

> S = S "a" / "b"

Left recursion is a fatal attribute as discussed above.

**Right Recursion**

> S = "a" S / "b"

Right recursion is a non-fatal attribute and generates repetitions.
The above rule matches strings of the form `a`<sup>`n`</sup>`b, n> 0`.

**Nested Recursion**

> S = "a" (S/"ab") "b"

Nested recursion is a non-fatal attribute and differentiates between regular and context-free expressions.
Rules that do not exhibit nested recursion are equivalent to regular expressions.
The above rule matches strings of the form `a`<sup>`n`</sup>`b`<sup>`n`</sup>, `n > 0`,
a well-known example of a rule that can't be represented with a regular expression.

**Cyclic Recursion**
> S = S

Cyclic recursion is a fatal attribute. It has no terminal nodes and cannot represent any string.

**empty**
> S = *"a"<br>
> E = ""

The empty attribute is non-fatal but, as we will see later, extremely important in the determination of the recursive attributes.
The rule `S` can be, but is not necessarily empty. It matches the strings `a`<sup>`n`</sup>, `n >= 0`.
On the other hand, the rule `E` only matches the empty string.

**finite**
> S = "a" S

The finite attribute is fatal if it is false. The above rule only matches infinite strings.
A rule must have at least one branch or alternative that ends with a terminal node to be finite.



[top](#top)
\htmlonly<a id="basics"></a>\endhtmlonly
## Some Attributes Basics

Attributes are determined by walking the syntax tree of opcodes.
The procedure is to walk down to a terminal node, set the known attributes of the terminal node
and then apply fixed modification rules at each of the non-terminal nodes walking back up the tree.
But what about recursive rules? They have infinitely deep syntax trees.
The saving grace is that all the attribute information can be had from a single expansion of each rule.
That is, APG will walk a Single-Expansion Syntax Tree (SEST) of the rule's opcodes to determine the attributes.
Let's look at an example.
\dot
digraph example {
size = "5,5";
ratio = "fill";
node [fontname=Helvetica, fontsize=10];
label="S = \"a\" S / \"b\"";
s [ label="RNM( S )"];
alt [label="ALT"];
cat [label="CAT"];
b [label="TLS( \"b\" )"];
a [label="TLS( \"a\" )"];
leaf [label="RNM( S-leaf )"];
s->alt
alt->{cat b}
cat->{a leaf}
}
\enddot
Here, the right-recursive rule `S` is expanded only once. When it is encountered a second time it is considered a special
type of terminal node, called here, somewhat arbitrarily, a leaf node. Let's walk this SEST and see how it is done. In this simplified example a lot of details and
special cases will be overlooked, but it will reveal the essentials of the algorithm.

The `TLS` terminals are non-recursive, finite and possibly empty, but non-empty in this case. But how do we handle the `S` leaf terminal node? The answer is to ask, "what would the attributes be if the rule were cyclic with no non-terminals in the tree?" For
> S = S

We would say the attributes were left, right and cyclic. So that is what we use for the `S` leaf node. In a correctly written grammar, the non-terminal nodes in the tree between the root `S` node and the `S` leaf node will modify those fatal attributes to non-fatal ones. Let's see how that works out. Moving up to the `CAT` node, its basic rules are:
 - if any child is not empty, `CAT` is not empty
 - if any child is not finite, `CAT` is not finite
 - if the right-most child is right-recursive, `CAT` is right-recursive
 - if the left-most child is left-recursive, `CAT` is left-recursive
 - if all children are cyclic, `CAT` is cyclic
In this case, we see that `CAT` is non-empty, finite, and right-recursive.

Moving up to the `ATL` node, its basic rule is:
 - if any child attribute is true, the corresponding `ALT` attribute is true

In this manner, we see that `ALT` and hence `S` is not empty, finite and right-recursive.

That's the basic idea. Walk the SEST of opcodes, work backwards from the known attributes of the terminal nodes and use combination rules at the non-terminal nodes to migrate the final attributes to the root rule.

[top](#top)
\htmlonly<a id="mutually"></a>\endhtmlonly
### Mutually-Recursive Rules
 Consider the grammar
> S = "s" A B<br>
> A = "a" B / "y"<br>
> B = "b" A / "x"<br>

\dot
digraph example {
size = "6, 7";
ratio = "fill";
node [fontname=Helvetica, fontsize=10];
label="S = \"s\" A B\nA = \"a\" / B\nB = \"b\" A / \"x\"";
R1 [ label="RNM( S )"];
cat1 [label="CAT"];
t1 [label="TLS( \"s\""];
R2 [ label="RNM( A )"];
R3 [ label="RNM( B )"];
alt1 [label="ALT"];
alt2 [label="ALT"];
cat2 [label="CAT"];
t2 [label="TLS( \"y\""];
cat3 [label="CAT"];
t3 [label="TLS( \"x\""];
t4 [label="TLS( \"a\""];
R4 [label="RNM( B )"];
t5 [label="TLS( \"b\""];
R5 [label="RNM( A )"];
alt3 [label="ALT"];
alt4 [label="ALT"];
cat4 [label="CAT"];
t6 [label="TLS( \"x\""];
cat5 [label="CAT"];
t7 [label="TLS( \"y\""];
t8 [label="TLS( \"b\""];
R6 [label="RNM( A-leaf )"];
t9 [label="TLS( \"a\""];
R7 [label="RNM( B-leaf )"];

R1->cat1
cat1->{t1 R2 R3}
R2->alt1
alt1->{cat2 t2}
cat2->{t4 R4}
R4->alt3
alt3->{cat4 t6}
cat4->{t8 R6}

R3->alt2
alt2->{cat3 t3}
cat3->{t5 R5}
R5->alt4
alt4->{cat5 t7}
cat5->{t9 R7}
}\enddot
Notice that `A` and `B` are recursive and refer to one another. That is, they are "mutually recursive". At first glance that would appear to be a problem in that we would need to know the attributes of `A` to determine those of `B` and vice versa &ndash; a catch-22.

However, the way out of this is to first make the observation that `S` is non-recursive. But if we include the recursive attributes of `A` and `B`, right-recursive, then we would incorrectly compute `S` to likewise be right-recursive. So we see that to compute the recursive attributes of `S` we need to ignore the recursive attributes of `A` and `B` (but not the empty and finite attributes.)

So this is our way out of the catch-22. When computing the recursive attributes of `A` we ignore those of `B`. In fact, the general algorithm is to ignore all recursive attributes of all rules accept the rule under consideration (the root of the SEST.)

[top](#top)
\htmlonly<a id="empty-strings"></a>\endhtmlonly
### The Role of Empty Strings
The empty attribute can be true or false. Both are acceptable. Neither is fatal causing the parser to fail.
However, the empty attribute plays a vital role in determining the recursive attributes. Consider the following rule.
> S = *"a" S / "y"

It looks a lot like the right-recurive example above. But there is a big difference.
Because the repetions operator `*` can accept an empty string, this rule is actually left recursive.
This means that our `CAT` rules have to be modified to read
- if the left-most non-empty term is left-recursive, `CAT` is left-recursive.
- if the right-most non-empty term is right-recursive, `CAT` is right-recursive.


\htmlonly<p class="clear-left"></p>\endhtmlonly
[top](#top)
\htmlonly<a id="how"></a>\endhtmlonly
## How are Attributes Determined?
As explained above, the SEST of opcodes is walked for each rule.
The terminal opcode nodes have known attributes and the non-terminal opcode nodes have rules for determining
their attribute from their children below.
The specifics of the actual alogrithms are explained here in more detail.
For complete details see the code in \ref rule-dependencies.c and \ref rule-attributes.c.

[top](#top)
\htmlonly<a id="types"></a>\endhtmlonly
### Rule Types
Although they are not used in the determination of the rule attributes, rules are classified into three types for information and display purposes.
 - N: non-recursive &ndash; the rule never refers to itself.
 - R: recursive &ndash; the rule refers to itself, ether directly or indirectly.
 - MR: mutually-recursive &ndash; a group of rules that refer to themselves and every other rule in the group.

The API offers tools to display the rules, by index (the order they appear in the grammar),
by name alphabetically, and by type, names alphabetical within a given type or mutually-recursive group.
See \ref vApiRulesToAscii and \ref vApiRulesToHtml.

[top](#top)
\htmlonly<a id="opcode-rules"></a>\endhtmlonly
### The Opcode Rules for Attributes
The terminals have known attributes and can be set directly.
All of their recursive attributes are false.
All of their finite attributes are true.
The empty attribute depends on the terminal operator.

<table>
<caption>Terminal Operators</caption>
    <tr>
        <th>terminal operator</th><th>attributes</th>
    </tr>
    <tr>
        <td>TLS: terminal literal string</td>
        <td>empty: true if string length = 0, false if not</td>
    </tr>
    <tr>
        <td>TBS: terminal binary string</td>
        <td>empty: false (binary string can never be empty)</td>
    </tr>
    <tr>
        <td>TRG: terminal range</td>
        <td>empty: false</td>
    </tr>
    <tr>
        <td>BKR: back reference</td>
        <td>empty: inherits from the back-referenced rule</td>
    </tr>
    <tr>
        <td>UDT: user-defined terminal</td>
        <td>empty: true if e_name, false if u_name</td>
    </tr>
    <tr>
        <td>ABG: begin of string anchor</td>
        <td>empty: true</td>
    </tr>
    <tr>
        <td>AEN: end of string anchor</td>
        <td>empty: true</td>
    </tr>
</table>
Then there is a group of non-terminal operators that inherit all attributes from their single child
except the empty attribute.
<table>
<caption>Non-terminal operators with single child.</caption>
    <tr>
        <th>operator</th><th>attributes</th>
    </tr>
    <tr>
        <td>REP: repetition, n*m</td>
        <td>empty: true if n = 0, inherited otherwise</td>
    </tr>
    <tr>
        <td>NOT: positive look ahead (&)</td>
        <td>empty: true</td>
    </tr>
    <tr>
        <td>AND: negative look ahead (!)</td>
        <td>empty: true
    </tr>
    <tr>
        <td>BKA: positive look behind (&&)</td>
        <td>empty: true
    </tr>
    <tr>
        <td>BKN: negative look behind (!!)</td>
        <td>empty: true
    </tr>
</table>
The remaining non-terminal rules get complicated. Here you are mostly referred to the actual code to see how it works.
<table>
<caption>Complex non-terminal operators.</caption>
    <tr>
        <th>operator</th><th>attributes</th>
    </tr>
    <tr>
        <td>ALT: alternation</td>
        <td>see `vAltAttrs()` in \ref rule-attributes.c</td>
    </tr>
    <tr>
        <td>CAT: concatenation</td>
        <td>see `vCatAttrs()` in \ref rule-attributes.c</td>
    </tr>
    <tr>
        <td>RNM(root): rule operator</td>
        <td>inherit all attributes from single child</td>
    </tr>
    <tr>
        <td>RNM(root leaf<sup>&dagger;</sup>):</td>
    <td>
        <ul>
            <li>
            left: true
            </li>
            <li>
            nested: false
            </li>
            <li>
            right: true
            </li>
            <li>
            cyclic: true
            </li>
            <li>
            empty: false
            </li>
            <li>
            finite: false
            </li>
        </ul>
    </td>
    <tr>
        <td>RNM(non-root leaf<sup>&Dagger;</sup>):</td>
    <td>
        <ul>
            <li>
            left: false
            </li>
            <li>
            nested: false
            </li>
            <li>
            right: false
            </li>
            <li>
            cyclic: false
            </li>
            <li>
            empty: false
            </li>
            <li>
            finite: false
            </li>
        </ul>
    </td>
</table>
<p>
<sup>&dagger;</sup> Root leaf is defined as the second occurrence of the root (start) rule on any branch.<br>
<sup>&Dagger;</sup> Non-root leaf is defined as the second occurrence of any rule other than the root rule on any branch.
</p>
[top](#top)
 */
#endif /* ATTRIBUTES_H_ */
