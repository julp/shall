#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "utils.h"

/**
 * Spec/language reference:
 * - 2.1
 *   + http://www.w3.org/TR/CSS2/grammar.html
 * - 3
 *   + http://www.w3.org/TR/css-syntax-3/
 *   + http://www.w3.org/TR/2003/WD-css3-syntax-20030813/
 */

enum {
    STATE(INITIAL),
    STATE(IN_COMMENT),
    STATE(IN_CONTENT),
    STATE(IN_STRING_SINGLE),
    STATE(IN_STRING_DOUBLE),
};

static named_element_t vendor_prefixes[] = {
    NE("-ah-"),
    NE("-atsc-"),
    NE("-hp-"),
    NE("-khtml-"),
    NE("-moz-"),
    NE("-ms-"),
    NE("-o-"),
    NE("-rim-"),
    NE("-ro-"),
    NE("-tc-"),
    NE("-wap-"),
    NE("-webkit-"),
    NE("-xv-"),
    NE("mso-"),
    NE("prince-")
};

static named_element_t attributes[] = {
    NE("align-content"),
    NE("align-items"),
    NE("align-self"),
    NE("alignment-adjust"),
    NE("alignment-baseline"),
    NE("all"),
    NE("anchor-point"),
    NE("animation"),
    NE("animation-delay"),
    NE("animation-direction"),
    NE("animation-duration"),
    NE("animation-fill-mode"),
    NE("animation-iteration-count"),
    NE("animation-name"),
    NE("animation-play-state"),
    NE("animation-timing-function"),
    NE("appearance"),
    NE("azimuth"),
    NE("backface-visibility"),
    NE("background"),
    NE("background-attachment"),
    NE("background-clip"),
    NE("background-color"),
    NE("background-image"),
    NE("background-origin"),
    NE("background-position"),
    NE("background-repeat"),
    NE("background-size"),
    NE("baseline-shift"),
    NE("binding"),
    NE("bleed"),
    NE("bookmark-label"),
    NE("bookmark-level"),
    NE("bookmark-state"),
    NE("bookmark-target"),
    NE("border"),
    NE("border-bottom"),
    NE("border-bottom-color"),
    NE("border-bottom-left-radius"),
    NE("border-bottom-right-radius"),
    NE("border-bottom-style"),
    NE("border-bottom-width"),
    NE("border-collapse"),
    NE("border-color"),
    NE("border-image"),
    NE("border-image-outset"),
    NE("border-image-repeat"),
    NE("border-image-slice"),
    NE("border-image-source"),
    NE("border-image-width"),
    NE("border-left"),
    NE("border-left-color"),
    NE("border-left-style"),
    NE("border-left-width"),
    NE("border-radius"),
    NE("border-right"),
    NE("border-right-color"),
    NE("border-right-style"),
    NE("border-right-width"),
    NE("border-spacing"),
    NE("border-style"),
    NE("border-top"),
    NE("border-top-color"),
    NE("border-top-left-radius"),
    NE("border-top-right-radius"),
    NE("border-top-style"),
    NE("border-top-width"),
    NE("border-width"),
    NE("bottom"),
    NE("box-align"),
    NE("box-decoration-break"),
    NE("box-direction"),
    NE("box-flex"),
    NE("box-flex-group"),
    NE("box-lines"),
    NE("box-ordinal-group"),
    NE("box-orient"),
    NE("box-pack"),
    NE("box-shadow"),
    NE("box-sizing"),
    NE("break-after"),
    NE("break-before"),
    NE("break-inside"),
    NE("caption-side"),
    NE("clear"),
    NE("clip"),
    NE("clip-path"),
    NE("clip-rule"),
    NE("color"),
    NE("color-profile"),
    NE("column-count"),
    NE("column-fill"),
    NE("column-gap"),
    NE("column-rule"),
    NE("column-rule-color"),
    NE("column-rule-style"),
    NE("column-rule-width"),
    NE("column-span"),
    NE("column-width"),
    NE("columns"),
    NE("content"),
    NE("counter-increment"),
    NE("counter-reset"),
    NE("crop"),
    NE("cue"),
    NE("cue-after"),
    NE("cue-before"),
    NE("cursor"),
    NE("direction"),
    NE("display"),
    NE("dominant-baseline"),
    NE("drop-initial-after-adjust"),
    NE("drop-initial-after-align"),
    NE("drop-initial-before-adjust"),
    NE("drop-initial-before-align"),
    NE("drop-initial-size"),
    NE("drop-initial-value"),
    NE("elevation"),
    NE("empty-cells"),
    NE("filter"),
    NE("fit"),
    NE("fit-position"),
    NE("flex"),
    NE("flex-basis"),
    NE("flex-direction"),
    NE("flex-flow"),
    NE("flex-grow"),
    NE("flex-shrink"),
    NE("flex-wrap"),
    NE("float"),
    NE("float-offset"),
    NE("font"),
    NE("font-family"),
    NE("font-feature-settings"),
    NE("font-kerning"),
    NE("font-language-override"),
    NE("font-size"),
    NE("font-size-adjust"),
    NE("font-stretch"),
    NE("font-style"),
    NE("font-synthesis"),
    NE("font-variant"),
    NE("font-variant-alternates"),
    NE("font-variant-caps"),
    NE("font-variant-east-asian"),
    NE("font-variant-ligatures"),
    NE("font-variant-numeric"),
    NE("font-variant-position"),
    NE("font-weight"),
    NE("grid-cell"),
    NE("grid-column"),
    NE("grid-column-align"),
    NE("grid-column-sizing"),
    NE("grid-column-span"),
    NE("grid-columns"),
    NE("grid-flow"),
    NE("grid-row"),
    NE("grid-row-align"),
    NE("grid-row-sizing"),
    NE("grid-row-span"),
    NE("grid-rows"),
    NE("grid-template"),
    NE("hanging-punctuation"),
    NE("height"),
    NE("hyphenate-after"),
    NE("hyphenate-before"),
    NE("hyphenate-character"),
    NE("hyphenate-lines"),
    NE("hyphenate-resource"),
    NE("hyphens"),
    NE("icon"),
    NE("image-orientation"),
    NE("image-rendering"),
    NE("image-resolution"),
    NE("ime-mode"),
    NE("inline-box-align"),
    NE("justify-content"),
    NE("left"),
    NE("letter-spacing"),
    NE("line-break"),
    NE("line-height"),
    NE("line-stacking"),
    NE("line-stacking-ruby"),
    NE("line-stacking-shift"),
    NE("line-stacking-strategy"),
    NE("list-style"),
    NE("list-style-image"),
    NE("list-style-position"),
    NE("list-style-type"),
    NE("margin"),
    NE("margin-bottom"),
    NE("margin-left"),
    NE("margin-right"),
    NE("margin-top"),
    NE("mark"),
    NE("mark-after"),
    NE("mark-before"),
    NE("marker-offset"),
    NE("marks"),
    NE("marquee-direction"),
    NE("marquee-loop"),
    NE("marquee-play-count"),
    NE("marquee-speed"),
    NE("marquee-style"),
    NE("mask"),
    NE("max-height"),
    NE("max-width"),
    NE("min-height"),
    NE("min-width"),
    NE("move-to"),
    NE("nav-down"),
    NE("nav-index"),
    NE("nav-left"),
    NE("nav-right"),
    NE("nav-up"),
    NE("object-fit"),
    NE("object-position"),
    NE("opacity"),
    NE("order"),
    NE("orphans"),
    NE("outline"),
    NE("outline-color"),
    NE("outline-offset"),
    NE("outline-style"),
    NE("outline-width"),
    NE("overflow"),
    NE("overflow-style"),
    NE("overflow-wrap"),
    NE("overflow-x"),
    NE("overflow-y"),
    NE("padding"),
    NE("padding-bottom"),
    NE("padding-left"),
    NE("padding-right"),
    NE("padding-top"),
    NE("page"),
    NE("page-break-after"),
    NE("page-break-before"),
    NE("page-break-inside"),
    NE("page-policy"),
    NE("pause"),
    NE("pause-after"),
    NE("pause-before"),
    NE("perspective"),
    NE("perspective-origin"),
    NE("phonemes"),
    NE("pitch"),
    NE("pitch-range"),
    NE("play-during"),
    NE("pointer-events"),
    NE("position"),
    NE("presentation-level"),
    NE("punctuation-trim"),
    NE("quotes"),
    NE("rendering-intent"),
    NE("resize"),
    NE("rest"),
    NE("rest-after"),
    NE("rest-before"),
    NE("richness"),
    NE("right"),
    NE("rotation"),
    NE("rotation-point"),
    NE("ruby-align"),
    NE("ruby-overhang"),
    NE("ruby-position"),
    NE("ruby-span"),
    NE("size"),
    NE("speak"),
    NE("speak-as"),
    NE("speak-header"),
    NE("speak-numeral"),
    NE("speak-punctuation"),
    NE("speech-rate"),
    NE("src"),
    NE("stress"),
    NE("string-set"),
    NE("tab-size"),
    NE("table-layout"),
    NE("target"),
    NE("target-name"),
    NE("target-new"),
    NE("target-position"),
    NE("text-align"),
    NE("text-align-last"),
    NE("text-combine-horizontal"),
    NE("text-decoration"),
    NE("text-decoration-color"),
    NE("text-decoration-line"),
    NE("text-decoration-skip"),
    NE("text-decoration-style"),
    NE("text-emphasis"),
    NE("text-emphasis-color"),
    NE("text-emphasis-position"),
    NE("text-emphasis-style"),
    NE("text-height"),
    NE("text-indent"),
    NE("text-justify"),
    NE("text-orientation"),
    NE("text-outline"),
    NE("text-overflow"),
    NE("text-rendering"),
    NE("text-shadow"),
    NE("text-space-collapse"),
    NE("text-transform"),
    NE("text-underline-position"),
    NE("text-wrap"),
    NE("top"),
    NE("transform"),
    NE("transform-origin"),
    NE("transform-style"),
    NE("transition"),
    NE("transition-delay"),
    NE("transition-duration"),
    NE("transition-property"),
    NE("transition-timing-function"),
    NE("unicode-bidi"),
    NE("vertical-align"),
    NE("visibility"),
    NE("voice-balance"),
    NE("voice-duration"),
    NE("voice-family"),
    NE("voice-pitch"),
    NE("voice-pitch-range"),
    NE("voice-range"),
    NE("voice-rate"),
    NE("voice-stress"),
    NE("voice-volume"),
    NE("volume"),
    NE("white-space"),
    NE("widows"),
    NE("width"),
    NE("word-break"),
    NE("word-spacing"),
    NE("word-wrap"),
    NE("writing-mode"),
    NE("z-index"),
};

