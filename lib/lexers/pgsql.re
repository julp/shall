/**
 * Portions Copyright (c) 1996-2015, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *    src/backend/parser/scan.l
 **/

#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "utils.h"
#include "tokens.h"
#include "lexer.h"

typedef struct {
    LexerData data;
    char *dolqstart;
    size_t dolqstart_len;
} PgLexerData;

typedef struct {
    int uppercase_keywords ALIGNED(sizeof(OptionValue));
    int standard_conforming_strings ALIGNED(sizeof(OptionValue));
} PgLexerOption;

enum {
    STATE(INITIAL),
    STATE(xb),     // bit string literal ([bB]'...')
    STATE(xc),     // extended C-style comments (/* ... */)
    STATE(xd),     // delimited identifiers (double-quoted identifiers) (standard "...")
    STATE(xh),     // hexadecimal numeric string ([xX]'...')
    STATE(xe),     // extended quoted strings (support backslash escape sequences) ([eE]'...' with \x[[:xdigit:]]{2}, \u[[:xdigit:]]{4}, \U[[:xdigit:]]{8}, \[0-7]{1,3} sequences)
    STATE(xq),     // standard quoted strings (standard '...')
    STATE(xdolq),  // dollar quoted strings (($([^$]*$)...\1)
    STATE(xui),    // quoted identifier with Unicode escapes ([uU]&"..." with \[[:xdigit:]]{4} or \+[[:xdigit:]]{6} - \ is the default escape character, it can be changed with a following UESCAPE)
    STATE(xuiend), // end of a quoted identifier with Unicode escapes, UESCAPE can follow
    STATE(xus),    // quoted string with Unicode escapes (simply the same as xui with ' as delimiter instead of ")
    STATE(xusend), // end of a quoted string with Unicode escapes, UESCAPE can follow
    STATE(xeu)     // Unicode surrogate pair in extended quoted string (just a "virtual" state to expect an UTF-16 trailing surrogate pair, shall don't check it?)
};

enum {
    UNRESERVED_KEYWORD,
    COL_NAME_KEYWORD,
    TYPE_FUNC_NAME_KEYWORD,
    RESERVED_KEYWORD
};

