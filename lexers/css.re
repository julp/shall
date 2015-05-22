#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "lexer-private.h"
#include "utils.h"

enum {
    STATE(INITIAL),
    STATE(IN_COMMENT),
    STATE(IN_CONTENT),
    STATE(IN_STRING_SINGLE),
    STATE(IN_STRING_DOUBLE),
};

typedef struct {
    const char *name;
    size_t name_len;
} named_element_t;

#define I(s) { s, STR_LEN(s) }

static named_element_t vendor_prefixes[] = {
    I("-ah-"),
    I("-atsc-"),
    I("-hp-"),
    I("-khtml-"),
    I("-moz-"),
    I("-ms-"),
    I("-o-"),
    I("-rim-"),
    I("-ro-"),
    I("-tc-"),
    I("-wap-"),
    I("-webkit-"),
    I("-xv-"),
    I("mso-"),
    I("prince-")
};

static named_element_t attributes[] = {
    I("align-content"),
    I("align-items"),
    I("align-self"),
    I("alignment-adjust"),
    I("alignment-baseline"),
    I("all"),
    I("anchor-point"),
    I("animation"),
    I("animation-delay"),
    I("animation-direction"),
    I("animation-duration"),
    I("animation-fill-mode"),
    I("animation-iteration-count"),
    I("animation-name"),
    I("animation-play-state"),
    I("animation-timing-function"),
    I("appearance"),
    I("azimuth"),
    I("backface-visibility"),
    I("background"),
    I("background-attachment"),
    I("background-clip"),
    I("background-color"),
    I("background-image"),
    I("background-origin"),
    I("background-position"),
    I("background-repeat"),
    I("background-size"),
    I("baseline-shift"),
    I("binding"),
    I("bleed"),
    I("bookmark-label"),
    I("bookmark-level"),
    I("bookmark-state"),
    I("bookmark-target"),
    I("border"),
    I("border-bottom"),
    I("border-bottom-color"),
    I("border-bottom-left-radius"),
    I("border-bottom-right-radius"),
    I("border-bottom-style"),
    I("border-bottom-width"),
    I("border-collapse"),
    I("border-color"),
    I("border-image"),
    I("border-image-outset"),
    I("border-image-repeat"),
    I("border-image-slice"),
    I("border-image-source"),
    I("border-image-width"),
    I("border-left"),
    I("border-left-color"),
    I("border-left-style"),
    I("border-left-width"),
    I("border-radius"),
    I("border-right"),
    I("border-right-color"),
    I("border-right-style"),
    I("border-right-width"),
    I("border-spacing"),
    I("border-style"),
    I("border-top"),
    I("border-top-color"),
    I("border-top-left-radius"),
    I("border-top-right-radius"),
    I("border-top-style"),
    I("border-top-width"),
    I("border-width"),
    I("bottom"),
    I("box-align"),
    I("box-decoration-break"),
    I("box-direction"),
    I("box-flex"),
    I("box-flex-group"),
    I("box-lines"),
    I("box-ordinal-group"),
    I("box-orient"),
    I("box-pack"),
    I("box-shadow"),
    I("box-sizing"),
    I("break-after"),
    I("break-before"),
    I("break-inside"),
    I("caption-side"),
    I("clear"),
    I("clip"),
    I("clip-path"),
    I("clip-rule"),
    I("color"),
    I("color-profile"),
    I("column-count"),
    I("column-fill"),
    I("column-gap"),
    I("column-rule"),
    I("column-rule-color"),
    I("column-rule-style"),
    I("column-rule-width"),
    I("column-span"),
    I("column-width"),
    I("columns"),
    I("content"),
    I("counter-increment"),
    I("counter-reset"),
    I("crop"),
    I("cue"),
    I("cue-after"),
    I("cue-before"),
    I("cursor"),
    I("direction"),
    I("display"),
    I("dominant-baseline"),
    I("drop-initial-after-adjust"),
    I("drop-initial-after-align"),
    I("drop-initial-before-adjust"),
    I("drop-initial-before-align"),
    I("drop-initial-size"),
    I("drop-initial-value"),
    I("elevation"),
    I("empty-cells"),
    I("filter"),
    I("fit"),
    I("fit-position"),
    I("flex"),
    I("flex-basis"),
    I("flex-direction"),
    I("flex-flow"),
    I("flex-grow"),
    I("flex-shrink"),
    I("flex-wrap"),
    I("float"),
    I("float-offset"),
    I("font"),
    I("font-family"),
    I("font-feature-settings"),
    I("font-kerning"),
    I("font-language-override"),
    I("font-size"),
    I("font-size-adjust"),
    I("font-stretch"),
    I("font-style"),
    I("font-synthesis"),
    I("font-variant"),
    I("font-variant-alternates"),
    I("font-variant-caps"),
    I("font-variant-east-asian"),
    I("font-variant-ligatures"),
    I("font-variant-numeric"),
    I("font-variant-position"),
    I("font-weight"),
    I("grid-cell"),
    I("grid-column"),
    I("grid-column-align"),
    I("grid-column-sizing"),
    I("grid-column-span"),
    I("grid-columns"),
    I("grid-flow"),
    I("grid-row"),
    I("grid-row-align"),
    I("grid-row-sizing"),
    I("grid-row-span"),
    I("grid-rows"),
    I("grid-template"),
    I("hanging-punctuation"),
    I("height"),
    I("hyphenate-after"),
    I("hyphenate-before"),
    I("hyphenate-character"),
    I("hyphenate-lines"),
    I("hyphenate-resource"),
    I("hyphens"),
    I("icon"),
    I("image-orientation"),
    I("image-rendering"),
    I("image-resolution"),
    I("ime-mode"),
    I("inline-box-align"),
    I("justify-content"),
    I("left"),
    I("letter-spacing"),
    I("line-break"),
    I("line-height"),
    I("line-stacking"),
    I("line-stacking-ruby"),
    I("line-stacking-shift"),
    I("line-stacking-strategy"),
    I("list-style"),
    I("list-style-image"),
    I("list-style-position"),
    I("list-style-type"),
    I("margin"),
    I("margin-bottom"),
    I("margin-left"),
    I("margin-right"),
    I("margin-top"),
    I("mark"),
    I("mark-after"),
    I("mark-before"),
    I("marker-offset"),
    I("marks"),
    I("marquee-direction"),
    I("marquee-loop"),
    I("marquee-play-count"),
    I("marquee-speed"),
    I("marquee-style"),
    I("mask"),
    I("max-height"),
    I("max-width"),
    I("min-height"),
    I("min-width"),
    I("move-to"),
    I("nav-down"),
    I("nav-index"),
    I("nav-left"),
    I("nav-right"),
    I("nav-up"),
    I("object-fit"),
    I("object-position"),
    I("opacity"),
    I("order"),
    I("orphans"),
    I("outline"),
    I("outline-color"),
    I("outline-offset"),
    I("outline-style"),
    I("outline-width"),
    I("overflow"),
    I("overflow-style"),
    I("overflow-wrap"),
    I("overflow-x"),
    I("overflow-y"),
    I("padding"),
    I("padding-bottom"),
    I("padding-left"),
    I("padding-right"),
    I("padding-top"),
    I("page"),
    I("page-break-after"),
    I("page-break-before"),
    I("page-break-inside"),
    I("page-policy"),
    I("pause"),
    I("pause-after"),
    I("pause-before"),
    I("perspective"),
    I("perspective-origin"),
    I("phonemes"),
    I("pitch"),
    I("pitch-range"),
    I("play-during"),
    I("pointer-events"),
    I("position"),
    I("presentation-level"),
    I("punctuation-trim"),
    I("quotes"),
    I("rendering-intent"),
    I("resize"),
    I("rest"),
    I("rest-after"),
    I("rest-before"),
    I("richness"),
    I("right"),
    I("rotation"),
    I("rotation-point"),
    I("ruby-align"),
    I("ruby-overhang"),
    I("ruby-position"),
    I("ruby-span"),
    I("size"),
    I("speak"),
    I("speak-as"),
    I("speak-header"),
    I("speak-numeral"),
    I("speak-punctuation"),
    I("speech-rate"),
    I("src"),
    I("stress"),
    I("string-set"),
    I("tab-size"),
    I("table-layout"),
    I("target"),
    I("target-name"),
    I("target-new"),
    I("target-position"),
    I("text-align"),
    I("text-align-last"),
    I("text-combine-horizontal"),
    I("text-decoration"),
    I("text-decoration-color"),
    I("text-decoration-line"),
    I("text-decoration-skip"),
    I("text-decoration-style"),
    I("text-emphasis"),
    I("text-emphasis-color"),
    I("text-emphasis-position"),
    I("text-emphasis-style"),
    I("text-height"),
    I("text-indent"),
    I("text-justify"),
    I("text-orientation"),
    I("text-outline"),
    I("text-overflow"),
    I("text-rendering"),
    I("text-shadow"),
    I("text-space-collapse"),
    I("text-transform"),
    I("text-underline-position"),
    I("text-wrap"),
    I("top"),
    I("transform"),
    I("transform-origin"),
    I("transform-style"),
    I("transition"),
    I("transition-delay"),
    I("transition-duration"),
    I("transition-property"),
    I("transition-timing-function"),
    I("unicode-bidi"),
    I("vertical-align"),
    I("visibility"),
    I("voice-balance"),
    I("voice-duration"),
    I("voice-family"),
    I("voice-pitch"),
    I("voice-pitch-range"),
    I("voice-range"),
    I("voice-rate"),
    I("voice-stress"),
    I("voice-volume"),
    I("volume"),
    I("white-space"),
    I("widows"),
    I("width"),
    I("word-break"),
    I("word-spacing"),
    I("word-wrap"),
    I("writing-mode"),
    I("z-index"),
};