static named_element_t constants[] = {
    NE("aliceblue"),
    NE("antiquewhite"),
    NE("aqua"),
    NE("aquamarine"),
    NE("azure"),
    NE("beige"),
    NE("bisque"),
    NE("black"),
    NE("blanchedalmond"),
    NE("blue"),
    NE("blueviolet"),
    NE("brown"),
    NE("burlywood"),
    NE("cadetblue"),
    NE("chartreuse"),
    NE("chocolate"),
    NE("coral"),
    NE("cornflowerblue"),
    NE("cornsilk"),
    NE("crimson"),
    NE("cyan"),
    NE("darkblue"),
    NE("darkcyan"),
    NE("darkgoldenrod"),
    NE("darkgray"),
    NE("darkgreen"),
    NE("darkkhaki"),
    NE("darkmagenta"),
    NE("darkolivegreen"),
    NE("darkorange"),
    NE("darkorchid"),
    NE("darkred"),
    NE("darksalmon"),
    NE("darkseagreen"),
    NE("darkslateblue"),
    NE("darkslategray"),
    NE("darkturquoise"),
    NE("darkviolet"),
    NE("deeppink"),
    NE("deepskyblue"),
    NE("dimgray"),
    NE("dodgerblue"),
    NE("firebrick"),
    NE("floralwhite"),
    NE("forestgreen"),
    NE("fuchsia"),
    NE("gainsboro"),
    NE("ghostwhite"),
    NE("gold"),
    NE("goldenrod"),
    NE("gray"),
    NE("green"),
    NE("greenyellow"),
    NE("honeydew"),
    NE("hotpink"),
    NE("indianred"),
    NE("indigo"),
    NE("ivory"),
    NE("khaki"),
    NE("lavender"),
    NE("lavenderblush"),
    NE("lawngreen"),
    NE("lemonchiffon"),
    NE("lightblue"),
    NE("lightcoral"),
    NE("lightcyan"),
    NE("lightgoldenrodyellow"),
    NE("lightgreen"),
    NE("lightgrey"),
    NE("lightpink"),
    NE("lightsalmon"),
    NE("lightseagreen"),
    NE("lightskyblue"),
    NE("lightslategray"),
    NE("lightsteelblue"),
    NE("lightyellow"),
    NE("lime"),
    NE("limegreen"),
    NE("linen"),
    NE("magenta"),
    NE("maroon"),
    NE("mediumaquamarine"),
    NE("mediumblue"),
    NE("mediumorchid"),
    NE("mediumpurple"),
    NE("mediumseagreen"),
    NE("mediumslateblue"),
    NE("mediumspringgreen"),
    NE("mediumturquoise"),
    NE("mediumvioletred"),
    NE("midnightblue"),
    NE("mintcream"),
    NE("mistyrose"),
    NE("moccasin"),
    NE("navajowhite"),
    NE("navy"),
    NE("oldlace"),
    NE("olive"),
    NE("olivedrab"),
    NE("orange"),
    NE("orangered"),
    NE("orchid"),
    NE("palegoldenrod"),
    NE("palegreen"),
    NE("paleturquoise"),
    NE("palevioletred"),
    NE("papayawhip"),
    NE("peachpuff"),
    NE("peru"),
    NE("pink"),
    NE("plum"),
    NE("powderblue"),
    NE("purple"),
    NE("red"),
    NE("rosybrown"),
    NE("royalblue"),
    NE("saddlebrown"),
    NE("salmon"),
    NE("sandybrown"),
    NE("seagreen"),
    NE("seashell"),
    NE("sienna"),
    NE("silver"),
    NE("skyblue"),
    NE("slateblue"),
    NE("slategray"),
    NE("snow"),
    NE("springgreen"),
    NE("steelblue"),
    NE("tan"),
    NE("teal"),
    NE("thistle"),
    NE("tomato"),
    NE("turquoise"),
    NE("violet"),
    NE("wheat"),
    NE("white"),
    NE("whitesmoke"),
    NE("yellow"),
    NE("yellowgreen"),
};