static const typed_named_element_t keywords[] = {
#define PG_TYPE(name) \
    { NE(name), KEYWORD_TYPE },
// PG_KEYWORD\(("[^"]*"),[^,]+,([^)]+)\) => PG_KEYWORD(\1,\2)
#define PG_KEYWORD(name, category) \
    { NE(name), KEYWORD },
PG_KEYWORD("abort", UNRESERVED_KEYWORD)
PG_KEYWORD("absolute", UNRESERVED_KEYWORD)
PG_KEYWORD("access", UNRESERVED_KEYWORD)
PG_KEYWORD("action", UNRESERVED_KEYWORD)
PG_KEYWORD("add", UNRESERVED_KEYWORD)
PG_KEYWORD("admin", UNRESERVED_KEYWORD)
PG_KEYWORD("after", UNRESERVED_KEYWORD)
PG_KEYWORD("aggregate", UNRESERVED_KEYWORD)
PG_KEYWORD("all", RESERVED_KEYWORD)
PG_KEYWORD("also", UNRESERVED_KEYWORD)
PG_KEYWORD("alter", UNRESERVED_KEYWORD)
PG_KEYWORD("always", UNRESERVED_KEYWORD)
PG_KEYWORD("analyse", RESERVED_KEYWORD)
PG_KEYWORD("analyze", RESERVED_KEYWORD)
PG_KEYWORD("and", RESERVED_KEYWORD)
PG_KEYWORD("any", RESERVED_KEYWORD)
PG_TYPE("anyarray")
PG_KEYWORD("array", RESERVED_KEYWORD)
PG_KEYWORD("as", RESERVED_KEYWORD)
PG_KEYWORD("asc", RESERVED_KEYWORD)
PG_KEYWORD("assertion", UNRESERVED_KEYWORD)
PG_KEYWORD("assignment", UNRESERVED_KEYWORD)
PG_KEYWORD("asymmetric", RESERVED_KEYWORD)
PG_KEYWORD("at", UNRESERVED_KEYWORD)
PG_KEYWORD("attribute", UNRESERVED_KEYWORD)
PG_KEYWORD("authorization", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("backward", UNRESERVED_KEYWORD)
PG_KEYWORD("before", UNRESERVED_KEYWORD)
PG_KEYWORD("begin", UNRESERVED_KEYWORD)
PG_KEYWORD("between", COL_NAME_KEYWORD)
PG_TYPE("bigint")
PG_TYPE("bigserial")
// PG_KEYWORD("bigint", COL_NAME_KEYWORD)
PG_KEYWORD("binary", TYPE_FUNC_NAME_KEYWORD)
PG_TYPE("bit") // TODO: bit varying
// PG_KEYWORD("bit", COL_NAME_KEYWORD)
PG_TYPE("bool")
PG_TYPE("boolean")
// PG_KEYWORD("boolean", COL_NAME_KEYWORD)
PG_KEYWORD("both", RESERVED_KEYWORD)
PG_TYPE("box")
PG_KEYWORD("by", UNRESERVED_KEYWORD)
PG_TYPE("bytea")
PG_KEYWORD("cache", UNRESERVED_KEYWORD)
PG_KEYWORD("called", UNRESERVED_KEYWORD)
PG_KEYWORD("cascade", UNRESERVED_KEYWORD)
PG_KEYWORD("cascaded", UNRESERVED_KEYWORD)
PG_KEYWORD("case", RESERVED_KEYWORD)
PG_KEYWORD("cast", RESERVED_KEYWORD)
PG_KEYWORD("catalog", UNRESERVED_KEYWORD)
PG_KEYWORD("chain", UNRESERVED_KEYWORD)
PG_KEYWORD("char", COL_NAME_KEYWORD)
// PG_KEYWORD("character", COL_NAME_KEYWORD)
PG_TYPE("character") // TODO: character varying
PG_KEYWORD("characteristics", UNRESERVED_KEYWORD)
PG_KEYWORD("check", RESERVED_KEYWORD)
PG_KEYWORD("checkpoint", UNRESERVED_KEYWORD)
PG_TYPE("cidr")
PG_TYPE("circle")
PG_TYPE("citext")
PG_KEYWORD("class", UNRESERVED_KEYWORD)
PG_KEYWORD("close", UNRESERVED_KEYWORD)
PG_KEYWORD("cluster", UNRESERVED_KEYWORD)
PG_KEYWORD("coalesce", COL_NAME_KEYWORD)
PG_KEYWORD("collate", RESERVED_KEYWORD)
PG_KEYWORD("collation", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("column", RESERVED_KEYWORD)
PG_KEYWORD("comment", UNRESERVED_KEYWORD)
PG_KEYWORD("comments", UNRESERVED_KEYWORD)
PG_KEYWORD("commit", UNRESERVED_KEYWORD)
PG_KEYWORD("committed", UNRESERVED_KEYWORD)
PG_KEYWORD("concurrently", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("configuration", UNRESERVED_KEYWORD)
PG_KEYWORD("connection", UNRESERVED_KEYWORD)
PG_KEYWORD("constraint", RESERVED_KEYWORD)
PG_KEYWORD("constraints", UNRESERVED_KEYWORD)
PG_KEYWORD("content", UNRESERVED_KEYWORD)
PG_KEYWORD("continue", UNRESERVED_KEYWORD)
PG_KEYWORD("conversion", UNRESERVED_KEYWORD)
PG_KEYWORD("copy", UNRESERVED_KEYWORD)
PG_KEYWORD("cost", UNRESERVED_KEYWORD)
PG_KEYWORD("create", RESERVED_KEYWORD)
PG_KEYWORD("cross", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("csv", UNRESERVED_KEYWORD)
PG_KEYWORD("current", UNRESERVED_KEYWORD)
PG_KEYWORD("current_catalog", RESERVED_KEYWORD)
PG_KEYWORD("current_date", RESERVED_KEYWORD)
PG_KEYWORD("current_role", RESERVED_KEYWORD)
PG_KEYWORD("current_schema", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("current_time", RESERVED_KEYWORD)
PG_KEYWORD("current_timestamp", RESERVED_KEYWORD)
PG_KEYWORD("current_user", RESERVED_KEYWORD)
PG_KEYWORD("cursor", UNRESERVED_KEYWORD)
PG_KEYWORD("cycle", UNRESERVED_KEYWORD)
PG_KEYWORD("data", UNRESERVED_KEYWORD)
PG_KEYWORD("database", UNRESERVED_KEYWORD)
PG_TYPE("date")
PG_KEYWORD("day", UNRESERVED_KEYWORD)
PG_KEYWORD("deallocate", UNRESERVED_KEYWORD)
PG_KEYWORD("dec", COL_NAME_KEYWORD)
// PG_KEYWORD("decimal", COL_NAME_KEYWORD)
PG_TYPE("decimal")
PG_KEYWORD("declare", UNRESERVED_KEYWORD)
PG_KEYWORD("default", RESERVED_KEYWORD)
PG_KEYWORD("defaults", UNRESERVED_KEYWORD)
PG_KEYWORD("deferrable", RESERVED_KEYWORD)
PG_KEYWORD("deferred", UNRESERVED_KEYWORD)
PG_KEYWORD("definer", UNRESERVED_KEYWORD)
PG_KEYWORD("delete", UNRESERVED_KEYWORD)
PG_KEYWORD("delimiter", UNRESERVED_KEYWORD)
PG_KEYWORD("delimiters", UNRESERVED_KEYWORD)
PG_KEYWORD("desc", RESERVED_KEYWORD)
PG_KEYWORD("dictionary", UNRESERVED_KEYWORD)
PG_KEYWORD("disable", UNRESERVED_KEYWORD)
PG_KEYWORD("discard", UNRESERVED_KEYWORD)
PG_KEYWORD("distinct", RESERVED_KEYWORD)
PG_KEYWORD("do", RESERVED_KEYWORD)
PG_KEYWORD("document", UNRESERVED_KEYWORD)
PG_KEYWORD("domain", UNRESERVED_KEYWORD)
PG_KEYWORD("double", UNRESERVED_KEYWORD)
PG_KEYWORD("drop", UNRESERVED_KEYWORD)
PG_KEYWORD("each", UNRESERVED_KEYWORD)
PG_KEYWORD("else", RESERVED_KEYWORD)
PG_KEYWORD("enable", UNRESERVED_KEYWORD)
PG_KEYWORD("encoding", UNRESERVED_KEYWORD)
PG_KEYWORD("encrypted", UNRESERVED_KEYWORD)
PG_KEYWORD("end", RESERVED_KEYWORD)
PG_KEYWORD("enum", UNRESERVED_KEYWORD)
PG_KEYWORD("escape", UNRESERVED_KEYWORD)
PG_KEYWORD("event", UNRESERVED_KEYWORD)
PG_KEYWORD("except", RESERVED_KEYWORD)
PG_KEYWORD("exclude", UNRESERVED_KEYWORD)
PG_KEYWORD("excluding", UNRESERVED_KEYWORD)
PG_KEYWORD("exclusive", UNRESERVED_KEYWORD)
PG_KEYWORD("execute", UNRESERVED_KEYWORD)
PG_KEYWORD("exists", COL_NAME_KEYWORD)
PG_KEYWORD("explain", UNRESERVED_KEYWORD)
PG_KEYWORD("extension", UNRESERVED_KEYWORD)
PG_KEYWORD("external", UNRESERVED_KEYWORD)
PG_KEYWORD("extract", COL_NAME_KEYWORD)
PG_KEYWORD("false", RESERVED_KEYWORD)
PG_KEYWORD("family", UNRESERVED_KEYWORD)
PG_KEYWORD("fetch", RESERVED_KEYWORD)
PG_KEYWORD("filter", UNRESERVED_KEYWORD)
PG_KEYWORD("first", UNRESERVED_KEYWORD)
// PG_KEYWORD("float", COL_NAME_KEYWORD)
PG_TYPE("float")
PG_TYPE("float4")
PG_TYPE("float8") // TODO: double precision
PG_KEYWORD("following", UNRESERVED_KEYWORD)
PG_KEYWORD("for", RESERVED_KEYWORD)
PG_KEYWORD("force", UNRESERVED_KEYWORD)
PG_KEYWORD("foreign", RESERVED_KEYWORD)
PG_KEYWORD("forward", UNRESERVED_KEYWORD)
PG_KEYWORD("freeze", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("from", RESERVED_KEYWORD)
PG_KEYWORD("full", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("function", UNRESERVED_KEYWORD)
PG_KEYWORD("functions", UNRESERVED_KEYWORD)
PG_KEYWORD("global", UNRESERVED_KEYWORD)
PG_KEYWORD("grant", RESERVED_KEYWORD)
PG_KEYWORD("granted", UNRESERVED_KEYWORD)
PG_KEYWORD("greatest", COL_NAME_KEYWORD)
PG_KEYWORD("group", RESERVED_KEYWORD)
PG_KEYWORD("handler", UNRESERVED_KEYWORD)
PG_KEYWORD("having", RESERVED_KEYWORD)
PG_KEYWORD("header", UNRESERVED_KEYWORD)
PG_KEYWORD("hold", UNRESERVED_KEYWORD)
PG_KEYWORD("hour", UNRESERVED_KEYWORD)
PG_KEYWORD("identity", UNRESERVED_KEYWORD)
PG_KEYWORD("if", UNRESERVED_KEYWORD)
PG_KEYWORD("ilike", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("immediate", UNRESERVED_KEYWORD)
PG_KEYWORD("immutable", UNRESERVED_KEYWORD)
PG_KEYWORD("implicit", UNRESERVED_KEYWORD)
PG_KEYWORD("import", UNRESERVED_KEYWORD)
PG_KEYWORD("in", RESERVED_KEYWORD)
PG_KEYWORD("including", UNRESERVED_KEYWORD)
PG_KEYWORD("increment", UNRESERVED_KEYWORD)
PG_KEYWORD("index", UNRESERVED_KEYWORD)
PG_KEYWORD("indexes", UNRESERVED_KEYWORD)
PG_TYPE("inet")
PG_KEYWORD("inherit", UNRESERVED_KEYWORD)
PG_KEYWORD("inherits", UNRESERVED_KEYWORD)
PG_KEYWORD("initially", RESERVED_KEYWORD)
PG_KEYWORD("inline", UNRESERVED_KEYWORD)
PG_KEYWORD("inner", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("inout", COL_NAME_KEYWORD)
PG_KEYWORD("input", UNRESERVED_KEYWORD)
PG_KEYWORD("insensitive", UNRESERVED_KEYWORD)
PG_KEYWORD("insert", UNRESERVED_KEYWORD)
PG_KEYWORD("instead", UNRESERVED_KEYWORD)
// PG_KEYWORD("int", COL_NAME_KEYWORD)
PG_TYPE("int")
PG_TYPE("int2")
PG_TYPE("int4")
PG_TYPE("int8")
PG_TYPE("integer")
// PG_KEYWORD("integer", COL_NAME_KEYWORD)
PG_KEYWORD("intersect", RESERVED_KEYWORD)
// PG_KEYWORD("interval", COL_NAME_KEYWORD)
PG_TYPE("interval")
PG_KEYWORD("into", RESERVED_KEYWORD)
PG_KEYWORD("invoker", UNRESERVED_KEYWORD)
PG_KEYWORD("is", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("isnull", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("isolation", UNRESERVED_KEYWORD)
PG_KEYWORD("join", TYPE_FUNC_NAME_KEYWORD)
PG_TYPE("json")
PG_TYPE("jsonb")
PG_KEYWORD("key", UNRESERVED_KEYWORD)
PG_KEYWORD("label", UNRESERVED_KEYWORD)
PG_KEYWORD("language", UNRESERVED_KEYWORD)
PG_KEYWORD("large", UNRESERVED_KEYWORD)
PG_KEYWORD("last", UNRESERVED_KEYWORD)
PG_KEYWORD("lateral", RESERVED_KEYWORD)
PG_KEYWORD("leading", RESERVED_KEYWORD)
PG_KEYWORD("leakproof", UNRESERVED_KEYWORD)
PG_KEYWORD("least", COL_NAME_KEYWORD)
PG_KEYWORD("left", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("level", UNRESERVED_KEYWORD)
PG_KEYWORD("like", TYPE_FUNC_NAME_KEYWORD)
PG_TYPE("line")
PG_KEYWORD("limit", RESERVED_KEYWORD)
PG_KEYWORD("listen", UNRESERVED_KEYWORD)
PG_KEYWORD("load", UNRESERVED_KEYWORD)
PG_KEYWORD("local", UNRESERVED_KEYWORD)
PG_KEYWORD("localtime", RESERVED_KEYWORD)
PG_KEYWORD("localtimestamp", RESERVED_KEYWORD)
PG_KEYWORD("location", UNRESERVED_KEYWORD)
PG_KEYWORD("lock", UNRESERVED_KEYWORD)
PG_KEYWORD("locked", UNRESERVED_KEYWORD)
PG_KEYWORD("logged", UNRESERVED_KEYWORD)
PG_TYPE("lseg")
PG_TYPE("macaddr")
PG_KEYWORD("mapping", UNRESERVED_KEYWORD)
PG_KEYWORD("match", UNRESERVED_KEYWORD)
PG_KEYWORD("materialized", UNRESERVED_KEYWORD)
PG_KEYWORD("maxvalue", UNRESERVED_KEYWORD)
PG_KEYWORD("minute", UNRESERVED_KEYWORD)
PG_KEYWORD("minvalue", UNRESERVED_KEYWORD)
PG_KEYWORD("mode", UNRESERVED_KEYWORD)
PG_TYPE("money")
PG_KEYWORD("month", UNRESERVED_KEYWORD)
PG_KEYWORD("move", UNRESERVED_KEYWORD)
PG_KEYWORD("name", UNRESERVED_KEYWORD)
PG_KEYWORD("names", UNRESERVED_KEYWORD)
PG_KEYWORD("national", COL_NAME_KEYWORD)
PG_KEYWORD("natural", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("nchar", COL_NAME_KEYWORD)
PG_KEYWORD("next", UNRESERVED_KEYWORD)
PG_KEYWORD("no", UNRESERVED_KEYWORD)
PG_KEYWORD("none", COL_NAME_KEYWORD)
PG_KEYWORD("not", RESERVED_KEYWORD)
PG_KEYWORD("nothing", UNRESERVED_KEYWORD)
PG_KEYWORD("notify", UNRESERVED_KEYWORD)
PG_KEYWORD("notnull", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("nowait", UNRESERVED_KEYWORD)
PG_KEYWORD("null", RESERVED_KEYWORD)
PG_KEYWORD("nullif", COL_NAME_KEYWORD)
PG_KEYWORD("nulls", UNRESERVED_KEYWORD)
// PG_KEYWORD("numeric", COL_NAME_KEYWORD)
PG_TYPE("numeric")
PG_KEYWORD("object", UNRESERVED_KEYWORD)
PG_KEYWORD("of", UNRESERVED_KEYWORD)
PG_KEYWORD("off", UNRESERVED_KEYWORD)
PG_KEYWORD("offset", RESERVED_KEYWORD)
PG_KEYWORD("oids", UNRESERVED_KEYWORD)
PG_KEYWORD("on", RESERVED_KEYWORD)
PG_KEYWORD("only", RESERVED_KEYWORD)
PG_KEYWORD("operator", UNRESERVED_KEYWORD)
PG_KEYWORD("option", UNRESERVED_KEYWORD)
PG_KEYWORD("options", UNRESERVED_KEYWORD)
PG_KEYWORD("or", RESERVED_KEYWORD)
PG_KEYWORD("order", RESERVED_KEYWORD)
PG_KEYWORD("ordinality", UNRESERVED_KEYWORD)
PG_KEYWORD("out", COL_NAME_KEYWORD)
PG_KEYWORD("outer", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("over", UNRESERVED_KEYWORD)
PG_KEYWORD("overlaps", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("overlay", COL_NAME_KEYWORD)
PG_KEYWORD("owned", UNRESERVED_KEYWORD)
PG_KEYWORD("owner", UNRESERVED_KEYWORD)
PG_KEYWORD("parser", UNRESERVED_KEYWORD)
PG_KEYWORD("partial", UNRESERVED_KEYWORD)
PG_KEYWORD("partition", UNRESERVED_KEYWORD)
PG_KEYWORD("passing", UNRESERVED_KEYWORD)
PG_KEYWORD("password", UNRESERVED_KEYWORD)
PG_TYPE("path")
PG_TYPE("pg_lsn")
PG_KEYWORD("placing", RESERVED_KEYWORD)
PG_KEYWORD("plans", UNRESERVED_KEYWORD)
PG_TYPE("point")
PG_KEYWORD("policy", UNRESERVED_KEYWORD)
PG_TYPE("polygon")
PG_KEYWORD("position", COL_NAME_KEYWORD)
PG_KEYWORD("preceding", UNRESERVED_KEYWORD)
PG_KEYWORD("precision", COL_NAME_KEYWORD)
PG_KEYWORD("prepare", UNRESERVED_KEYWORD)
PG_KEYWORD("prepared", UNRESERVED_KEYWORD)
PG_KEYWORD("preserve", UNRESERVED_KEYWORD)
PG_KEYWORD("primary", RESERVED_KEYWORD)
PG_KEYWORD("prior", UNRESERVED_KEYWORD)
PG_KEYWORD("privileges", UNRESERVED_KEYWORD)
PG_KEYWORD("procedural", UNRESERVED_KEYWORD)
PG_KEYWORD("procedure", UNRESERVED_KEYWORD)
PG_KEYWORD("program", UNRESERVED_KEYWORD)
PG_KEYWORD("quote", UNRESERVED_KEYWORD)
PG_KEYWORD("range", UNRESERVED_KEYWORD)
PG_KEYWORD("read", UNRESERVED_KEYWORD)
// PG_KEYWORD("real", COL_NAME_KEYWORD)
PG_TYPE("real")
PG_KEYWORD("reassign", UNRESERVED_KEYWORD)
PG_KEYWORD("recheck", UNRESERVED_KEYWORD)
PG_KEYWORD("recursive", UNRESERVED_KEYWORD)
PG_KEYWORD("ref", UNRESERVED_KEYWORD)
PG_KEYWORD("references", RESERVED_KEYWORD)
PG_KEYWORD("refresh", UNRESERVED_KEYWORD)
PG_KEYWORD("reindex", UNRESERVED_KEYWORD)
PG_KEYWORD("relative", UNRESERVED_KEYWORD)
PG_KEYWORD("release", UNRESERVED_KEYWORD)
PG_KEYWORD("rename", UNRESERVED_KEYWORD)
PG_KEYWORD("repeatable", UNRESERVED_KEYWORD)
PG_KEYWORD("replace", UNRESERVED_KEYWORD)
PG_KEYWORD("replica", UNRESERVED_KEYWORD)
PG_KEYWORD("reset", UNRESERVED_KEYWORD)
PG_KEYWORD("restart", UNRESERVED_KEYWORD)
PG_KEYWORD("restrict", UNRESERVED_KEYWORD)
PG_KEYWORD("returning", RESERVED_KEYWORD)
PG_KEYWORD("returns", UNRESERVED_KEYWORD)
PG_KEYWORD("revoke", UNRESERVED_KEYWORD)
PG_KEYWORD("right", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("role", UNRESERVED_KEYWORD)
PG_KEYWORD("rollback", UNRESERVED_KEYWORD)
PG_KEYWORD("row", COL_NAME_KEYWORD)
PG_KEYWORD("rows", UNRESERVED_KEYWORD)
PG_KEYWORD("rule", UNRESERVED_KEYWORD)
PG_KEYWORD("savepoint", UNRESERVED_KEYWORD)
PG_KEYWORD("schema", UNRESERVED_KEYWORD)
PG_KEYWORD("scroll", UNRESERVED_KEYWORD)
PG_KEYWORD("search", UNRESERVED_KEYWORD)
PG_KEYWORD("second", UNRESERVED_KEYWORD)
PG_KEYWORD("security", UNRESERVED_KEYWORD)
PG_KEYWORD("select", RESERVED_KEYWORD)
PG_KEYWORD("sequence", UNRESERVED_KEYWORD)
PG_KEYWORD("sequences", UNRESERVED_KEYWORD)
PG_TYPE("serial")
PG_TYPE("serial2")
PG_TYPE("serial4")
PG_TYPE("serial8")
PG_KEYWORD("serializable", UNRESERVED_KEYWORD)
PG_KEYWORD("server", UNRESERVED_KEYWORD)
PG_KEYWORD("session", UNRESERVED_KEYWORD)
PG_KEYWORD("session_user", RESERVED_KEYWORD)
PG_KEYWORD("set", UNRESERVED_KEYWORD)
PG_KEYWORD("setof", COL_NAME_KEYWORD)
PG_KEYWORD("share", UNRESERVED_KEYWORD)
PG_KEYWORD("show", UNRESERVED_KEYWORD)
PG_KEYWORD("similar", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("simple", UNRESERVED_KEYWORD)
PG_KEYWORD("skip", UNRESERVED_KEYWORD)
// PG_KEYWORD("smallint", COL_NAME_KEYWORD)
PG_TYPE("smallint")
PG_TYPE("smallserial")
PG_KEYWORD("snapshot", UNRESERVED_KEYWORD)
PG_KEYWORD("some", RESERVED_KEYWORD)
PG_KEYWORD("stable", UNRESERVED_KEYWORD)
PG_KEYWORD("standalone", UNRESERVED_KEYWORD)
PG_KEYWORD("start", UNRESERVED_KEYWORD)
PG_KEYWORD("statement", UNRESERVED_KEYWORD)
PG_KEYWORD("statistics", UNRESERVED_KEYWORD)
PG_KEYWORD("stdin", UNRESERVED_KEYWORD)
PG_KEYWORD("stdout", UNRESERVED_KEYWORD)
PG_KEYWORD("storage", UNRESERVED_KEYWORD)
PG_KEYWORD("strict", UNRESERVED_KEYWORD)
PG_KEYWORD("strip", UNRESERVED_KEYWORD)
PG_KEYWORD("substring", COL_NAME_KEYWORD)
PG_KEYWORD("symmetric", RESERVED_KEYWORD)
PG_KEYWORD("sysid", UNRESERVED_KEYWORD)
PG_KEYWORD("system", UNRESERVED_KEYWORD)
PG_KEYWORD("table", RESERVED_KEYWORD)
PG_KEYWORD("tables", UNRESERVED_KEYWORD)
PG_KEYWORD("tablespace", UNRESERVED_KEYWORD)
PG_KEYWORD("temp", UNRESERVED_KEYWORD)
PG_KEYWORD("template", UNRESERVED_KEYWORD)
PG_KEYWORD("temporary", UNRESERVED_KEYWORD)
// PG_KEYWORD("text", UNRESERVED_KEYWORD)
PG_TYPE("text")
PG_KEYWORD("then", RESERVED_KEYWORD)
// PG_KEYWORD("time", COL_NAME_KEYWORD)
PG_TYPE("time") // TODO: with(?:out)? time zone
// PG_KEYWORD("timestamp", COL_NAME_KEYWORD)
PG_TYPE("timestamp") // TODO: with(?:out)? time zone
PG_KEYWORD("to", RESERVED_KEYWORD)
PG_KEYWORD("trailing", RESERVED_KEYWORD)
PG_KEYWORD("transaction", UNRESERVED_KEYWORD)
PG_KEYWORD("treat", COL_NAME_KEYWORD)
PG_KEYWORD("trigger", UNRESERVED_KEYWORD)
PG_KEYWORD("trim", COL_NAME_KEYWORD)
PG_KEYWORD("true", RESERVED_KEYWORD)
PG_KEYWORD("truncate", UNRESERVED_KEYWORD)
PG_KEYWORD("trusted", UNRESERVED_KEYWORD)
PG_TYPE("tsquery")
PG_TYPE("tsvector")
PG_TYPE("txid_snapshot")
PG_KEYWORD("type", UNRESERVED_KEYWORD)
PG_KEYWORD("types", UNRESERVED_KEYWORD)
PG_KEYWORD("unbounded", UNRESERVED_KEYWORD)
PG_KEYWORD("uncommitted", UNRESERVED_KEYWORD)
PG_KEYWORD("unencrypted", UNRESERVED_KEYWORD)
PG_KEYWORD("union", RESERVED_KEYWORD)
PG_KEYWORD("unique", RESERVED_KEYWORD)
PG_KEYWORD("unknown", UNRESERVED_KEYWORD)
PG_KEYWORD("unlisten", UNRESERVED_KEYWORD)
PG_KEYWORD("unlogged", UNRESERVED_KEYWORD)
PG_KEYWORD("until", UNRESERVED_KEYWORD)
PG_KEYWORD("update", UNRESERVED_KEYWORD)
PG_KEYWORD("user", RESERVED_KEYWORD)
PG_KEYWORD("using", RESERVED_KEYWORD)
PG_TYPE("uuid")
PG_KEYWORD("vacuum", UNRESERVED_KEYWORD)
PG_KEYWORD("valid", UNRESERVED_KEYWORD)
PG_KEYWORD("validate", UNRESERVED_KEYWORD)
PG_KEYWORD("validator", UNRESERVED_KEYWORD)
PG_KEYWORD("value", UNRESERVED_KEYWORD)
PG_KEYWORD("values", COL_NAME_KEYWORD)
PG_TYPE("varbit")
// PG_KEYWORD("varchar", COL_NAME_KEYWORD)
PG_TYPE("varchar")
PG_KEYWORD("variadic", RESERVED_KEYWORD)
PG_KEYWORD("varying", UNRESERVED_KEYWORD)
PG_KEYWORD("verbose", TYPE_FUNC_NAME_KEYWORD)
PG_KEYWORD("version", UNRESERVED_KEYWORD)
PG_KEYWORD("view", UNRESERVED_KEYWORD)
PG_KEYWORD("views", UNRESERVED_KEYWORD)
PG_KEYWORD("volatile", UNRESERVED_KEYWORD)
PG_KEYWORD("when", RESERVED_KEYWORD)
PG_KEYWORD("where", RESERVED_KEYWORD)
PG_KEYWORD("whitespace", UNRESERVED_KEYWORD)
PG_KEYWORD("window", RESERVED_KEYWORD)
PG_KEYWORD("with", RESERVED_KEYWORD)
PG_KEYWORD("within", UNRESERVED_KEYWORD)
PG_KEYWORD("without", UNRESERVED_KEYWORD)
PG_KEYWORD("work", UNRESERVED_KEYWORD)
PG_KEYWORD("wrapper", UNRESERVED_KEYWORD)
PG_KEYWORD("write", UNRESERVED_KEYWORD)
// PG_KEYWORD("xml", UNRESERVED_KEYWORD)
PG_TYPE("xml")
PG_KEYWORD("xmlattributes", COL_NAME_KEYWORD)
PG_KEYWORD("xmlconcat", COL_NAME_KEYWORD)
PG_KEYWORD("xmlelement", COL_NAME_KEYWORD)
PG_KEYWORD("xmlexists", COL_NAME_KEYWORD)
PG_KEYWORD("xmlforest", COL_NAME_KEYWORD)
PG_KEYWORD("xmlparse", COL_NAME_KEYWORD)
PG_KEYWORD("xmlpi", COL_NAME_KEYWORD)
PG_KEYWORD("xmlroot", COL_NAME_KEYWORD)
PG_KEYWORD("xmlserialize", COL_NAME_KEYWORD)
PG_KEYWORD("year", UNRESERVED_KEYWORD)
PG_KEYWORD("yes", UNRESERVED_KEYWORD)
PG_KEYWORD("zone", UNRESERVED_KEYWORD)
#undef PG_KEYWORD
#undef PG_TYPE
};

static void pgfinalize(LexerData *data)
{
    PgLexerData *mydata;

    mydata = (PgLexerData *) data;
    if (NULL != mydata->dolqstart) {
        free(mydata->dolqstart);
    }
}

static int pglex(YYLEX_ARGS)
{
    PgLexerData *mydata;
    const PgLexerOption *myoptions;

    mydata = (PgLexerData *) data;
    myoptions = (const PgLexerOption *) options;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;

/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

space = [ \t\n\r\f];
horiz_space = [ \t\f];
newline = [\n\r];
non_newline = [^\n\r];
comment = ("--" non_newline*);
whitespace = (space+ | comment);

self = [,()\[\].;\:\+\-\*\/\%\^\<\>\=];
op_chars = [\~\!\@\#\^\&\|\`\?\+\-\*\/\%\<\>\=];
operator = op_chars+;

special_whitespace = (space+ | comment newline);
horiz_whitespace = (horiz_space | comment);
whitespace_with_newline = (horiz_whitespace* newline special_whitespace*);

quote = ['];
quotestop = quote whitespace*;
quotecontinue = quote whitespace_with_newline quote;
quotefail = quote whitespace* "-";

xbstart = [bB] quote;
xbinside = [^']*;
// should be xbinside = [01]*; but we don't take care of string content (same as pgsql, it checks this at a different level)

xhstart = [xX] quote;
xhinside = [^']*;
// should be xhinside = [0-9a-fA-F]*; but we don't take care of string content (same as pgsql, it checks this at a different level)

xnstart = [nN] quote;

xestart = [eE] quote;
xeinside = [^\\']+;
xeescape = [\\][^0-7];
xeoctesc = [\\][0-7]{1,3};
xehexesc = [\\][x][0-9A-Fa-f]{1,2};
xeunicode = [\\]([u][0-9A-Fa-f]{4}|[U][0-9A-Fa-f]{8});
xeunicodefail = [\\]([u][0-9A-Fa-f]{0,3}|[U][0-9A-Fa-f]{0,7});

xqstart = quote;
xqdouble = quote quote;
xqinside = [^']+;

dolq_start = [A-Za-z\200-\377_];
dolq_cont = [A-Za-z\200-\377_0-9];
dolqdelim = [$] (dolq_start dolq_cont*)? [$];
dolqfailed = [$] dolq_start dolq_cont*;
dolqinside = [^$]+;

dquote = ["];
xdstart = dquote;
xdstop = dquote;
xddouble = dquote dquote;
xdinside = [^"]+;

uescape = 'uescape' whitespace* quote [^'] quote;
uescapefail = 'uescape' whitespace* "-" | 'uescape' whitespace* quote [^'] | 'uescape' whitespace* quote | 'uescape' whitespace* | 'uescap' | 'uesca' | 'uesc' | 'ues' | 'ue' | 'u';

xuistart = [uU][&]dquote;
xusstart = [uU][&]quote;
xustop1 = uescapefail?;
xustop2 = uescape;
xufailed = [uU][&];

xcstart = "/*" op_chars*;
xcstop = "*"+"/";
xcinside = [^*/]+;

digit = [0-9];
ident_start = [A-Za-z\200-\377_];
ident_cont = [A-Za-z\200-\377_0-9\$];
identifier = ident_start ident_cont*;

typecast = "::";
dot_dot = "..";
colon_equals = ":=";
equals_greater = "=>";
less_equals = "<=";
greater_equals = ">=";
less_greater = "<>";
not_equals = "!=";

integer = digit+;
decimal = ((digit* "." digit+) | (digit+ "." digit*));
decimalfail = digit+ "..";
real = (integer|decimal)[Ee][-+]?digit+;
realfail1 = (integer|decimal)[Ee];
realfail2 = (integer|decimal)[Ee][-+];

param = "$" integer;

other = .;

/**
 * Difference with original pgsql lexer: split whitespace in two distinct cases:
 * - comment
 * - space
 * Needed to highlight oneline comment.
 **/
<INITIAL> comment {
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> space {
    TOKEN(IGNORABLE);
}

<INITIAL> xcstart {
    BEGIN(xc);
    yyless(2);
    TOKEN(COMMENT_MULTILINE);
}

<xc> xcstart {
    yyless(2);
    TOKEN(COMMENT_MULTILINE);
}

<xc> xcstop {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

<xc> xcinside {
    TOKEN(COMMENT_MULTILINE);
}

<xc> op_chars  {
    TOKEN(COMMENT_MULTILINE);
}

<xc> [*]+ {
    TOKEN(COMMENT_MULTILINE);
}

// <xc><<EOF>>

<INITIAL> xbstart {
    BEGIN(xb);
    TOKEN(NUMBER_BINARY);
}

<xb> quotestop | quotefail {
    yyless(1);
    BEGIN(INITIAL);
    TOKEN(NUMBER_BINARY);
}

<xb> xbinside {
    TOKEN(NUMBER_BINARY);
}

<xh> xhinside {
    TOKEN(NUMBER_HEXADECIMAL);
}

// <xh>{quotecontinue} | <xb>{quotecontinue}

// <xb><<EOF>>

<INITIAL> xhstart {
    BEGIN(xh);
    TOKEN(NUMBER_HEXADECIMAL);
}

<xh> quotestop | quotefail {
    yyless(1);
    BEGIN(INITIAL);
    TOKEN(NUMBER_HEXADECIMAL);
}

// <xh><<EOF>>

<INITIAL> xnstart {
    // ?
}

<INITIAL> xqstart {
    if (myoptions->standard_conforming_strings) {
        BEGIN(xq);
    } else {
        BEGIN(xe);
    }
    TOKEN(STRING_SINGLE);
}

<INITIAL> xestart {
    BEGIN(xe);
    TOKEN(STRING_SINGLE);
}

<INITIAL> xusstart {
//     if (!myoptions->standard_conforming_strings)
    BEGIN(xus);
    TOKEN(STRING_SINGLE);
}

<xq,xe> quotestop | quotefail {
    yyless(1);
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<xus> quotestop | quotefail {
    yyless(1);
    BEGIN(xusend);
    TOKEN(STRING_SINGLE);
}

<xusend> whitespace {
    TOKEN(STRING_SINGLE);
}

<xusend> other | xustop1 {
    yyless(0);
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<xusend> xustop2 {
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<xq,xe,xus> xqdouble {
    TOKEN(STRING_SINGLE);
}

<xq,xus> xqinside {
    TOKEN(STRING_SINGLE);
}

<xe> xeinside {
    TOKEN(STRING_SINGLE);
}

<xe> xeunicode {
#if 0
    if (is_utf16_surrogate_first(c)) {
        BEGIN(xeu);
    } else {
        // invalid
    }
#else
//     BEGIN(xeu);
#endif
    TOKEN(ESCAPED_CHAR);
}

<xeu> xeunicode {
    BEGIN(xe);
    TOKEN(ESCAPED_CHAR);
}

// <xeu> .
// <xeu> \n
// <xeu> <<EOF>>
// <xe,xeu> xeunicodefail

<xe> xeescape {
    TOKEN(ESCAPED_CHAR);
}

<xe> xeoctesc {
    TOKEN(ESCAPED_CHAR);
}

<xe> xehexesc {
    TOKEN(ESCAPED_CHAR);
}

<xq,xe,xus> quotecontinue {
    TOKEN(STRING_SINGLE);
}

<xe> . {
    TOKEN(STRING_SINGLE);
}

// <xq,xe,xus><<EOF>>

<INITIAL> dolqdelim {
    BEGIN(xdolq);
    mydata->dolqstart_len = YYCURSOR - YYTEXT;
    mydata->dolqstart = strndup((char *) YYTEXT, mydata->dolqstart_len);
    TOKEN(STRING_SINGLE);
}

<INITIAL> dolqfailed {
    yyless(1);
    TOKEN(IGNORABLE);
}

<xdolq> dolqdelim {
    if (((size_t) (YYCURSOR - YYTEXT)) == mydata->dolqstart_len && 0 == memcmp(YYTEXT, mydata->dolqstart, mydata->dolqstart_len)) {
        free(mydata->dolqstart);
        mydata->dolqstart = NULL;
        BEGIN(INITIAL);
    } else {
        yyless((YYCURSOR - YYTEXT) - 1);
    }
    TOKEN(STRING_SINGLE);
}

<xdolq> dolqinside {
    TOKEN(STRING_SINGLE);
}

<xdolq> dolqfailed {
    TOKEN(STRING_SINGLE);
}

<xdolq> . {
    TOKEN(STRING_SINGLE);
}

// <xdolq><<EOF>>

<INITIAL> xdstart {
    BEGIN(xd);
    TOKEN(STRING_SINGLE);
}

<INITIAL> xuistart {
    BEGIN(xui);
    TOKEN(STRING_SINGLE);
}

<xd> xdstop {
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<xui> dquote {
    yyless(1);
    BEGIN(xuiend);
    TOKEN(STRING_SINGLE);
}

<xuiend> whitespace {
    TOKEN(IGNORABLE);
}

<xuiend> other | xustop1 {
    yyless(0);
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<xuiend> xustop2 {
    BEGIN(INITIAL);
    TOKEN(STRING_SINGLE);
}

<xd,xui> xddouble  {
    TOKEN(STRING_SINGLE);
}

<xd,xui> xdinside {
    TOKEN(STRING_SINGLE);
}

// <xd,xui><<EOF>>

<INITIAL> xufailed {
    yyless(1);
    TOKEN(STRING_SINGLE);
}

<INITIAL> typecast | dot_dot | colon_equals | equals_greater | less_equals | greater_equals | less_greater | not_equals | self {
    TOKEN(OPERATOR);
}

<INITIAL> operator {
    // TODO: stricter
    TOKEN(OPERATOR);
}

<INITIAL> param {
    TOKEN(NAME_VARIABLE);
}

<INITIAL> integer {
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> decimal {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> decimalfail {
    yyless((YYCURSOR - YYTEXT) - 2);
    TOKEN(NUMBER_DECIMAL);
}

<INITIAL> real {
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> realfail1 {
    yyless((YYCURSOR - YYTEXT) - 1);
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> realfail2 {
    yyless((YYCURSOR - YYTEXT) - 2);
    TOKEN(NUMBER_FLOAT);
}

<INITIAL> identifier {
    named_element_t key = { (char *) YYTEXT, YYLENG };
    typed_named_element_t *match;

    if (NULL != (match = bsearch(&key, keywords, ARRAY_SIZE(keywords), sizeof(keywords[0]), named_elements_casecmp))) {
        if (myoptions->uppercase_keywords/* && KEYWORD == match->type*/) {
            YYCTYPE *p;

            for (p = YYTEXT; p <= YYCURSOR; p++) {
                *p = ascii_toupper((int) *p);
            }
        }
        TOKEN(match->type);
    } else {
        TOKEN(IGNORABLE);
    }
}

<INITIAL> other {
    TOKEN(IGNORABLE);
}

// <<EOF>>
*/
    }
    DONE();
}

LexerImplementation postgresql_lexer = {
    "PostgreSQL",
    "Lexer for the PostgreSQL dialect of SQL",
    (const char * const []) { "pgsql", "postgre", NULL },
    NULL, // "*.sql" but it may conflict with future mysql & co?
    (const char * const []) { "text/x-postgresql", NULL },
    NULL,
    NULL,
    NULL,
    pglex,
    sizeof(PgLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { "uppercase_keywords",          OPT_TYPE_BOOL, offsetof(PgLexerOption, uppercase_keywords),          OPT_DEF_BOOL(0), "when true, PostgreSQL keywords are uppercased" },
        { "standard_conforming_strings", OPT_TYPE_BOOL, offsetof(PgLexerOption, standard_conforming_strings), OPT_DEF_BOOL(1), "To treat backslashes literally in ordinary string literals (`'...'`) or not" },
        END_OF_LEXER_OPTIONS
    },
    NULL
};