static named_element_t constants[] = {
    I("aliceblue"),
    I("antiquewhite"),
    I("aqua"),
    I("aquamarine"),
    I("azure"),
    I("beige"),
    I("bisque"),
    I("black"),
    I("blanchedalmond"),
    I("blue"),
    I("blueviolet"),
    I("brown"),
    I("burlywood"),
    I("cadetblue"),
    I("chartreuse"),
    I("chocolate"),
    I("coral"),
    I("cornflowerblue"),
    I("cornsilk"),
    I("crimson"),
    I("cyan"),
    I("darkblue"),
    I("darkcyan"),
    I("darkgoldenrod"),
    I("darkgray"),
    I("darkgreen"),
    I("darkkhaki"),
    I("darkmagenta"),
    I("darkolivegreen"),
    I("darkorange"),
    I("darkorchid"),
    I("darkred"),
    I("darksalmon"),
    I("darkseagreen"),
    I("darkslateblue"),
    I("darkslategray"),
    I("darkturquoise"),
    I("darkviolet"),
    I("deeppink"),
    I("deepskyblue"),
    I("dimgray"),
    I("dodgerblue"),
    I("firebrick"),
    I("floralwhite"),
    I("forestgreen"),
    I("fuchsia"),
    I("gainsboro"),
    I("ghostwhite"),
    I("gold"),
    I("goldenrod"),
    I("gray"),
    I("green"),
    I("greenyellow"),
    I("honeydew"),
    I("hotpink"),
    I("indianred"),
    I("indigo"),
    I("ivory"),
    I("khaki"),
    I("lavender"),
    I("lavenderblush"),
    I("lawngreen"),
    I("lemonchiffon"),
    I("lightblue"),
    I("lightcoral"),
    I("lightcyan"),
    I("lightgoldenrodyellow"),
    I("lightgreen"),
    I("lightgrey"),
    I("lightpink"),
    I("lightsalmon"),
    I("lightseagreen"),
    I("lightskyblue"),
    I("lightslategray"),
    I("lightsteelblue"),
    I("lightyellow"),
    I("lime"),
    I("limegreen"),
    I("linen"),
    I("magenta"),
    I("maroon"),
    I("mediumaquamarine"),
    I("mediumblue"),
    I("mediumorchid"),
    I("mediumpurple"),
    I("mediumseagreen"),
    I("mediumslateblue"),
    I("mediumspringgreen"),
    I("mediumturquoise"),
    I("mediumvioletred"),
    I("midnightblue"),
    I("mintcream"),
    I("mistyrose"),
    I("moccasin"),
    I("navajowhite"),
    I("navy"),
    I("oldlace"),
    I("olive"),
    I("olivedrab"),
    I("orange"),
    I("orangered"),
    I("orchid"),
    I("palegoldenrod"),
    I("palegreen"),
    I("paleturquoise"),
    I("palevioletred"),
    I("papayawhip"),
    I("peachpuff"),
    I("peru"),
    I("pink"),
    I("plum"),
    I("powderblue"),
    I("purple"),
    I("red"),
    I("rosybrown"),
    I("royalblue"),
    I("saddlebrown"),
    I("salmon"),
    I("sandybrown"),
    I("seagreen"),
    I("seashell"),
    I("sienna"),
    I("silver"),
    I("skyblue"),
    I("slateblue"),
    I("slategray"),
    I("snow"),
    I("springgreen"),
    I("steelblue"),
    I("tan"),
    I("teal"),
    I("thistle"),
    I("tomato"),
    I("turquoise"),
    I("violet"),
    I("wheat"),
    I("white"),
    I("whitesmoke"),
    I("yellow"),
    I("yellowgreen"),
};