static named_element_t builtins[] = {
    NE("above"),
    NE("absolute"),
    NE("always"),
    NE("armenian"),
    NE("aural"),
    NE("auto"),
    NE("avoid"),
    NE("baseline"),
    NE("behind"),
    NE("below"),
    NE("bidi-override"),
    NE("blink"),
    NE("block"),
    NE("bold"),
    NE("bolder"),
    NE("both"),
    NE("bottom"),
    NE("bottom"),
    NE("capitalize"),
    NE("center"),
    NE("center-left"),
    NE("center-right"),
    NE("circle"),
    NE("cjk-ideographic"),
    NE("close-quote"),
    NE("collapse"),
    NE("condensed"),
    NE("continuous"),
    NE("crop"),
    NE("cross"),
    NE("crosshair"),
    NE("cursive"),
    NE("dashed"),
    NE("decimal"),
    NE("decimal-leading-zero"),
    NE("default"),
    NE("digits"),
    NE("disc"),
    NE("dotted"),
    NE("double"),
    NE("e-resize"),
    NE("embed"),
    NE("expanded"),
    NE("extra-condensed"),
    NE("extra-expanded"),
    NE("fantasy"),
    NE("far-left"),
    NE("far-right"),
    NE("fast"),
    NE("faster"),
    NE("fixed"),
    NE("georgian"),
    NE("groove"),
    NE("hebrew"),
    NE("help"),
    NE("hidden"),
    NE("hide"),
    NE("high"),
    NE("higher"),
    NE("hiragana"),
    NE("hiragana-iroha"),
    NE("icon"),
    NE("inherit"),
    NE("inline"),
    NE("inline-table"),
    NE("inset"),
    NE("inside"),
    NE("invert"),
    NE("italic"),
    NE("justify"),
    NE("katakana"),
    NE("katakana-iroha"),
    NE("landscape"),
    NE("large"),
    NE("larger"),
    NE("left"),
    NE("left"),
    NE("left-side"),
    NE("leftwards"),
    NE("level"),
    NE("lighter"),
    NE("line-through"),
    NE("list-item"),
    NE("loud"),
    NE("low"),
    NE("lower"),
    NE("lower-alpha"),
    NE("lower-greek"),
    NE("lower-roman"),
    NE("lowercase"),
    NE("ltr"),
    NE("medium"),
    NE("message-box"),
    NE("middle"),
    NE("mix"),
    NE("monospace"),
    NE("n-resize"),
    NE("narrower"),
    NE("ne-resize"),
    NE("no-close-quote"),
    NE("no-open-quote"),
    NE("no-repeat"),
    NE("none"),
    NE("normal"),
    NE("nowrap"),
    NE("nw-resize"),
    NE("oblique"),
    NE("once"),
    NE("open-quote"),
    NE("outset"),
    NE("outside"),
    NE("overline"),
    NE("pointer"),
    NE("portrait"),
    NE("px"),
    NE("relative"),
    NE("repeat"),
    NE("repeat-x"),
    NE("repeat-y"),
    NE("rgb"),
    NE("ridge"),
    NE("right"),
    NE("right-side"),
    NE("rightwards"),
    NE("s-resize"),
    NE("sans-serif"),
    NE("scroll"),
    NE("se-resize"),
    NE("semi-condensed"),
    NE("semi-expanded"),
    NE("separate"),
    NE("serif"),
    NE("show"),
    NE("silent"),
    NE("slow"),
    NE("slower"),
    NE("small-caps"),
    NE("small-caption"),
    NE("smaller"),
    NE("soft"),
    NE("solid"),
    NE("spell-out"),
    NE("square"),
    NE("static"),
    NE("status-bar"),
    NE("super"),
    NE("sw-resize"),
    NE("table-caption"),
    NE("table-cell"),
    NE("table-column"),
    NE("table-column-group"),
    NE("table-footer-group"),
    NE("table-header-group"),
    NE("table-row"),
    NE("table-row-group"),
    NE("text"),
    NE("text-bottom"),
    NE("text-top"),
    NE("thick"),
    NE("thin"),
    NE("top"),
    NE("transparent"),
    NE("ultra-condensed"),
    NE("ultra-expanded"),
    NE("underline"),
    NE("upper-alpha"),
    NE("upper-latin"),
    NE("upper-roman"),
    NE("uppercase"),
    NE("url"),
    NE("visible"),
    NE("w-resize"),
    NE("wait"),
    NE("wider"),
    NE("x-fast"),
    NE("x-high"),
    NE("x-large"),
    NE("x-loud"),
    NE("x-low"),
    NE("x-small"),
    NE("x-soft"),
    NE("xx-large"),
    NE("xx-small"),
    NE("yes"),
};

static int csslex(YYLEX_ARGS) {
    (void) ctxt;
    (void) data;
    (void) options;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

w = [ \t\r\n\f]*;
num = [0-9]+ | [0-9]*"."[0-9]+;
nl = "\r\n" | [\r\n\f];
nonascii = [^\000-\177];
unicode = "\\"[0-9a-f]{1,6}("\r\n"|[ \n\r\t\f])?;

escape = unicode | "\\"[^\n\r\f0-9a-f];
nmstart = [_a-z] | nonascii | escape;
nmchar = [_a-z0-9-] | nonascii | escape;

ident = [-]? nmstart nmchar*;
name = nmchar+;

string1 = '"' ([^\n\r\f\\"] | "\\"nl | nonascii | escape)* '"';
string2 = "'" ([^\n\r\f\\'] | "\\"nl | nonascii | escape)* "'";
string = string1 | string2;
invalid1 = '"' ([^\n\r\f\\"] | "\\"nl | nonascii | escape)*;
invalid2 = "'" ([^\n\r\f\\'] | "\\"nl | nonascii | escape)*;
invalid = invalid1 | invalid2;

<INITIAL>"<!--" {
    TOKEN(IGNORABLE);
}

<INITIAL>"-->" {
    TOKEN(IGNORABLE);
}

<*> ":" ":"? ident {
    TOKEN(KEYWORD_PSEUDO);
}

<*>"/*" {
    if (STATE(IN_COMMENT) != YYSTATE) {
        PUSH_STATE(IN_COMMENT);
    }
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL>"{" {
    BEGIN(IN_CONTENT);
    TOKEN(PUNCTUATION);
}

<IN_CONTENT>"}" {
    BEGIN(INITIAL);
    TOKEN(PUNCTUATION);
}

<IN_COMMENT>[^] {
    TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT>"*/" {
    POP_STATE();
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL>[~|^$*]"=" {
    TOKEN(OPERATOR);
}

<IN_CONTENT>string {
    TOKEN(STRING_SINGLE);
}

<INITIAL>[,()] {
    TOKEN(PUNCTUATION);
}

<IN_CONTENT>[;()] {
    TOKEN(PUNCTUATION);
}

<IN_CONTENT>"!" w "important" {
    TOKEN(TAG_PREPROC);
}

<INITIAL>[~+>*|[\]] {
    TOKEN(OPERATOR);
}

<IN_CONTENT>"#" [0-9a-fA-F]{6} {
    TOKEN(NUMBER_DECIMAL);
}

<IN_CONTENT>ident {
    named_element_t key = { (char *) YYTEXT, YYLENG };

    if (NULL != bsearch(&key, builtins, ARRAY_SIZE(builtins), sizeof(builtins[0]), named_elements_cmp)) {
        TOKEN(NAME_BUILTIN);
    }
    TOKEN(KEYWORD);
}

<IN_CONTENT>ident ":" {
    named_element_t key = { (char *) YYTEXT, 0 };

    key.name_len = YYLENG - 1;
    yyless(key.name_len);
    if (NULL != bsearch(&key, attributes, ARRAY_SIZE(attributes), sizeof(attributes[0]), named_elements_cmp)) {
        TOKEN(NAME_BUILTIN);
    } else {
        size_t i;

        for (i = 0; i < ARRAY_SIZE(vendor_prefixes); i++) {
            if (0 == strncmp_l(vendor_prefixes[i].name, vendor_prefixes[i].name_len, key.name, key.name_len, vendor_prefixes[i].name_len)) {
                TOKEN(KEYWORD_BUILTIN);
            }
        }
    }
    TOKEN(KEYWORD);
}

// "@page" pseudo_page?
// "@charset" string
// "@media" media_list
<INITIAL>"@" ident {
    TOKEN(KEYWORD);
}

<INITIAL>"." ident {
    TOKEN(NAME_CLASS);
}

<INITIAL>"#" name {
    TOKEN(NAME_FUNCTION);
}

<INITIAL>ident {
    TOKEN(KEYWORD);
}

<INITIAL,IN_CONTENT>ident "(" {
    yyless(YYLENG - 1);
    TOKEN(NAME_FUNCTION);
}

<IN_CONTENT>num {
    TOKEN(NUMBER_DECIMAL);
}

// CSS2 defines em, ex, px, cm, mm, in, pt, pc, deg, rad, grad, ms, s, hz, khz (in all variants - case and escape character)
// but CSS3 is wider, unity is simply an "identifier"
<IN_CONTENT>num ("%" | ident) {
    TOKEN(NUMBER_DECIMAL);
}

<*>[^] {
    TOKEN(IGNORABLE);
}
*/
    }
    DONE();
}

LexerImplementation css_lexer = {
    "CSS",
    "For CSS (Cascading Style Sheets)",
    NULL,
    (const char * const []) { "*.css", NULL },
    (const char * const []) { "text/css", NULL },
    NULL, // interpreters
    NULL, // analyse
    NULL, // init
    csslex,
    NULL, // finalyze
    sizeof(LexerData),
    NULL, // options
    NULL // dependencies
};