static named_element_t builtins[] = {
    I("above"),
    I("absolute"),
    I("always"),
    I("armenian"),
    I("aural"),
    I("auto"),
    I("avoid"),
    I("baseline"),
    I("behind"),
    I("below"),
    I("bidi-override"),
    I("blink"),
    I("block"),
    I("bold"),
    I("bolder"),
    I("both"),
    I("bottom"),
    I("bottom"),
    I("capitalize"),
    I("center"),
    I("center-left"),
    I("center-right"),
    I("circle"),
    I("cjk-ideographic"),
    I("close-quote"),
    I("collapse"),
    I("condensed"),
    I("continuous"),
    I("crop"),
    I("cross"),
    I("crosshair"),
    I("cursive"),
    I("dashed"),
    I("decimal"),
    I("decimal-leading-zero"),
    I("default"),
    I("digits"),
    I("disc"),
    I("dotted"),
    I("double"),
    I("e-resize"),
    I("embed"),
    I("expanded"),
    I("extra-condensed"),
    I("extra-expanded"),
    I("fantasy"),
    I("far-left"),
    I("far-right"),
    I("fast"),
    I("faster"),
    I("fixed"),
    I("georgian"),
    I("groove"),
    I("hebrew"),
    I("help"),
    I("hidden"),
    I("hide"),
    I("high"),
    I("higher"),
    I("hiragana"),
    I("hiragana-iroha"),
    I("icon"),
    I("inherit"),
    I("inline"),
    I("inline-table"),
    I("inset"),
    I("inside"),
    I("invert"),
    I("italic"),
    I("justify"),
    I("katakana"),
    I("katakana-iroha"),
    I("landscape"),
    I("large"),
    I("larger"),
    I("left"),
    I("left"),
    I("left-side"),
    I("leftwards"),
    I("level"),
    I("lighter"),
    I("line-through"),
    I("list-item"),
    I("loud"),
    I("low"),
    I("lower"),
    I("lower-alpha"),
    I("lower-greek"),
    I("lower-roman"),
    I("lowercase"),
    I("ltr"),
    I("medium"),
    I("message-box"),
    I("middle"),
    I("mix"),
    I("monospace"),
    I("n-resize"),
    I("narrower"),
    I("ne-resize"),
    I("no-close-quote"),
    I("no-open-quote"),
    I("no-repeat"),
    I("none"),
    I("normal"),
    I("nowrap"),
    I("nw-resize"),
    I("oblique"),
    I("once"),
    I("open-quote"),
    I("outset"),
    I("outside"),
    I("overline"),
    I("pointer"),
    I("portrait"),
    I("px"),
    I("relative"),
    I("repeat"),
    I("repeat-x"),
    I("repeat-y"),
    I("rgb"),
    I("ridge"),
    I("right"),
    I("right-side"),
    I("rightwards"),
    I("s-resize"),
    I("sans-serif"),
    I("scroll"),
    I("se-resize"),
    I("semi-condensed"),
    I("semi-expanded"),
    I("separate"),
    I("serif"),
    I("show"),
    I("silent"),
    I("slow"),
    I("slower"),
    I("small-caps"),
    I("small-caption"),
    I("smaller"),
    I("soft"),
    I("solid"),
    I("spell-out"),
    I("square"),
    I("static"),
    I("status-bar"),
    I("super"),
    I("sw-resize"),
    I("table-caption"),
    I("table-cell"),
    I("table-column"),
    I("table-column-group"),
    I("table-footer-group"),
    I("table-header-group"),
    I("table-row"),
    I("table-row-group"),
    I("text"),
    I("text-bottom"),
    I("text-top"),
    I("thick"),
    I("thin"),
    I("top"),
    I("transparent"),
    I("ultra-condensed"),
    I("ultra-expanded"),
    I("underline"),
    I("upper-alpha"),
    I("upper-latin"),
    I("upper-roman"),
    I("uppercase"),
    I("url"),
    I("visible"),
    I("w-resize"),
    I("wait"),
    I("wider"),
    I("x-fast"),
    I("x-high"),
    I("x-large"),
    I("x-loud"),
    I("x-low"),
    I("x-small"),
    I("x-soft"),
    I("xx-large"),
    I("xx-small"),
    I("yes"),
};

static int named_elements_cmp(const void *a, const void *b)
{
    const named_element_t *na, *nb;

    na = (const named_element_t *) a; /* key */
    nb = (const named_element_t *) b;

    return strcmp_l(na->name, na->name_len, nb->name, nb->name_len);
}

static int csslex(YYLEX_ARGS) {
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;

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
    return IGNORABLE;
}

<INITIAL>"-->" {
    return IGNORABLE;
}

<INITIAL>"/*" {
    BEGIN(IN_COMMENT); // TODO: PUSH
    return COMMENT_MULTILINE;
}

<INITIAL>"{" {
    BEGIN(IN_CONTENT);
    return PUNCTUATION;
}

<IN_CONTENT>"}" {
    BEGIN(INITIAL);
    return PUNCTUATION;
}

<IN_COMMENT>[^] {
    return COMMENT_MULTILINE;
}

<IN_COMMENT>"*/" {
    BEGIN(INITIAL); // TODO: POP
    return COMMENT_MULTILINE;
}

<INITIAL>[~|^$*]"=" {
    return OPERATOR;
}

<IN_CONTENT>string {
    return STRING_SINGLE;
}

<INITIAL>[,()] {
    return PUNCTUATION;
}

<IN_CONTENT>[;()] {
    return PUNCTUATION;
}

<IN_CONTENT>"!" w "important" {
    return TAG_PREPROC;
}

<INITIAL>[~+>*|[\]] {
    return OPERATOR;
}

<IN_CONTENT>"#" [0-9a-fA-F]{6} {
    return NUMBER_DECIMAL;
}

<IN_CONTENT>ident {
    named_element_t key = { (char *) YYTEXT, YYLENG };

    if (NULL != bsearch(&key, builtins, ARRAY_SIZE(builtins), sizeof(builtins[0]), named_elements_cmp)) {
        return NAME_BUILTIN;
    }
    return KEYWORD;
}

<IN_CONTENT>ident ":" {
    named_element_t key = { (char *) YYTEXT, 0 };

    key.name_len = YYLENG - 1;
    yyless(key.name_len);
    if (NULL != bsearch(&key, attributes, ARRAY_SIZE(attributes), sizeof(attributes[0]), named_elements_cmp)) {
        return NAME_BUILTIN;
    } else {
        size_t i;

        for (i = 0; i < ARRAY_SIZE(vendor_prefixes); i++) {
            if (0 == strncmp_l(vendor_prefixes[i].name, vendor_prefixes[i].name_len, key.name, key.name_len, vendor_prefixes[i].name_len)) {
                return KEYWORD_BUILTIN;
            }
        }
    }
    return KEYWORD;
}

<INITIAL>"." name {
    return NAME_CLASS;
}

<INITIAL>"#" name {
    return NAME_FUNCTION;
}

<INITIAL>ident {
    return KEYWORD;
}

<INITIAL,IN_CONTENT>ident "(" {
    yyless(YYLENG - 1);
    return NAME_FUNCTION;
}

<IN_CONTENT>num {
    return NUMBER_DECIMAL;
}

<IN_CONTENT>num ("%" | ident) {
    return NUMBER_DECIMAL;
}

<*>[^] {
    return IGNORABLE;
}
*/
}

LexerImplementation css_lexer = {
    "CSS",
    "TODO",
    NULL,
    (const char * const []) { "*.css", NULL },
    (const char * const []) { "text/css", NULL },
    NULL,
    NULL,
    NULL,
    csslex,
    sizeof(LexerData),
    NULL
};
