/***
This package wraps the GLib library fairly thinly.
Unlike other lua-glib implementations, this is not meant to be a
stepping stone for GTK+ support.  It is meant to be a portability
and utility library.  There are other projects which provide
support for GTK+ and other various derivatives of GLib.

No support is provided for Type Conversion Macros, Byte Order Macros,
Numerical Definitions, or Miscellaneous Macros, as they are all
either too low-level or C-specific.
No support is provided for Atomic Operations, Threads, Thread Pools, or
Asynchronous queues; use a Lua-specific threading module instead
(such as Lanes).  No support is provided for The Main Event Loop, as
its utility versus implementation effort seems low.  No support is
provided for Dynamic Loading of Modules, because lua already does
that.  No support is provided for Memory Allocate or Memory Slices,
since you shouldn't be doing that in Lua.  IO Channels also seem
below my minimum utility-to-effort ratio.  There is no direct support
for the Error Reporting, Message Output, and Debugging Functions.
There is no support for replacing the log handlers or setting the
fatality flags in log messages, although that may come some day.
The String Utility Functions are C-specific, and not supported.
The Hook Functions seem useless for Lua.  The Lexical Scanner is
not very configurable and hard to bind to Lua; use a made-for-Lua
scanner instead like Lpeg.

The only Standard Macros supported are the OS and path separator
macros.  Date and Time Functions are mostly unsupported; the only
one which could not be replaced by a native Lua time library is
usleep, which is supported.  The GTimeZone and GDateTime sections
are not supported for the same reason.

The sections which are mostly supported are Version Information,
Character Set Conversion (including streaming), Unicode
Manipulation (skipping the low-level support functions and others
which would really need low-level Lua Unicode support), Base64
Encoding (including streaming), Data Checksums (including
streaming), Secure HMAC Digets (including streaming),
Internationalization (including gettext macros, but excluding some
of the low-level support functions), Miscellaneous Utility
Functions (except for bit operations, which really need to be added
to lua-bit, and a few other low-level/internal-use type functions),
Timers, Spawning Processes (including some extra features),
File Utilities
@module glib
*/

/* ldoc.lua -p lua-glib -o lua-glib -d /tmp -f markdown -t 'Lua GLib Library' lua-glib.c */

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/* this is the only exported function: */
int luaopen_glib(lua_State *L);

#ifdef _MSC_VER
#include <io.h>
/* MS VS 2005 deprecatesregular versions in favor of _ versions */
#if _MSC_VER >= 1400
#ifdef dup
#undef dup
#endif
#ifdef dup2
#undef dup2
#endif
#ifdef write
#undef write
#endif
#ifdef read
#undef read
#endif
#ifdef close
#undef close
#endif
#define dup _dup
#define dup2 _dup2
#define write _write
#define read _read
#define close _close
#endif
#else
#include <unistd.h>
#endif

/* Some of this was taken from cmorris' lua-glib */
/* but this has flat structure and many more glib functions */

#define LGLIB_IO_BUFSIZE 8192

#define alloc_udata(L, v, t) \
    t *v = (t *)lua_newuserdata(L, sizeof(t)); \
    luaL_getmetatable(L, "glib."#t); \
    memset(v, 0, sizeof(t)); \
    lua_setmetatable(L, -2)

#define get_udata(L, n, v, t) \
    t *v = (t *)luaL_checkudata(L, n, "glib."#t)

/* compare against structure w/ name as 1st element */
static int ns_cmp(const void *a, const void *b)
{
    return strcmp((const char *)a, *(const char * const *)b);
}

/***
Message Logging
@section Message Logging
*/

static struct lf {
    const gchar *name;
    GLogLevelFlags level;
} log_flags[] = {
    {"crit", G_LOG_LEVEL_CRITICAL},
    {"critical", G_LOG_LEVEL_CRITICAL},
    {"debug", G_LOG_LEVEL_DEBUG},
    {"err", G_LOG_LEVEL_ERROR},
    {"error", G_LOG_LEVEL_ERROR},
    {"info", G_LOG_LEVEL_INFO},
    {"message", G_LOG_LEVEL_MESSAGE},
    {"msg", G_LOG_LEVEL_MESSAGE},
    {"warn", G_LOG_LEVEL_WARNING},
    {"warning", G_LOG_LEVEL_WARNING}
};

/***
This is a wrapper for `g_log()`.
@function log
@tparam string domain (optional) The log domain.  This parameter may be absent
 to use `G_LOG_DOMAIN`.
@tparam string level (optional) The log level.  Acceptable values are:

  - 'crit/'critical'
  - 'debug'
  - 'err'/'error'
  - 'info'
  - 'msg'/'message'
  - 'warn'/'warning'

This parameter may be absent along with *domain* to indicate 'msg'.
@tparam string msg The log message.  Unlike g_log(), no formatting is
 permitted.  Collect and format your string before logging.
*/

static int glib_log(lua_State *L)
{
    const gchar *dom, *lev, *msg;
    int narg = lua_gettop(L);
    struct lf *levf;

    if(!narg)
	luaL_argerror(L, 1, "expected argument");
    if(narg == 1) {
	dom = G_LOG_DOMAIN;
	lev = "msg";
	msg = luaL_checkstring(L, 1);
    } else if(narg == 2) {
	dom = G_LOG_DOMAIN;
	lev = luaL_checkstring(L, 1);
	msg = luaL_checkstring(L, 2);
    } else {
	dom = luaL_checkstring(L, 1);
	lev = luaL_checkstring(L, 2);
	msg = luaL_checkstring(L, 3);
    }
    levf = bsearch(lev, log_flags, sizeof(log_flags)/sizeof(log_flags[0]),
		   sizeof(log_flags[0]), ns_cmp);
    luaL_argcheck(L, levf != NULL, narg - 1, "expected one of err/crit/warn/msg/info/debug");
    g_log(dom, levf->level, "%s", msg);
    return 0;
}

/***
Character Set Conversion
@section Character Set Conversion
*/

typedef struct convert_state {
    GIConv conv;
    GString in_buf, out_buf;
} convert_state;

static int free_convert_state(lua_State *L)
{
    get_udata(L, 1, st, convert_state);
    if(st->conv)
	g_iconv_close(st->conv);
    if(st->in_buf.str)
	g_free(st->in_buf.str);
    if(st->out_buf.str)
	g_free(st->out_buf.str);
    return 0;
}

/***
Stream character conversion function returned by convert.
This function is returned by convert to support converting
streams piecewise.  Simply call with string arguments, accumulating
the returned strings.  When finished with the stream, call with
no arguments.  This will return the final string to append and reset
the stream for reuse.
@function _convert_
@see convert
@tparam string str (optional) The next piece of the string to convert;
 absent to finish conversion
@treturn string The next piece of the converted string
*/

static int stream_convert(lua_State *L)
{
    const char *s;
    size_t sz;
    get_udata(L, lua_upvalueindex(1), st, convert_state);
    if(!lua_gettop(L)) {
	s = NULL;
	sz = 0;
    } else
	/* normal */
	s = luaL_checklstring(L, 1, &sz);
    if(s && st->in_buf.len) {
	g_string_append_len(&st->in_buf, s, sz);
	sz = st->in_buf.len;
    }
    if(!st->out_buf.allocated_len)
	g_string_set_size(&st->out_buf, sz > 32 ? sz : 32);
    {
	gchar *inb = !s ? NULL : st->in_buf.len ? st->in_buf.str : (gchar *)s, *outb;
	gsize insz = sz, outsz, ret;
	while(1) {
	    outb = st->out_buf.str;
	    outsz = st->out_buf.allocated_len;
	    ret = g_iconv(st->conv, &inb, &insz, &outb, &outsz);
	    if(ret >= 0 || errno != E2BIG)
		break;
	    g_string_set_size(&st->out_buf, st->out_buf.allocated_len * 2 - 1);
	}
	if(ret < 0)
	    lua_pushnil(L);
	else
	    lua_pushlstring(L, st->out_buf.str, (ptrdiff_t)(outb - st->out_buf.str));
	g_string_set_size(&st->in_buf, 0);
	if(s && insz != 0)
	    g_string_append_len(&st->in_buf, inb, insz);
	if(!s || ret < 0) {
	    g_iconv(st->conv, NULL, NULL, NULL, NULL);
	    if(st->in_buf.str)
		g_free(st->in_buf.str);
	    if(st->out_buf.str)
		g_free(st->out_buf.str);
	    memset(&st->in_buf, 0, sizeof(st->in_buf));
	    memset(&st->out_buf, 0, sizeof(st->out_buf));
	}
    }
    return 1;
}

/***
Convert strings from one character set to another.
This is a wrapper for `g_convert()` and friends.  To convert a stream,
pass in no arguments or nil for str.  The return value is either
the converted string, or a function matching the _convert_ type
below.
@function convert
@see _convert_
@tparam string str (optional) The string to convert, or nil/absent to produce a
 streaming converter
@tparam string to (optional) The target character set. This may be nil or
 absent to indicate the current locale.  This may be 'filename' to indicate
 the filename character set.
@tparam string from (optional) The target character set. This may be nil or
 absent to indicate the current locale.  This may be 'filename' to
 indicate the filename character set.
@tparam string fallback (optional) Any characters in *from* which have no
 equivalent in *to* are converted to this string.  This is not
 supported in stream mode.
@treturn string Converted string, if *str* was specified
@treturn function stream convert function, if *str* was nil or missing
@treturn nil on errors, followed by error message string
*/
static int glib_convert(lua_State *L)
{
    int narg = lua_gettop(L);
    const char *s, *from = NULL, *to = NULL;
    size_t slen = 0;
    GError *err = NULL;
    char *ret;
    gsize retlen;

    if(narg < 2 || lua_isnil(L, 2))
	g_get_charset(&to);
    else
	to = luaL_checkstring(L, 2);
    if(narg < 3 || lua_isnil(L, 3))
	g_get_charset(&from);
    else
	from = luaL_checkstring(L, 3);
    if(!strcasecmp(from, "filename")) {
	const gchar **fncs;
	g_get_filename_charsets(&fncs);
	from = fncs[0];
    }
    if(!strcasecmp(to, "filename")) {
	const gchar **fncs;
	g_get_filename_charsets(&fncs);
	to = fncs[0];
    }
    if(narg == 0 || lua_isnil(L, 1)) {
	/* there's no way I'm repeating glib's fallback code */
	/* it does the following: */
	/*   -> if a straight convert succeeds, return that */
	/*   -> otherwise, convert from->utf8->to */
	/*      utf8->to is done 1 char at a time, replacing failures */
	/* the only way to support this in a stream is to always assume */
	/* failures will occur, forcing dual char-by-char conversions */
	if(narg > 3)
	    luaL_argerror(L, 4, "fallback not supported for streams");
	else {
	    alloc_udata(L, st, convert_state);
	    st->conv = g_iconv_open(to, from);
	    lua_pushcclosure(L, stream_convert, 1);
	    return 1;
	}
    }
    s = luaL_checklstring(L, 1, &slen);
    if(narg < 4 || lua_isnil(L, 4))
	ret = g_convert(s, slen, to, from, NULL, &retlen, &err);
    else
	ret = g_convert_with_fallback(s, slen, to, from,
				      luaL_checkstring(L, 4), NULL, &retlen,
				      &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	return 2;
    } else {
	lua_pushlstring(L, ret, retlen);
	g_free(ret);
	return 1;
    }
}

/***
Unicode Manipulation
@section Unicode Manipulation
*/

#define one_unichar() \
    gunichar ch; \
    luaL_argcheck(L, lua_gettop(L) == 1, 1, "one character expected"); \
    if(lua_isnumber(L, 1)) \
	ch = lua_tonumber(L, 1); \
    else { \
	size_t sz; \
	const char *s = luaL_checklstring(L, 1, &sz); \
	ch = g_utf8_get_char_validated(s, sz); \
	luaL_argcheck(L, ch >= 0, 1, "invalid UTF-8 string"); \
    }

#define uni_bool(n) \
static int glib_##n(lua_State *L) \
{ \
    one_unichar(); \
    lua_pushboolean(L, g_unichar_##n(ch)); \
    return 1; \
}

/***
Check if unicode character is valid.
This is a wrapper for `g_unichar_validate()`.
@function validate
@tparam string|number c The Unicode character, as either an integer or a
 utf-8 string.
@treturn boolean True if *c* is valid
*/
uni_bool(validate)
/***
Check if Unicode character is alphabetic.
This is a wrapper for `g_unichar_isalpha()`.
@function isalpha
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is alphabetic
*/
uni_bool(isalpha)
/***
Check if Unicode character is a control character.
This is a wrapper for `g_unichar_iscntrl()`.
@function iscntrl
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is a control character
*/
uni_bool(iscntrl)
/***
Check if Unicode character is explicitly defined in the Unicode standard.
This is a wrapper for `g_unichar_isdefined()`.
@function isdefined
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is defined
*/
uni_bool(isdefined)
/***
Check if Unicode character is a decimal digit.
This is a wrapper for `g_unichar_isdigit()`.
@function isdigit
@see isxdigit
@see digit_value
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is a digit-like character
*/
uni_bool(isdigit)
/***
Check if Unicode character is a visible, printable character.
This is a wrapper for `g_unichar_isgraph()`.
@function isgraph
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is a graphic character
*/
uni_bool(isgraph)
/***
Check if Unicode character is a lower-case alphabetic character.
This is a wrapper for `g_unichar_islower()`.
@function islower
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is lower-case.
*/
uni_bool(islower)
/***
Check if Unicode character is a mark.
This is a wrapper for `g_unichar_ismark()`.
@function ismark
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is a non-spacing mark, combining mark, or
 enclosing mark
*/
uni_bool(ismark)
/***
Check if Unicode character is printable.
This is a wrapper for `g_unichar_isprint()`.
@function isprint
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is printable, even if blank
*/
uni_bool(isprint)
/***
Check if Unicode character is a punctuation or symbol character.
This is a wrapper for `g_unichar_ispunct()`.
@function ispunct
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is a punctuation or symbol character
*/
uni_bool(ispunct)
/***
Check if Unicode character is whitespace.
This is a wrapper for `g_unichar_isspace()`.
@function isspace
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is whitespace
*/
uni_bool(isspace)
/***
Check if Unicode character is titlecase.
This is a wrapper for `g_unichar_istitle()`.
@function istitle
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is titlecase alphabetic
*/
uni_bool(istitle)
/***
Check if Unicode character is upper-case.
This is a wrapper for `g_unichar_isupper()`.
@function isupper
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is upper-case alphabetic
*/
uni_bool(isupper)
/***
Check if Unicode character is hexadecimal digit.
This is a wrapper for `g_unichar_isxdigit()`.
@function isxdigit
@see isdigit
@see xdigit_value
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is a hexadecimal digit
*/
uni_bool(isxdigit)
/***
Check if Unicode character is wide.
This is a wrapper for `g_unichar_iswide()`.
@function iswide
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is double-width
*/
uni_bool(iswide)
/***
Check if Unicode character is wide in legacy East Asian locales.
This is a wrapper for `g_unichar_iswide_cjk()`.
@function iswide_cjk
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is double-width normally or in legacy
 East-Asian locales
*/
uni_bool(iswide_cjk)
/***
Check if Unicode character is zero-width.
This is a wrapper for `g_unichar_iszerowidth()`.
@function iszerowidth
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn boolean True if *c* is a zero-width combining character
*/
uni_bool(iszerowidth)

#define uni_int_ch(n) \
static int glib_##n(lua_State *L) \
{ \
    one_unichar(); \
    if(lua_isnumber(L, 1)) \
	lua_pushnumber(L, g_unichar_##n(ch)); \
    else { \
	char buf[6]; \
	int len = g_unichar_to_utf8(g_unichar_##n(ch), buf); \
	lua_pushlstring(L, buf, len); \
    } \
    return 1; \
}

/***
Convert Unicode character to upper-case.
This is a wrapper for `g_unichar_toupper()`.
@function toupper
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn string|number The result of conversion, as either an integer or a
 utf-8 string, depending on what *c* was.
*/
uni_int_ch(toupper)
/***
Convert Unicode character to lower-case.
This is a wrapper for `g_unichar_tolower()`.
@function tolower
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn string|number The result of conversion, as either an integer or a
 utf-8 string, depending on what *c* was.
*/
uni_int_ch(tolower)
/***
Convert Unicode character to title case.
This is a wrapper for `g_unichar_totitle()`.
@function totitle
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn string|number The result of conversion, as either an integer or a
 utf-8 string, depending on what *c* was.
*/
uni_int_ch(totitle)

#define uni_int(n) \
static int glib_##n(lua_State *L) \
{ \
    one_unichar(); \
    lua_pushnumber(L, g_unichar_##n(ch)); \
    return 1; \
}

/***
Convert digit to its numeric value.
This is a wrapper for `g_unichar_digit_value()`.
@function digit_value
@see isdigit
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn number The digit's numeric value, or -1 if *c* is not a digit.
*/
uni_int(digit_value)
/***
Convert hexadecimal digit to its numeric value.
This is a wrapper for `g_unichar_xdigit_value()`.
@function xdigit_value
@see isxdigit
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn number The hex digit's numeric value, or -1 if *c* is not a hex digit.
*/
uni_int(xdigit_value)

/***
Find Unicode character class.
This is a wrapper for `g_unichar_type()`.
@function type
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn string The character's class; one of:
    'control', 'format', 'unassigned', 'private_use', 'surrogate',
    'lowercase_letter', 'modifier_letter', 'other_letter', 'titlecase_letter',
    'uppercase_letter', 'spacing_mark', 'enclosing_mark', 'non_spacing_mark',
    'decimal_number', 'letter_number', 'other_number', 'connect_punctuation',
    'dash_punctuation', 'close_punctuation', 'final_punctuation',
    'initial_punctuation', 'other_punctuation', 'open_punctuation',
    'currency_symbol', 'modifier_symbol', 'math_symbol', 'other_symbol',
    'line_separator', 'paragraph_separator', 'space_separator'
*/
static int glib_type(lua_State *L)
{
    const char *t;
    one_unichar();
    switch(g_unichar_type(ch)) {
      case G_UNICODE_CONTROL: t = "control"; break;
      case G_UNICODE_FORMAT: t = "format"; break;
      case G_UNICODE_UNASSIGNED: t = "unassigned"; break;
      case G_UNICODE_PRIVATE_USE: t = "private_use"; break;
      case G_UNICODE_SURROGATE: t = "surrogate"; break;
      case G_UNICODE_LOWERCASE_LETTER: t = "lowercase_letter"; break;
      case G_UNICODE_MODIFIER_LETTER: t = "modifier_letter"; break;
      case G_UNICODE_OTHER_LETTER: t = "other_letter"; break;
      case G_UNICODE_TITLECASE_LETTER: t = "titlecase_letter"; break;
      case G_UNICODE_UPPERCASE_LETTER: t = "uppercase_letter"; break;
      case G_UNICODE_SPACING_MARK: t = "spacing_mark"; break;
      case G_UNICODE_ENCLOSING_MARK: t = "enclosing_mark"; break;
      case G_UNICODE_NON_SPACING_MARK: t = "non_spacing_mark"; break;
      case G_UNICODE_DECIMAL_NUMBER: t = "decimal_number"; break;
      case G_UNICODE_LETTER_NUMBER: t = "letter_number"; break;
      case G_UNICODE_OTHER_NUMBER: t = "other_number"; break;
      case G_UNICODE_CONNECT_PUNCTUATION: t = "connect_punctuation"; break;
      case G_UNICODE_DASH_PUNCTUATION: t = "dash_punctuation"; break;
      case G_UNICODE_CLOSE_PUNCTUATION: t = "close_punctuation"; break;
      case G_UNICODE_FINAL_PUNCTUATION: t = "final_punctuation"; break;
      case G_UNICODE_INITIAL_PUNCTUATION: t = "initial_punctuation"; break;
      case G_UNICODE_OTHER_PUNCTUATION: t = "other_punctuation"; break;
      case G_UNICODE_OPEN_PUNCTUATION: t = "open_punctuation"; break;
      case G_UNICODE_CURRENCY_SYMBOL: t = "currency_symbol"; break;
      case G_UNICODE_MODIFIER_SYMBOL: t = "modifier_symbol"; break;
      case G_UNICODE_MATH_SYMBOL: t = "math_symbol"; break;
      case G_UNICODE_OTHER_SYMBOL: t = "other_symbol"; break;
      case G_UNICODE_LINE_SEPARATOR: t = "line_separator"; break;
      case G_UNICODE_PARAGRAPH_SEPARATOR: t = "paragraph_separator"; break;
      case G_UNICODE_SPACE_SEPARATOR: t = "space_separator"; break;
      default: t = "unknown"; break;
    }
    lua_pushstring(L, t);
    return 1;
}

/***
Find Unicode character's line break classification.
This is a wrapper for `g_unichar_break_type()`.
@function break_type
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn string The line break classification; one of:
    'mandatory', 'carriage_return', 'line_feed', 'combining_mark', 'surrogate',
    'zero_width_space', 'inseperable', 'non_breaking_glue', 'contingent',
    'space', 'after', 'before', 'before_and_after', 'hyphen', 'non_starter',
    'open_punctuation', 'close_punctuation', 'quotation', 'exclamation',
    'ideographic', 'numeric', 'infix_separator', 'symbol', 'alphabetic',
    'prefix', 'postfix', 'complex_context', 'ambiguous', 'unknown', 'next_line',
    'word_joiner', 'hangul_l_jamo', 'hangul_v_jamo', 'hangul_t_jamo',
    'hangul_lv_syllable', 'hangul_lvt_syllable', 'close_parenthesis',
    'conditional_japanese_starter', 'hebrew_letter'
*/
static int glib_break_type(lua_State *L)
{
    const char *t;
    one_unichar();
    switch(g_unichar_break_type(ch)) {
      case G_UNICODE_BREAK_MANDATORY: t = "mandatory"; break;
      case G_UNICODE_BREAK_CARRIAGE_RETURN: t = "carriage_return"; break;
      case G_UNICODE_BREAK_LINE_FEED: t = "line_feed"; break;
      case G_UNICODE_BREAK_COMBINING_MARK: t = "combining_mark"; break;
      case G_UNICODE_BREAK_SURROGATE: t = "surrogate"; break;
      case G_UNICODE_BREAK_ZERO_WIDTH_SPACE: t = "zero_width_space"; break;
      case G_UNICODE_BREAK_INSEPARABLE: t = "inseparable"; break;
      case G_UNICODE_BREAK_NON_BREAKING_GLUE: t = "non_breaking_glue"; break;
      case G_UNICODE_BREAK_CONTINGENT: t = "contingent"; break;
      case G_UNICODE_BREAK_SPACE: t = "space"; break;
      case G_UNICODE_BREAK_AFTER: t = "after"; break;
      case G_UNICODE_BREAK_BEFORE: t = "before"; break;
      case G_UNICODE_BREAK_BEFORE_AND_AFTER: t = "before_and_after"; break;
      case G_UNICODE_BREAK_HYPHEN: t = "hyphen"; break;
      case G_UNICODE_BREAK_NON_STARTER: t = "non_starter"; break;
      case G_UNICODE_BREAK_OPEN_PUNCTUATION: t = "open_punctuation"; break;
      case G_UNICODE_BREAK_CLOSE_PUNCTUATION: t = "close_punctuation"; break;
      case G_UNICODE_BREAK_QUOTATION: t = "quotation"; break;
      case G_UNICODE_BREAK_EXCLAMATION: t = "exclamation"; break;
      case G_UNICODE_BREAK_IDEOGRAPHIC: t = "ideographic"; break;
      case G_UNICODE_BREAK_NUMERIC: t = "numeric"; break;
      case G_UNICODE_BREAK_INFIX_SEPARATOR: t = "infix_separator"; break;
      case G_UNICODE_BREAK_SYMBOL: t = "symbol"; break;
      case G_UNICODE_BREAK_ALPHABETIC: t = "alphabetic"; break;
      case G_UNICODE_BREAK_PREFIX: t = "prefix"; break;
      case G_UNICODE_BREAK_POSTFIX: t = "postfix"; break;
      case G_UNICODE_BREAK_COMPLEX_CONTEXT: t = "complex_context"; break;
      case G_UNICODE_BREAK_AMBIGUOUS: t = "ambiguous"; break;
      case G_UNICODE_BREAK_UNKNOWN: t = "unknown"; break;
      case G_UNICODE_BREAK_NEXT_LINE: t = "next_line"; break;
      case G_UNICODE_BREAK_WORD_JOINER: t = "word_joiner"; break;
      case G_UNICODE_BREAK_HANGUL_L_JAMO: t = "hangul_l_jamo"; break;
      case G_UNICODE_BREAK_HANGUL_V_JAMO: t = "hangul_v_jamo"; break;
      case G_UNICODE_BREAK_HANGUL_T_JAMO: t = "hangul_t_jamo"; break;
      case G_UNICODE_BREAK_HANGUL_LV_SYLLABLE: t = "hangul_lv_syllable"; break;
      case G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE: t = "hangul_lvt_syllable"; break;
      case G_UNICODE_BREAK_CLOSE_PARANTHESIS: t = "close_paranthesis"; break;
      case G_UNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER: t = "conditional_japanese_starter"; break;
      case G_UNICODE_BREAK_HEBREW_LETTER: t = "hebrew_letter"; break;
      default: t = "unknown"; break;
    }
    lua_pushstring(L, t);
    return 1;
}

/***
Find Unicode mirroring character.
This is a wrapper for `g_unichar_get_mirror_char()`.
@function get_mirror_char
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn nil|string|number if the character has no mirror; otherwise, the
 mirror character.  The returned character is either an integer or a utf-8
 string, depending on the input.
*/
static int glib_get_mirror_char(lua_State *L)
{
    gunichar ret;
    one_unichar();
    if(g_unichar_get_mirror_char(ch, &ret)) {
	if(lua_isnumber(L, 1))
	    lua_pushnumber(L, ret);
	else {
	    char buf[6];
	    int len = g_unichar_to_utf8(ret, buf);
	    lua_pushlstring(L, buf, len);
	}
    } else {
	lua_pushnil(L);
    }
    return 1;
}

/***
Find ISO 15924 script for a Unicode character.
This is a wrapper for `g_unichar_get_script()`.
@function get_script
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn string The four-letter ISO 15924 script code for *c*.
*/
static int glib_get_script(lua_State *L)
{
    GUnicodeScript scr;
    guint32 ret;
    one_unichar();
    scr = g_unichar_get_script(ch);
    ret = GUINT32_FROM_BE(g_unicode_script_to_iso15924(scr));
    lua_pushlstring(L, (char *)&ret, 4);
    return 1;
}

/***
Obtain a substring of a utf-8-encoded string.
This is not a wrapper for `g_utf8_substring, but instead code which
emulates string.sub using g_utf8_offset_to_pointer()` and
g_utf8_strlen().  Positive positions start at the beginining of the
string, and negative positions start at the end.  Position numbers
refer to code points rather than byte offsets.
@function utf8_sub
@tparam string s The utf-8-encoded source string
@tparam number first (optional) The first Unicode code point (default is 1)
@tparam number last (optional) The last Unicode code point (default is -1)
@treturn string The requested substring.  Out-of-bound ranges result in an
 empty string.
*/
static int glib_utf8_sub(lua_State *L)
{
    size_t sz, ul;
    const char *s = luaL_checklstring(L, 1, &sz), *sub, *sube;
    int narg = lua_gettop(L);
    lua_Integer first, last;
    ul = g_utf8_strlen(s, sz);
    if(narg > 1)
	first = luaL_checkinteger(L, 2);
    else
	first = 1;
    if(first < 0)
	first += ul + 1;
    if(first < 1)
	first = 1;
    if(first > ul)
	first = ul;
    if(narg > 2)
	last = luaL_checkinteger(L, 3);
    else
	last = ul;
    if(last < 0)
	last += ul + 1;
    if(last < 1)
	last = 1;
    if(last > ul)
	last = ul;
    if(last < first || !ul) {
	lua_pushliteral(L, "");
	return 1;
    }
    sub = g_utf8_offset_to_pointer(s, first - 1);
    if(last != ul)
	sube = g_utf8_offset_to_pointer(sub, last - first + 1);
    else
	sube = s + sz;
    lua_pushlstring(L, sub, (ptrdiff_t)(sube - sub));
    return 1;
}

/***
Obtain the number of code points in a utf-8-encoded string.
This is a wrapper for `g_utf8_strlen()`.
@function utf8_len
@tparam string s The string
@treturn number The length of *s*, in code points.
*/
static int glib_utf8_len(lua_State *L)
{
    size_t sz, ul;
    const char *s = luaL_checklstring(L, 1, &sz);
    ul = g_utf8_strlen(s, sz);
    lua_pushnumber(L, ul);
    return 1;
}

/***
Check if a string is valid UTF-8.
This is a wrapper for `g_utf8_validate()`
@function utf8_validate
@tparam string s The string
@treturn boolean True if *s* is valid UTF-8.
*/
static int glib_utf8_validate(lua_State *L)
{
    size_t sz;
    const char *s = luaL_checklstring(L, 1, &sz), *e;
    gboolean v = g_utf8_validate(s, sz, &e);
    lua_pushboolean(L, v);
    if(!v) {
	lua_pushlstring(L, e, sz - (int)(e - s));
	return 2;
    }
    return 1;
}

#define uni_str(n) \
static int glib_##n(lua_State *L) \
{ \
    size_t sz; \
    const char *s = luaL_checklstring(L, 1, &sz); \
    char *ret = g_##n(s, sz); \
    lua_pushstring(L, ret); \
    g_free(ret); \
    return 1; \
}

/***
Convert UTF-8 string to upper-case.
This is a wrapper for `g_utf8_strup()`
@function utf8_strup
@tparam string s The source string
@treturn string *s*, with all lower-case characters converted to upper-case
*/
uni_str(utf8_strup)
/***
Convert UTF-8 string to lower-case.
This is a wrapper for `g_utf8_strdown()`
@function utf8_strdown
@tparam string s The source string
@treturn string *s*, with all upper-case characters converted to lower-case
*/
uni_str(utf8_strdown)
/***
Convert UTF-8 string to case-independent form.
This is a wrapper for `g_utf8_casefold()`
@function utf8_casefold
@tparam string s The source string
@treturn string *s*, in a form that is suitable for case-insensitive direct
 string comparison.
*/
uni_str(utf8_casefold)

/***
Perform standard Unicode normalization on a UTF-8 string.
This is a wrapper for `g_utf8_normalize()`.  The four standard
normalizations are NFD (the default), NFC (compose), NFKD (compatible),
and NFKC (compose, compatible).
@function utf8_normalize
@tparam string s The string to normalize
@tparam boolean compose If true, perform canonical composition.  Otherwise,
 leave in decomposed form.
@tparam boolean compatible If true, decompose using compatibility
 decompostions.  Otherwise, only decompose using canonical decompositions.
@treturn string The normalized UTF-8 string.
*/
static int glib_utf8_normalize(lua_State *L)
{
    size_t sz;
    const char *s = luaL_checklstring(L, 1, &sz);
    char *ret;
    int narg = lua_gettop(L);
    int docomp = narg > 1 ? lua_toboolean(L, 2) : 0;
    int docompat = narg > 2 ? lua_toboolean(L, 3) : 0;
    GNormalizeMode nm;
    if(docomp)
	nm = docompat ? G_NORMALIZE_NFKC : G_NORMALIZE_NFC;
    else
	nm = docompat ? G_NORMALIZE_NFKD : G_NORMALIZE_NFD;
    ret = g_utf8_normalize(s, sz, nm);
    if(!ret) {
	lua_pushnil(L);
	lua_pushliteral(L, "Invalid utf-8");
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Compare UTF-8 strings for collation.
This is a wrapper for `g_utf8_collate()`.
@function utf8_collate
@see utf8_collate_key
@see utf8_collate_key_for_filename
@tparam string s1 The first string
@tparam string s2 The second string
@treturn number Numeric comparison result: less than zero if *s1* comes
 before *s2*, greater than zero if *s1* comes after *s2*, and zero
 if *s1* and *s2* are equivalent.
*/
static int glib_utf8_collate(lua_State *L)
{
    const char *s1 = luaL_checkstring(L, 1);
    const char *s2 = luaL_checkstring(L, 2);
    lua_pushnumber(L, g_utf8_collate(s1, s2));
    return 1;
}

/***
Create a comparison key for a UTF-8 string.
This is a wrapper for `g_utf8_collate_key()`.
@function utf8_collate_key
@see utf8_collate
@see utf8_collate_key_for_filename
@tparam string s The string.
@treturn string A form of the string which can be compared using direct
 string comparison rather than utf8_collate.
*/
uni_str(utf8_collate_key)
/***
Create a comparison key for a UTF-8 filename string.
This is a wrapper for `g_collate_key_for_filename()`
@function utf8_collate_key_for_filename
@see utf8_collate
@see utf8_collate_key
@tparam string s The string.
@treturn string A form of the string which can be compared using direct
 string comparison.  Dots and numeric sequences are treated
 differently.  There is no equivalent `utf8_collate_filename()`.
*/
uni_str(utf8_collate_key_for_filename)

#define uni_conv(n, st, rt) \
static int glib_##n(lua_State *L) \
{ \
    size_t sz; \
    const char *s = luaL_checklstring(L, 1, &sz); \
    GError *err = NULL; \
    glong rlen; \
    rt *res = g_##n((st *)s, sz / sizeof(st), NULL, &rlen, &err); \
 \
    if(err) { \
	lua_pushnil(L); \
	lua_pushstring(L, err->message); \
	return 2; \
    } \
    lua_pushlstring(L, (char *)res, rlen * sizeof(rt)); \
    g_free(res); \
    return 1; \
}

/***
Convert a UTF-8 string to UTF-16.
This is a wrapper for `g_utf8_to_utf16()`
@function utf8_to_utf16
@tparam string s The source string
@treturn string *s*, converted to UTF-16, or nil followed by an error message
 if *s* is not valid UTF-8.
*/
uni_conv(utf8_to_utf16, gchar, gunichar2)
/***
Convert a UTF-8 string to UCS-4.
This is a wrapper for `g_utf8_to_ucs4()`
@function utf8_to_ucs4
@tparam string s The source string
@treturn string *s*, converted to UCS-4, or nil followed by an error message
 if *s* is not valid UTF-8.
*/
uni_conv(utf8_to_ucs4, gchar, gunichar)
/***
Convert a UTF-16 string to UTF-8.
This is a wrapper for `g_utf16_to_utf8()`
@function utf16_to_utf8
@tparam string s The source string
@treturn string *s*, converted to UTF-8, or nil followed by an error message
 if *s* is not valid UTF-16.
*/
uni_conv(utf16_to_ucs4, gunichar2, gunichar)
/***
Convert a UTF-16 string to UCS-4.
This is a wrapper for `g_utf16_to_ucs4()`
@function utf16_to_ucs4
@tparam string s The source string
@treturn string *s*, converted to UCS-4, or nil followed by an error message
 if *s* is not valid UTF-16.
*/
uni_conv(utf16_to_utf8, gunichar2, gchar)
/***
Convert a UCS-4 string to UTF-16.
This is a wrapper for `g_utf8_to_utf16()`
@function ucs4_to_utf16
@tparam string s The source string
@treturn string *s*, converted to UTF-16, or nil followed by an error message
 if *s* is not valid UCS-4.
*/
uni_conv(ucs4_to_utf16, gunichar, gunichar2)
/***
Convert a UCS-4 string to UTF-8.
This is a wrapper for `g_utf16_to_utf8()`
@function ucs4_to_utf8
@tparam string s The source string
@treturn string *s*, converted to UTF-8, or nil followed by an error message
 if *s* is not valid UCS-4.
*/
uni_conv(ucs4_to_utf8, gunichar, gchar)

/***
Convert a UCS-4 code point to UTF-8.
This is a wrapper for `g_unichar_to_utf8()`
@function to_utf8
@tparam string|number c The Unicode character, as either an integer
 or a utf-8 string.
@treturn string A UTF-8 string representing *c*.  Note that this basically
 has no effect if *c* is a string already.
*/
static int glib_to_utf8(lua_State *L)
{
    char buf[6];
    int len;
    one_unichar();
    len = g_unichar_to_utf8(ch, buf);
    lua_pushlstring(L, buf, len);
    return 1;
}

/***
Base64 Encoding
@section Base64 Encoding
*/

#define B64_ENCCHUNK 80
#define B64_BUFSIZE_NCR ((B64_ENCCHUNK/3+1)*4+4)
#define B64_BUFSIZE (B64_BUFSIZE_NCR + B64_BUFSIZE_NCR/72 + 1)
#define B64_DECCHUNK ((B64_BUFSIZE-3)/3*4)

typedef struct base64_state {
    gint state, save;
    char obuf[B64_BUFSIZE];
} base64_state;

/***
Stream Base64-encoding function returned by base64\_encode.
This function is returned by base64\_encode to support
piecewise-encoding streams.  Simply call with string arguments,
accumulating the returned strings.  When finished with the stream,
call with no arguments.  This will return the final string to
append and reset the stream for reuse.
@function _base64_encode_
@see base64_encode
@tparam string s (optional) The next piece of the string to convert; absent
 to finish conversion
@treturn string The next piece of the converted string
*/
static int stream_base64_encode(lua_State *L)
{
    size_t sz;
    const char *s;

    get_udata(L, lua_upvalueindex(1), st, base64_state);
    if(lua_gettop(L) == 0 || lua_isnil(L, 1)) {
	sz = g_base64_encode_close(0, st->obuf, &st->state, &st->save);
	lua_pushlstring(L, st->obuf, sz);
	memset(st, 0, sizeof(*st));
	return 1;
    }
    s = luaL_checklstring(L, 1, &sz);
    lua_pushliteral(L, "");
    while(sz > 0) {
	int encsz = sz > B64_ENCCHUNK ? B64_ENCCHUNK : sz, retsz;
	retsz = g_base64_encode_step((const guchar *)s, encsz, 0, st->obuf,
				     &st->state, &st->save);
	if(retsz) {
	    lua_pushlstring(L, st->obuf, retsz);
	    lua_concat(L, 2);
	}
	sz -= encsz;
	s += encsz;
    }
    return 1;
}

/***
Base64-encode a string.
This is a wrapper for `g_base64_encode()` and friends.
@function base64_encode
@see _base64_encode_
@tparam string s (optinal) The data to encode.  If absent, return a function
 like \_base64\_encode\_ for encoding a stream.
@treturn string|function The base64-encoded stream (without newlines), or a
 function to do the same on a stream.
*/
static int glib_base64_encode(lua_State *L)
{
    size_t sz;
    const char *s;
    char *ret;
    if(lua_gettop(L) == 0) {
	alloc_udata(L, st, base64_state);
	lua_pushcclosure(L, stream_base64_encode, 1);
	return 1;
    }
    s = luaL_checklstring(L, 1, &sz);
    ret = g_base64_encode((const guchar *)s, sz);
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Stream Base64-decoding function returned by base64\_decode.
This function is returned by base64\_decode to support
piecewise-decoding streams.  Simply call with string arguments,
accumulating the returned strings.  When finished with the stream,
call with no arguments.  This will return the final string to
append and reset the stream for reuse.
@function _base64_decode_
@see base64_decode
@tparam string s (optional) The next piece of the string to convert; absent
 to finish conversion
@treturn string The next piece of the converted output
*/
static int stream_base64_decode(lua_State *L)
{
    size_t sz;
    const char *s;

    get_udata(L, lua_upvalueindex(1), st, base64_state);
    if(lua_gettop(L) == 0 || lua_isnil(L, 1)) {
	memset(st, 0, sizeof(*st));
	lua_pushliteral(L, "");
	return 1;
    }
    s = luaL_checklstring(L, 1, &sz);
    lua_pushliteral(L, "");
    while(sz > 0) {
	int decsz = sz > B64_DECCHUNK ? B64_DECCHUNK : sz, retsz;
	retsz = g_base64_decode_step(s, decsz, (guchar *)st->obuf,
				     &st->state, (guint *)&st->save);
	if(retsz) {
	    lua_pushlstring(L, st->obuf, retsz);
	    lua_concat(L, 2);
	}
	sz -= decsz;
	s += decsz;
    }
    return 1;
}

/***
Base64-decode a string.
This is a wrapper for `g_base64_deocde()` and friends.
@function base64_decode
@see _base64_decode_
@tparam string s (optional) The data to decode.  If absent, return a function
 like _base64_decode_ for decoding a stream.
@treturn string|function The decoded form of the base64-encoded stream, or a
 function to do the same on a stream.
*/
static int glib_base64_decode(lua_State *L)
{
    const char *s;
    guchar *ret;
    gsize rsz;
    if(lua_gettop(L) == 0) {
	alloc_udata(L, st, base64_state);
	lua_pushcclosure(L, stream_base64_decode, 1);
	return 1;
    }
    s = luaL_checkstring(L, 1);
    ret = g_base64_decode(s, &rsz);
    lua_pushlstring(L, (char *)ret, rsz);
    g_free(ret);
    return 1;
}

/***
Data Checksums
@section Data Checksums
*/

typedef struct sumstate {
    GChecksum *sum;
} sumstate;

static int free_sumstate(lua_State *L)
{
    get_udata(L, 1, st, sumstate);
    if(st->sum)
	g_checksum_free(st->sum);
    return 0;
}

/* return type is lifted from cmorris' lua-glib */
static int finalize_sum(lua_State *L, GChecksum *sum, gboolean raw)
{
    if(raw) {
        guint8 digest[64];
        gsize digest_len = 64;
        g_checksum_get_digest(sum, digest, &digest_len);
        lua_pushlstring(L, (char *)digest, digest_len);
    } else {
        const gchar *digest = g_checksum_get_string(sum);
        lua_pushstring(L, digest);
    }
    g_checksum_free(sum);
    return 1;
}

/***
Stream MD5 calculation function returned by md5sum.
This function is returned by md5sum to support computing
stream checksums piecewise.  Simply call with string arguments,
until the stream is complete.  Then, call with an absent or nil
string argument to return the final checksum.  Doing so invalidates
the state, so that the function returns an error from that point
forward.
@function _md5sum_
@see md5sum
@tparam string s (optional) The next piece of the string to checksum; absent
 or nil to finish checksum
@tparam boolean raw (optional) True if checksum should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is not nil)
@treturn |nil|string Nothing unless *s* is absent or nil.  Otherwise, return
 the computed checksum.  If the state is invalid, always return nil.
*/
/***
Stream SHA1 calculation function returned by sha1sum.
This function is returned by sha1sum to support computing
stream checksums piecewise.  Simply call with string arguments,
until the stream is complete.  Then, call with an absent or nil
string argument to return the final checksum.  Doing so invalidates
the state, so that the function returns an error from that point
forward.
@function _sha1sum_
@see sha1sum
@tparam string s (optional) The next piece of the string to checksum; absent
 or nil to finish checksum
@tparam boolean raw (optional) True if checksum should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is not nil)
@treturn |nil|string Nothing unless *s* is absent or nil.  Otherwise, return
 the computed checksum.  If the state is invalid, always return nil.
*/
/***
Stream SHA256 calculation function returned by sha256sum.
This function is returned by sha256sum to support computing
stream checksums piecewise.  Simply call with string arguments,
until the stream is complete.  Then, call with an absent or nil
string argument to return the final checksum.  Doing so invalidates
the state, so that the function returns an error from that point
forward.
@function _sha256sum_
@see sha256sum
@tparam string s (optional) The next piece of the string to checksum; absent
 or nil to finish checksum
@tparam boolean raw (optional) True if checksum should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is not nil)
@treturn |nil|string Nothing unless *s* is absent or nil.  Otherwise, return
 the computed checksum.  If the state is invalid, always return nil.
*/
static int stream_sum(lua_State *L)
{
    GChecksum *sum;
    get_udata(L, lua_upvalueindex(1), st, sumstate);
    if(!st->sum) {
	lua_pushnil(L);
	return 1;
    }
    if(lua_gettop(L) > 0 && lua_isstring(L, 1)) {
	size_t sz;
	const char *s = luaL_checklstring(L, 1, &sz);
	g_checksum_update(st->sum, (const guchar *)s, sz);
	return 0;
    }
    sum = st->sum;
    st->sum = NULL;
    return finalize_sum(L, sum, lua_toboolean(L, 1));
}

static int glib_sum(lua_State *L, GChecksumType ct)
{
    size_t sz;
    const char *s;
    GChecksum *sum;

    if(lua_gettop(L) == 0) {
	alloc_udata(L, st, sumstate);
	st->sum = g_checksum_new(ct);
	lua_pushcclosure(L, stream_sum, 1);
	return 1;
    }
    sum = g_checksum_new(ct);
    s = luaL_checklstring(L, 1, &sz);
    g_checksum_update(sum, (const guchar *)s, sz);
    return finalize_sum(L, sum, lua_toboolean(L, 2));
}

/***
Compute MD5 checksum of a string.
This is a wrapper for `g_checksum_new()` and friends.
@function md5sum
@see _md5sum_
@tparam string s (optional) The data to checksum.  If absent, return a
 function like \_md5sum\_ to checksum a stream piecewise.
@tparam boolean raw (optional) True if checksum should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is nil)
@treturn string|function The MD5 checksum or a stream converter function.
*/
static int glib_md5sum(lua_State *L)
{
    return glib_sum(L, G_CHECKSUM_MD5);
}

/***
Compute SHA1 checksum of a string.
This is a wrapper for `g_checksum_new()` and friends.
@function sha1sum
@see _sha1sum_
@tparam string s (optional) The data to checksum.  If absent, return a
 function like \_sha1sum\_ to checksum a stream piecewise.
@tparam boolean raw (optional) True if checksum should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is nil)
@treturn string|function The SHA1 checksum or a stream converter function.
*/
static int glib_sha1sum(lua_State *L)
{
    return glib_sum(L, G_CHECKSUM_SHA1);
}

/***
Compute SHA256 checksum of a string.
This is a wrapper for `g_checksum_new()` and friends.
@function sha256sum
@see _sha256sum_
@tparam string s (optional) The data to checksum.  If absent, return a
 function like \_sha256sum\_ to checksum a stream piecewise.
@tparam boolean raw (optional) True if checksum should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is nil)
@treturn string|function The SHA256 checksum or a stream converter function.
*/
static int glib_sha256sum(lua_State *L)
{
    return glib_sum(L, G_CHECKSUM_SHA256);
}

/***
Secure HMAC Digests
@section Secure HMAC Digests
*/

typedef struct hmacstate {
    GHmac *sum;
} hmacstate;

static int free_hmacstate(lua_State *L)
{
    get_udata(L, 1, st, hmacstate);
    if(st->sum)
	g_hmac_unref(st->sum);
    return 0;
}

/* return type is lifted from cmorris' lua-glib */
static int finalize_hmac(lua_State *L, GHmac *sum, gboolean raw)
{
    if(raw) {
        guint8 digest[64];
        gsize digest_len = 64;
        g_hmac_get_digest(sum, digest, &digest_len);
        lua_pushlstring(L, (char *)digest, digest_len);
    } else {
        const gchar *digest = g_hmac_get_string(sum);
        lua_pushstring(L, digest);
    }
    g_hmac_unref(sum);
    return 1;
}

/***
Stream MD5 HMAC calculation function returned by md5hmac.
This function is returned by md5hmac to support computing
stream digests piecewise.  Simply call with string arguments,
until the stream is complete.  Then, call with an absent or nil
string argument to return the final digest.  Doing so invalidates
the state, so that the function returns an error from that point
forward.
@function _md5hmac_
@see md5hmac
@tparam string s (optional) The next piece of the string to digest; absent
 or nil to finish digest
@tparam boolean raw (optional) True if digest should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is not nil)
@treturn |nil|string Nothing unless *s* is absent or nil.  Otherwise, return
 the computed digest.  If the state is invalid, always return nil.
*/
/***
Stream SHA1 HMAC calculation function returned by sha1hmac.
This function is returned by sha1hmac to support computing
stream digests piecewise.  Simply call with string arguments,
until the stream is complete.  Then, call with an absent or nil
string argument to return the final digest.  Doing so invalidates
the state, so that the function returns an error from that point
forward.
@function _sha1hmac_
@see sha1hmac
@tparam string s (optional) The next piece of the string to digest; absent
 or nil to finish digest
@tparam boolean raw (optional) True if digest should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is not nil)
@treturn |nil|string Nothing unless *s* is absent or nil.  Otherwise, return
 the computed digest.  If the state is invalid, always return nil.
*/
/***
Stream SHA256 HMAC calculation function returned by sha256hmac.
This function is returned by sha256hmac to support computing
stream digests piecewise.  Simply call with string arguments,
until the stream is complete.  Then, call with an absent or nil
string argument to return the final digest.  Doing so invalidates
the state, so that the function returns an error from that point
forward.
@function _sha256hmac_
@see sha256hmac
@tparam string s (optional) The next piece of the string to digest; absent
 or nil to finish digest
@tparam boolean raw (optional) True if digest should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is not nil)
@treturn |nil|string Nothing unless *s* is absent or nil.  Otherwise, return
 the computed digest.  If the state is invalid, always return nil.
*/
static int stream_hmac(lua_State *L)
{
    GHmac *sum;
    get_udata(L, lua_upvalueindex(1), st, hmacstate);
    if(!st->sum) {
	lua_pushnil(L);
	return 1;
    }
    if(lua_gettop(L) > 0 && lua_isstring(L, 1)) {
	size_t sz;
	const char *s = luaL_checklstring(L, 1, &sz);
	g_hmac_update(st->sum, (const guchar *)s, sz);
	return 0;
    }
    sum = st->sum;
    st->sum = NULL;
    return finalize_hmac(L, sum, lua_toboolean(L, 1));
}

static int glib_hmac(lua_State *L, GChecksumType ct)
{
    size_t keysz, sz;
    const char *key, *s;
    GHmac *sum;

    key = luaL_checklstring(L, 1, &keysz);
    if(lua_gettop(L) == 1) {
	alloc_udata(L, st, hmacstate);
	st->sum = g_hmac_new(ct, (const guchar *)key, keysz);
	lua_pushcclosure(L, stream_hmac, 1);
	return 1;
    }
    s = luaL_checklstring(L, 2, &sz);
    sum = g_hmac_new(ct, (const guchar *)key, keysz);
    g_hmac_update(sum, (const guchar *)s, sz);
    return finalize_hmac(L, sum, lua_toboolean(L, 3));
}

/***
Compute secure HMAC digest using MD5.
This is a wrapper for `g_hmac_new()` and friends.
@function md5hmac
@see _md5hmac_
@tparam string key HMAC key
@tparam string s (optional) The data to digest.  If absent, return a
 function like \_md5hmac\_ to digest a stream piecewise.
@tparam boolean raw (optional) True if digest should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is nil)
@treturn string|function The MD5 HMAC digest or a stream converter function.
*/
static int glib_md5hmac(lua_State *L)
{
    return glib_hmac(L, G_CHECKSUM_MD5);
}

/***
Compute secure HMAC digest using SHA1.
This is a wrapper for `g_hmac_new()` and friends.
@function sha1hmac
@see _sha1hmac_
@tparam string key HMAC key
@tparam string s (optional) The data to digest.  If absent, return a
 function like \_sha1hmac\_ to digest a stream piecewise.
@tparam boolean raw (optional) True if digest should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is nil)
@treturn string|function The SHA1 HMAC digest or a stream converter function.
*/
static int glib_sha1hmac(lua_State *L)
{
    return glib_hmac(L, G_CHECKSUM_SHA1);
}

/***
Compute secure HMAC digest using SHA256.
This is a wrapper for `g_hmac_new()` and friends.
@function sha256hmac
@see _sha256hmac_
@tparam string key HMAC key
@tparam string s (optional) The data to digest.  If absent, return a
 function like \_sha256hmac\_ to digest a stream piecewise.
@tparam boolean raw (optional) True if digest should be returned in binary
 form.  Otherwise, return the lower-case hexadecimal-encoded form (ignored
 if *s* is nil)
@treturn string|function The SHA256 HMAC digest or a stream converter function.
*/
static int glib_sha256hmac(lua_State *L)
{
    return glib_hmac(L, G_CHECKSUM_SHA256);
}

/***
Internationalization
@section Internationalization
*/

/***
Replace text with its translation.
This is a wrapper for `_()`.  It resides in the global symbol table
 rather than in glib.
@function _
@tparam string s The text to translate
@treturn string The translated text, or *s* if there is no translation
*/
static int glib_gettext(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    lua_pushstring(L, gettext(s));
    return 1;
}

/***
Replace text and context with its translation.
This is a wrapper for `Q_()`.  It resides in the global symbol table
 rather than in glib.
@function Q_
@tparam string s the optional context, followed by a vertical bar, followed
 by the text to translate
@treturn string The translated text, or *s* with its context stripped if there
 is no translation
*/
static int glib_dpgettext0(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    lua_pushstring(L, g_dpgettext(NULL, s, 0));
    return 1;
}

/***
Replace text and context with its translation.
This is a wrapper for `C_()`.  It resides in the global symbol table
 rather than in glib.
@function C_
@tparam string c The context
@tparam string s The text to translate
@treturn string The translated text, or *s* if there is no translation
*/
static int glib_dpgettext4(lua_State *L)
{
    size_t sz;
    const char *s;

    s = luaL_checklstring(L, 1, &sz);
    lua_pushliteral(L, "\4");
    lua_insert(L, 2);
    lua_concat(L, 3);
    s = luaL_checkstring(L, 1);
    lua_pushstring(L, g_dpgettext(NULL, s, sz + 1));
    return 1;
}

/***
Mark text for translation.
This is a wrapper for `N_()`.  It resides in the global symbol table
 rather than in glib.
@function N_
@tparam string s The text to translate
@treturn string *s*
*/
static int glib_ngettext(lua_State *L)
{
    luaL_checkstring(L, 1);
    return 1;
}

/***
Mark text for translation with context.
This is a wrapper for `NC_()`.  It resides in the global symbol table
 rather than in 
@function NC_
@tparam string c The context
@tparam string s The text to translate.
@treturn string *s*
*/
static int glib_ndpgettext4(lua_State *L)
{
    luaL_checkstring(L, 1);
    luaL_checkstring(L, 2);
    return 1;
}

/***
Obtain list of valid locale names.
This is a wrapper for `g_get_locale_variants()` and
`g_get_language_names()`
@function get_locale_variants
@tparam string locale (optional) If present, find valid locale names derived
 from this.  Otherwise, find valid names for the default locale (including C).
@treturn {string,...} A table array containing valid names for the locale, in
 order of preference.
*/
static int glib_get_locale_variants(lua_State *L)
{
    gchar **var = NULL;
    const gchar * const * svar;
    int i;
    if(lua_gettop(L) > 0) {
	const char *s = luaL_checkstring(L, 1);
	svar = (const gchar * const *)(var = g_get_locale_variants(s));
    } else
	svar = g_get_language_names();
    for(i = 0; svar[i]; i++);
    lua_createtable(L, i, 0);
    for(i = 0; svar[i]; i++) {
	lua_pushstring(L, svar[i]);
	lua_rawseti(L, -2, i + 1);
    }
    if(var)
	g_strfreev(var);
    return 1;
}

/***
Date and Time Functions
@section Date and Time Functions
*/

/* sort of from cmorris' lua-glib */
/***
Suspend execution for some seconds.
This is a wrapper for `g_usleep()`
@function sleep
@tparam number t Number of seconds to sleep (microsecond accuracy)
*/
static int glib_sleep(lua_State *L)
{
    g_usleep(luaL_checknumber(L, 1) * 1e6);
    return 0;
}

/***
Suspend execution for some microseconds.
This is a wrapper for `g_usleep()`
@function usleep
@tparam number t Number of microseconds to sleep
*/
static int glib_usleep(lua_State *L)
{
    g_usleep(luaL_checknumber(L, 1));
    return 0;
}

/***
Random Numbers
@section Random Numbers
*/

/***
Obtain a psuedorandom number.
This is a wrapper for `g_random()` and friends.  This is a clone of
the standard Lua math.rand(), but using a different random number
algorithm.  Note that there is no way to set the seed; use
rand\_new() if you need to do that.  In fact, you can
simply replace random with the results of rand\_new() if you
want to simulate setting a seed for this function.
@function random
@see rand_new
@tparam number low (optional) If *high* is present, this is the low end of
 the range of random integers to return
@tparam number high (optional) If present, return a range of random integers,
 from *low* to *high* inclusive.  If not present, return a floating point
 number in the range from zero to one exclusive of one. If *low* is not
 present, and *high* is, *low* is 1.
*/
static int glib_random(lua_State *L)
{
    int narg = lua_gettop(L);

    /* this behavior is copied from lua's math.random() */
    if(!narg)
	lua_pushnumber(L, g_random_double());
    else if(narg == 1)
	lua_pushinteger(L, g_random_int_range(1, luaL_checkinteger(L, 1) + 1));
    else
	lua_pushinteger(L, g_random_int_range(luaL_checkinteger(L, 1),
					      luaL_checkinteger(L, 2) + 1));
    return 1;
}

typedef struct rand_state {
    GRand *state;
} rand_state;

static int free_rand_state(lua_State *L)
{
    get_udata(L, 1, st, rand_state);
    if(st->state)
	g_rand_free(st->state);
    return 0;
}

static int glib_rand(lua_State *L)
{
    int narg = lua_gettop(L);
    get_udata(L, lua_upvalueindex(1), st, rand_state);
    
    /* this behavior is copied from lua's math.random() */
    if(!narg)
	lua_pushnumber(L, g_rand_double(st->state));
    else if(narg == 1)
	lua_pushinteger(L, g_rand_int_range(st->state, 1, luaL_checkinteger(L, 1) + 1));
    else
	lua_pushinteger(L, g_rand_int_range(st->state,
					    luaL_checkinteger(L, 1),
					    luaL_checkinteger(L, 2) + 1));
    return 1;
}

/***
Obtain a psuedorandom number generator, given a seed.
This is a wrapper for `g_rand_new()` and friends.
@function rand_new
@see random
@tparam number|{number,...} seed (optional) seed (if not specified, one will
 be selected by the library).  This may be either a number or a table
 array of numbers.
@treturn function A function with the same behavior as random.
*/
static int glib_rand_new(lua_State *L)
{
    int nargs = lua_gettop(L);
    alloc_udata(L, st, rand_state);
    if(!nargs)
	st->state = g_rand_new();
    else if(lua_isnumber(L, 1))
	st->state = g_rand_new_with_seed(lua_tonumber(L, 1));
    else if(lua_istable(L, 1)) {
	size_t len = lua_objlen(L, 1);
	guint32 *arr = g_malloc(len * sizeof(guint32));
	int i;
	for(i = 0; i < len; i++) {
	    lua_pushinteger(L, i + 1);
	    lua_gettable(L, 1);
	    if(!lua_isnumber(L, -1)) {
		i = -1;
		break;
	    }
	    arr[i] = lua_tonumber(L, -1);
	    lua_pop(L, 1);
	}
	if(i >= 0)
	    st->state = g_rand_new_with_seed_array(arr, len);
	g_free(arr);
	luaL_argcheck(L, i >= 0, 1, "Seed must be numeric");
    } else
	luaL_argerror(L, 1, "Expected seed");
    lua_pushcclosure(L, glib_rand, 1);
    return 1;
}

/***
Miscellaneous Utility Functions
@section Miscellaneous Utility Functions
*/

/***
Set or get localized application name.
This is a wrapper for `g_get_application_name()` and friends
@function application_name
@see prgname
@tparam string name (optional) If present, set the application name
@treturn string The name of the application, as set by a previous
 invocation of this function.
*/
static int glib_application_name(lua_State *L)
{
    lua_pushstring(L, g_get_application_name());
    if(lua_gettop(L) > 0)
	g_set_application_name(luaL_checkstring(L, 1));
    return 1;
}

/***
Set or get program name.
This is a wrapper for `g_get_prgname()` and friends
@function prgname
@see application_name
@tparam string name (optional) If present, set the program name
@treturn string The name of the program, as set by a previous
 invocation of this function.
*/
static int glib_prgname(lua_State *L)
{
    lua_pushstring(L, g_get_prgname());
    if(lua_gettop(L) > 0)
	g_set_prgname(luaL_checkstring(L, 1));
    return 1;
}

/***
Get environment variable value.
This is a wrapper for `g_getenv()`.  It is safer to use this than
os.getenv() if you are going to modify the environment.
@function getenv
@see setenv
@see unsetenv
@tparam string name Name of environment variable to retrieve
@treturn string Value of variable (string) if present; otherwise nil
*/
static int glib_getenv(lua_State *L)
{
    const gchar *e = g_getenv(luaL_checkstring(L, 1));
    if(e)
	lua_pushstring(L, e);
    else
	lua_pushnil(L);
    return 1;
}

/***
Set environment variable value.
This is a wrapper for `g_setenv()`.  If you use this, you should use
getenv instead of os.getenv as well.
@function setenv
@see getenv
@see unsetenv
@tparam string name Name of variable to set
@tparam string value New value of variable
@tparam boolean replace (optional) True to replace if exists; otherwise
 leave old value
@treturn boolean True if set succeeded; false otherwise
*/
static int glib_setenv(lua_State *L)
{
    lua_pushboolean(L, g_setenv(luaL_checkstring(L, 1), luaL_checkstring(L, 2),
				lua_toboolean(L, 3)));
    return 1;
}

/***
Remove environment variable.
This is a wrapper for `g_unsetenv()`.  If you use this, you should use
getenv instead of os.getenv as well.
@function unsetenv
@see getenv
@see setenv
@tparam string name Name of variable to remove from environment
*/
static int glib_unsetenv(lua_State *L)
{
    g_unsetenv(luaL_checkstring(L, 1));
    return 0;
}

/***
Obtain names of all environment variables.
This is a wrapper for `g_listenv()`.
@function listenv
@treturn {string,...} An array table whose entries are environment variable
 names
*/
static int glib_listenv(lua_State *L)
{
    gchar **env = g_listenv(), **envp;
    int i;

    for(envp = env, i = 0; *envp; envp++, i++);
    lua_createtable(L, i, 0);
    for(envp = env, i = 0; *envp; envp++, i++) {
	lua_pushstring(L, *envp);
	lua_rawseti(L, -2, i + 1);
    }
    g_strfreev(env);
    return 1;
}

/***
Obtain system user name for current user.
This is a wrapper for `g_get_user_name()`
@function get_user_name
@treturn string The name of the user
*/
static int glib_get_user_name(lua_State *L)
{
    lua_pushstring(L, g_get_user_name());
    return 1;
}

/***
Obtain full user name for current user.
This is a wrapper for `g_get_real_name()`
@function get_real_name
@treturn string The full name of the user, or Unknown if this
 cannot be obtained.
*/
static int glib_get_real_name(lua_State *L)
{
    lua_pushstring(L, g_get_real_name());
    return 1;
}

static struct dn {
    const gchar *name;
    const gchar *(*fn)(void);
    int special;
    const gchar * const *(*mfn)(void);
} dirnames[] = {
    { "cache", g_get_user_cache_dir },
    { "config", g_get_user_config_dir },
    { "data", g_get_user_data_dir },
    { "desktop", NULL, G_USER_DIRECTORY_DESKTOP },
    { "documents", NULL, G_USER_DIRECTORY_DOCUMENTS },
    { "download", NULL, G_USER_DIRECTORY_DOWNLOAD },
    { "help", NULL, -1 },
    { "home", g_get_home_dir },
    { "music", NULL, G_USER_DIRECTORY_MUSIC },
    { "pictures", NULL, G_USER_DIRECTORY_PICTURES },
    { "runtime", g_get_user_runtime_dir },
    { "share", NULL, G_USER_DIRECTORY_PUBLIC_SHARE },
    { "system_config", NULL, 0, g_get_system_config_dirs },
    { "system_data", NULL, 0, g_get_system_data_dirs },
    { "templates", NULL, G_USER_DIRECTORY_TEMPLATES },
    { "tmp", g_get_tmp_dir },
    { "videos", NULL, G_USER_DIRECTORY_VIDEOS }
};
#define NDIRNAMES (sizeof(dirnames)/sizeof(dirnames[0]))

/***
Obtain a standard directory name.
This is a wrapper for `g_get_*_dir()`
@function get_dir_name
@tparam string d The directory to obtain; one of
     'cache', 'config', 'data', 'desktop', 'documents', 'download',
     'home', 'music', 'pictures', 'runtime', 'share', 'system_config',
     'system_data', 'templates', 'tmp', 'videos', 'help'
 'help' just returns this list of names.
@treturn string|{string,...}|nil The directory (a string) if there can be only
 one; if there can be more than one, the list of directories is returned
 as a table array of strings.  Currently, only 'help', 'system_data, and
 'system_config' return more than one.  If there is no standard directory
 of the given kind, returns nil.
*/
static int glib_get_dir_name(lua_State *L)
{
    const char *n = luaL_optstring(L, 1, "help");
    struct dn *d = bsearch(n, dirnames, NDIRNAMES, sizeof(dirnames[0]), ns_cmp);
    if(!d) {
	lua_pushnil(L);
	return 1;
    }
    if(d->fn)
	lua_pushstring(L, d->fn());
    else if(d->mfn) {
	const gchar * const * res = d->mfn(), * const * resp;
	int i;
	for(i = 0, resp = res; *resp; resp++, i++);
	lua_createtable(L, i, 0);
	for(i = 0, resp = res; *resp; resp++, i++) {
	    lua_pushstring(L, *resp);
	    lua_rawseti(L, -2, i + 1);
	}
    } else if(d->special < 0) {
	int i;
	lua_createtable(L, NDIRNAMES, 0);
	for(i = 0; i < NDIRNAMES; i++) {
	    lua_pushstring(L, dirnames[i].name);
	    lua_rawseti(L, -2, i + 1);
	}
    } else
	lua_pushstring(L, g_get_user_special_dir(d->special));
    return 1;
}

/***
Obtain current host name.
This is a wrapper for `g_get_host_name()`
@function get_host_name
@treturn string The local host name.  This is not guaranteed to be a unique
 identifier for the host, or in any way related to its DNS entry.
*/
static int glib_get_host_name(lua_State *L)
{
    lua_pushstring(L, g_get_host_name());
    return 1;
}

/***
Obtain current working directry.
This is a wrapper for `g_get_current_dir()`
@function get_current_dir
@treturn string The current working directory, as an absolute path.
*/
static int glib_get_current_dir(lua_State *L)
{
    gchar *cwd = g_get_current_dir();
    lua_pushstring(L, cwd);
    g_free(cwd);
    return 1;
}

/***
Check if a directory is absolute.
This is a wrapper for `g_path_is_absolute()`
@function path_is_absolute
@tparam string d The directory to check
@treturn boolean True if d is absolute; false otherwise.
*/
static int glib_path_is_absolute(lua_State *L)
{
    lua_pushboolean(L, g_path_is_absolute(luaL_checkstring(L, 1)));
    return 1;
}

/***
Split the root part from a path.
This is a wrapper for `g_path_skip_root()`
@function path_split_root
@tparam string d The path to split
@treturn string|nil The root part.  If the path is not absolute, this will be
 nil.
@treturn string The non-root part
*/
static int glib_path_split_root(lua_State *L)
{
    const gchar *s = luaL_checkstring(L, 1);
    const gchar *rest = g_path_skip_root(s);
    if(!rest) {
	lua_pushnil(L);
	rest = s;
    } else
	lua_pushlstring(L, s, (int)(rest - s));
    lua_pushstring(L, rest);
    return 2;
}

/***
Obtain the last element of a path.
This is a wrapper for `g_path_basename()`
@function path_get_basename
@tparam string d The path to split
@treturn string The last path element of the path
*/
/* note that cmorris rejected the result if it was ., / , or \ */
static int glib_path_get_basename(lua_State *L)
{
    gchar *s = g_path_get_basename(luaL_checkstring(L, 1));
    lua_pushstring(L, s);
    g_free(s);
    return 1;
}

/***
Obtain all but the last element of a path.
This is a wrapper for `g_path_dirname()`
@function path_get_dirname
@tparam string d The path to split
@return All but the last element of the path.
*/
/* note that cmorris rejected the result if it was . */
static int glib_path_get_dirname(lua_State *L)
{
    gchar *s = g_path_get_dirname(luaL_checkstring(L, 1));
    lua_pushstring(L, s);
    g_free(s);
    return 1;
}

static gchar **build_varargs(lua_State *L, int skip)
{
    int narg = lua_gettop(L) - skip;
    const gchar **args;
    int i;

    ++skip; /* instead of 1 + skip everywhere */
    if(narg == 1 && lua_istable(L, skip)) {
	size_t len = lua_objlen(L, skip);
	luaL_checkstack(L, len, "unpacking table");
	args = g_malloc((len + 1) * sizeof(args));
	for(i = 0; i < len; i++) {
	    lua_pushinteger(L, i + 1);
	    lua_gettable(L, skip);
	    args[i] = lua_tostring(L, -1);
	    if(!args[i]) {
		g_free(args);
		luaL_checkstring(L, skip);
	    }
	}
	args[len] = NULL;
    } else {
	for(i = 0; i < narg; i++)
	    luaL_checkstring(L, i + skip);
	args = g_malloc((narg + 1) * sizeof(args));
	for(i = 0; i < narg; i++)
		args[i] = lua_tostring(L, i + skip);
	args[narg] = NULL;
    }
    return (gchar **)args;
}

static void free_varargs(lua_State *L, gchar **args, int skip)
{
    int i;
    if(args[0] && lua_istable(L, 1 + skip)) {
	for(i = 0; args[i]; ++i);
	lua_pop(L, i);
    }
    g_free(args);
}

/***
Construct a file name from its path constituents.
This is a wrapper for `g_build_filenamev()`
@function build_filename
@tparam {string,...}|string,... ... If the first parameter is a table, this
 table contains a list of path element strings.  Otherwise, all parameters
 are taken to be the path element strings.
@treturn string The result of concatenating all supplied path elements,
 separated by at most one directory separator as appropriate.
*/
static int glib_build_filename(lua_State *L)
{
    gchar **args = build_varargs(L, 0);
    gchar *res = g_build_filenamev(args);
    free_varargs(L, args, 0);
    lua_pushstring(L, res);
    g_free(res);
    return 1;
}

/***
Construct a path from its constituents.
This is a wrapper for `g_build_pathv()`
@function build_path
@see build_filename
@tparam string sep The path element separator
@tparam {string,...}|string,... ... If the first parameter is a table, this
 table contains a list of path element strings.  Otherwise, all parameters
 are taken to be the path element strings.
@treturn string The result of concatenating all supplied path elements,
 separated by at most one path separator.
*/
static int glib_build_path(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    gchar **args = build_varargs(L, 1);
    gchar *res = g_build_pathv(s, args);
    free_varargs(L, args, 1);
    lua_pushstring(L, res);
    g_free(res);
    return 1;
}

/***
Print sizes in scientific notation.
This is a wrapper for `g_format_size_full()`
@function format_size
@tparam number size The size to format
@tparam boolean pow2 (optional) Set to true to use power-of-2 units rather
 than power-of-10 units.
@tparam boolean long (optional) Set to true to display the full size, in
 parentheses, after the short size.
@treturn string A string representing the size with SI units
*/
static int glib_format_size(lua_State *L)
{
    int flags = G_FORMAT_SIZE_DEFAULT;
    guint64 size = luaL_checkinteger(L, 1);
    gchar *res;

    if(lua_toboolean(L, 2))
	flags |= G_FORMAT_SIZE_IEC_UNITS;
    if(lua_toboolean(L, 3))
	flags |= G_FORMAT_SIZE_LONG_FORMAT;
    res = g_format_size_full(size, flags);
    lua_pushstring(L, res);
    g_free(res);
    return 1;
}

/***
Locate an executable using the operating system's search method.
This is a wrapper for `g_find_program_in_path()`
@function find_program_in_path
@tparam string p The program name to find
@treturn string|nil An absolute path to the program, or nil if it can't be
 found.
*/
static int glib_find_program_in_path(lua_State *L)
{
    lua_pushstring(L, g_find_program_in_path(luaL_checkstring(L, 1)));
    return 1;
}

static int qsort_fun(gconstpointer _a, gconstpointer _b, gpointer l)
{
    const size_t *a = _a, *b = _b;
    lua_State *L = l;
    gboolean has_fun = lua_gettop(L) > 1;
    int cmp;
    /* first, extract the two elements */
    lua_pushinteger(L, *a);
    lua_gettable(L, 1);
    lua_pushinteger(L, *b);
    lua_gettable(L, 1);
    /* next, call the comparison function */
    if(has_fun) {
	lua_pushvalue(L, 2);
	lua_pushvalue(L, -3);
	lua_pushvalue(L, -3);
	lua_call(L, 2, 1);
	/* if it's a number, assume it's -1/0/1 */
	if(lua_isnumber(L, -1)) {
	    cmp = lua_tonumber(L, -1);
	    lua_pop(L, 1);
	    return cmp;
	}
	/* otherwise, assume it's like less-than */
	cmp = lua_toboolean(L, -1);
	lua_pop(L, 1);
    } else
	cmp = lua_lessthan(L, -2, -1);
    /* if strictly less than, return that */
    if(cmp) {
	lua_pop(L, 2);
	return -1;
    }
    /* otherwise, we have to do another comparison to see if equal to */
    if(has_fun) {
	lua_pushvalue(L, 2);
	lua_pushvalue(L, -2);
	lua_pushvalue(L, -4);
	lua_call(L, 2, 1);
	cmp = lua_toboolean(L, -1);
	lua_pop(L, 1);
    } else
	cmp = lua_lessthan(L, -1, -2);
    lua_pop(L, 2);
    /* and return 1 if greater, or 0 if not (aka equal) */
    return cmp;
}

/***
Sort a table using a stable quicksort algorithm.
This is a wrapper for `g_qsort_with_data()`.  This sorts a table
in-place the same way as table.sort() does, but it performs an extra
comparison if necessary to determine of two elements are equal (i.e.,
*cmp*(a, b) == *cmp*(b, a) == false).  If so, they are sorted in the
order they appeared in the original table.  The extra comparison can be
avoided by returning a number instead of a boolean; in this case, the
number's relationship with 0 indicates a's relationship with b.
@function qsort
@see utf8_collate
@see cmp
@tparam table t Table to sort
@tparam function cmp (optional) Function to use for comparison; takes two
 table elements and returns true if the first is less than the second.  If
 not specified, Lua's standard less-than operator is used.  The function
 may also return an integer instead of a boolean, in which case the number
 must be 0, less than 0, or greater than 0, indicating a is equal to, less
 than, or greater than b, respectively.
*/
static int glib_qsort(lua_State *L)
{
    size_t nind, i;
    size_t *ind;

    luaL_checktype(L, 1, LUA_TTABLE);
    if(lua_gettop(L) > 1)
	luaL_checktype(L, 2, LUA_TFUNCTION);
    /* sort an array of indices instead of Lua array directly */
    nind = lua_objlen(L, 1);
    ind = g_malloc(nind * sizeof(*ind) * 2);
    for(i = 0; i < nind; i++)
	ind[i] = i + 1;
    g_qsort_with_data(ind, nind, sizeof(*ind), qsort_fun, L);
    /* now move the actual values around */
    /* first find where each element will end up */
    for(i = 0; i < nind; i++)
	ind[nind + ind[i] - 1] = i + 1;
    /* now do one element at a time, chaining until done */
    /* this way, only two elements are on stack at once */
    for(i = 0; i < nind; i++) {
	size_t j = ind[i], k;
	if(!j || j == i + 1)
	    continue; /* flagged as done, or already in place */
	/* x = a[i + 1] */
	lua_pushinteger(L, i + 1);
	lua_gettable(L, 1);
	/* a[i + 1] = a[j] */
	lua_pushinteger(L, i + 1);
	lua_pushinteger(L, j);
	lua_gettable(L, 1);
	lua_settable(L, 1);
	ind[i] = 0; /* mark as done */
	/* shuffle around */
	j = i + 1; /* j is what's on top of stack */
	do {
	    k = ind[nind + j - 1]; /* k is where it needs to go */
	    ind[nind + j - 1] = 0; /* flag it as empty */
	    if(ind[nind + k - 1]) { /* if not empty */
		/* y = a[k] */
		lua_pushinteger(L, k);
		lua_gettable(L, 1);
		/* swap(x, y) */
		lua_pushvalue(L, -2);
		lua_remove(L, -3);
		j = k;
	    } /* else y = x; x = empty */
	    /* a[k] = y */
	    lua_pushinteger(L, k);
	    lua_pushvalue(L, -2);
	    lua_remove(L, -3);
	    lua_settable(L, 1);
	    ind[k - 1] = 0; /* mark as done */
	} while(ind[nind + k - 1]); /* repeat until x empty */
    }
    g_free(ind);
    return 0;
}

/***
Compare two objects.
This function returns the difference between two objects.  If the *sub*
operation is available in the first object's metatable, that is called.
Otherwise, if both parameters are numbers, they are subtracted.  Otherwise,
if they are both strings, they are byte-wise compared and their relative
order is returned.  Finally, as a last resort, the standard less-than
operator is used, possibly twice, to indicate relative order.
@function cmp
@see utf8_collate
@tparam string|number a The first object
@tparam string|number b The second object
@treturn integer Greater than zero if *a* is greater than *b*; less than
 zero if *a* is less than *b*; zero if *a* is equal to *b*.
*/
static int glib_cmp(lua_State *L)
{
    if(lua_getmetatable(L, 1)) {
	lua_pushliteral(L, "__sub");
	lua_rawget(L, -2);
	lua_remove(L, -2);
	if(!lua_isnil(L, -1)) {
	    lua_pushvalue(L, 1);
	    lua_pushvalue(L, 2);
	    lua_call(L, 2, 1);
	    return 1;
	}
	lua_pop(L, 1);
    }
    if(lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
	lua_pushnumber(L, lua_tonumber(L, 1) - lua_tonumber(L, 2));
	return 1;
    }
    if(lua_isstring(L, 1) && lua_isstring(L, 2)) {
	size_t sza, szb, sz;
	const char *sa, *sb;
	int ret;

	sa = lua_tolstring(L, 1, &sza);
	sb = lua_tolstring(L, 2, &szb);
	sz = sza > szb ? szb : sza;
	ret = memcmp(sa, sb, sz);
	lua_pushinteger(L, ret ? ret : sza - szb);
	return 1;
    }
    if(lua_lessthan(L, 1, 2)) {
	lua_pushinteger(L, -1);
	return 1;
    }
    lua_pushinteger(L, lua_lessthan(L, 2, 1));
    return 1;
}

/***
Timers
@section Timers
*/

typedef struct timer_state {
    GTimer *timer;
} timer_state;

static int free_timer_state(lua_State *L)
{
    get_udata(L, 1, st, timer_state);
    if(st->timer) {
	g_timer_destroy(st->timer);
	st->timer = NULL;
    }
    return 0;
}

/***
Create a stopwatch-like timer.
This is a wrapper for `g_timer_new()`
@function timer_new
@treturn timer A timer object
*/
static int glib_timer_new(lua_State *L)
{
    alloc_udata(L, st, timer_state);
    st->timer = g_timer_new();
    return 1;
}

/****
@type timer
*/
/***
Start or reset the timer.
This is a wrapper for `g_timer_start()`
@function timer:start
*/
static int timer_start(lua_State *L)
{
    get_udata(L, 1, st, timer_state);
    if(st->timer)
	g_timer_start(st->timer);
    return 0;
}

/***
Stop the timer if it is running.
This is a wrapper for `g_timer_stop()`
@function timer:stop
*/
static int timer_stop(lua_State *L)
{
    get_udata(L, 1, st, timer_state);
    if(st->timer)
	g_timer_stop(st->timer);
    return 0;
}

/***
Resume the timer if it is stopped.
This is a wrapper for `g_timer_continue()`
@function timer:continue
*/
static int timer_continue(lua_State *L)
{
    get_udata(L, 1, st, timer_state);
    if(st->timer)
	g_timer_continue(st->timer);
    return 0;
}

/***
Return the amount of time counted so far.
This is a wrapper for `g_timer_elapsed()`
@function timer:elapsed
@treturn number The time counted so far, in seconds.
*/

static int timer_elapsed(lua_State *L)
{
    get_udata(L, 1, st, timer_state);
    if(!st->timer)
	return 0;
    lua_pushnumber(L, g_timer_elapsed(st->timer, NULL));
    return 1;
}

luaL_Reg timer_state_funcs[] = {
    {"start", timer_start},
    {"stop", timer_stop},
    {"continue", timer_continue},
    {"elapsed", timer_elapsed},
    {"__gc", free_timer_state},
    {NULL, NULL}
};

/***
Spawning Processes
@section Spawning Processes
*/

/* Notes regarding g_spawn:
 * glib has no portable way to reap a child other than using GMainContext.
 * This means a main loop needs to be created just for that.
 * In addition, there is no way to read/write pipes asynchronously on
 * Windows:  no way to poll for available data, and no way to set to
 * non-blocking.  This means that threads need to be created to read and
 * write properly.
 * Finally, there is no kill() equivalent, so no functions are provided
 * here for killing processes
 */

typedef struct spawn_state {
    GPid pid;
    gint status; /* valid if pid == 0 */
    GMainContext *mctx;
    GSource *reaper;
    GMutex lock;
    GCond signal;
    GThread *it, *ot, *et;
    /* req: */
    /* 0 == ready */
    /* -1 == suicide/eof/error */
    /* out:-2 == need line */
    /* out:-3 == need number (a non-blank segment followed by blank or EOF) */
    /* out:-4 == need all */
    int infd;
    /* input comes from here; it's freed after output */
    char *in_buf;
    /* > 0 is length of in_buf */
    gssize inreq;
    struct outinfo_t {
	int fd;
	int offset; /* pretend stuff up to offset not there */
	/* >0 == buffer read */
	gssize req;
	/* output is located here */
	/* buf.len == actual bytes read */
	/* [note: set to 0 when done with data] */
	GString buf;
    } outinfo[2];
    /* when the process dies, its status goes here */
    int waitstat;
} spawn_state;

static gpointer in_thread(gpointer data)
{
    spawn_state *st = data;
    g_mutex_lock(&st->lock);
    while(1) {
	gssize inreq;
	char *in_buf, *in_ptr;
	int infd;
	while(!(inreq = st->inreq))
	    g_cond_wait(&st->signal, &st->lock);
	in_ptr = in_buf = st->in_buf;
	st->in_buf = NULL;
	infd = st->infd;
	if(st->inreq < 0) {
	    close(infd);
	    st->infd = 0;
	}
	g_mutex_unlock(&st->lock);
	if(inreq == -1) {
	    if(in_buf)
		g_free(in_buf);
	    return NULL;
	}
	do {
	    int nw = write(st->infd, in_ptr, inreq);
	    if(nw < 0 && errno != EAGAIN && errno != EINTR)
		break;
	    if(nw > 0) {
		inreq -= nw;
		in_ptr += nw;
	    }
	} while(inreq > 0);
	g_free(in_buf);
	g_mutex_lock(&st->lock);
	st->inreq = inreq == 0 ? 0 : -1;
	g_cond_broadcast(&st->signal);
    }
}

static gpointer read_thread(spawn_state *st, int whichout)
{
    struct outinfo_t *oi = &st->outinfo[whichout];
    g_mutex_lock(&st->lock);
    while(1) {
	gssize outreq;
	gsize inlen, n_to_read, offset;
	while(!(outreq = oi->req))
	    g_cond_wait(&st->signal, &st->lock);
	inlen = oi->buf.len;
	offset = oi->offset;
	g_mutex_unlock(&st->lock);
	if(outreq == -1) {
	    close(oi->fd);
	    return NULL;
	}
	/* shortcut if nbytes already read */
	if(outreq > 0 && inlen >= offset + outreq) {
	    g_mutex_lock(&st->lock);
	    g_cond_broadcast(&st->signal);
	    oi->req = 0;
	    continue;
	}
	/* shortcut if a line already read for line mode */
	if(outreq == -2 && inlen > offset &&
	   memchr(oi->buf.str + offset, '\n', inlen - offset)) {
	    g_mutex_lock(&st->lock);
	    g_cond_broadcast(&st->signal);
	    oi->req = 0;
	    continue;
	}
	/* shortcut if a possible number read for number mode */
	if(outreq == -3 && inlen > offset) {
	    const char *s;
	    for(s = oi->buf.str + offset; s < oi->buf.str + inlen; s++)
		if(!isspace(*s))
		    break;
	    for(; s < oi->buf.str + inlen; s++)
		if(isspace(*s))
		    break;
	    if(s < oi->buf.str + inlen) {
		g_mutex_lock(&st->lock);
		g_cond_broadcast(&st->signal);
		oi->req = 0;
		continue;
	    }
	}
	n_to_read = outreq > 0 ? outreq : 128;
	while(1) {
	    gssize nread;
	    while(n_to_read <= inlen)
		n_to_read *= 2;
	    if(n_to_read > oi->buf.allocated_len) {
		g_mutex_lock(&st->lock);
		g_string_set_size(&oi->buf, n_to_read);
		oi->buf.len = inlen;
		g_mutex_unlock(&st->lock);
	    }
	    nread = read(oi->fd, oi->buf.str + inlen, n_to_read - inlen);
	    if(nread < 0 && errno != EAGAIN && errno != EINTR) {
		outreq = -1;
		break;
	    }
	    if(!nread) {
		outreq = -1;
		break;
	    }
	    if(nread < 0)
		continue;
	    if(outreq == -2 && memchr(oi->buf.str + inlen, '\n', nread)) {
		outreq = 0;
		inlen += nread;
		break;
	    }
	    if(outreq == -3) {
		const char *s;
		for(s = oi->buf.str + (inlen > offset ? inlen - 1 : inlen);
		    s < oi->buf.str + inlen + nread; s++)
		    if(!isspace(*s))
			break;
		for(; s < oi->buf.str + inlen + nread; s++)
		    if(isspace(*s))
			break;
		if(s < oi->buf.str + inlen + nread) {
		    outreq = 0;
		    inlen += nread;
		    break;
		}
	    }
	    inlen += nread;
	    if(outreq > 0 && outreq <= inlen) {
		outreq = 0;
		break;
	    }
	}
	g_mutex_lock(&st->lock);
	oi->buf.len = inlen;
	oi->req = outreq;
	g_cond_broadcast(&st->signal);
    }
}

static gpointer out_thread(gpointer data)
{
    spawn_state *st = data;
    return read_thread(st, 0);
}

static gpointer err_thread(gpointer data)
{
    spawn_state *st = data;
    return read_thread(st, 1);
}

static void proc_reap(GPid pid, gint status, gpointer user_data)
{
    spawn_state *st = user_data;
    g_mutex_lock(&st->lock);
    st->status = status;
    st->pid = 0;
    g_spawn_close_pid(pid);
    g_cond_broadcast(&st->signal);
    g_mutex_unlock(&st->lock);
}

/* FIXME: ensure that file descriptors are not gc'd */
/***
Run a command asynchronously.
This is a wrapper for `g_spawn_async_with_pipes()`.  The other spawn
functions can easily be emulated using this.
@function spawn
@tparam table|string args If the parameter is a string, parse the string
as a shell command and execute it.  Otherwise, the command is taken from
the table.  If the table has no array component, the command must be
specified using the *cmd* field.  Otherwise, the array elements specify
the argument vector of the command.  If both are provided, the command
to actually execute is *cmd*, but the first array element is still
passed in as the proposed process name.  In addition to the command arguments
and *cmd* field, the following fields specify options for the spawned
command:

 * *stdin*: **file|string|boolean** (default = false) Specify the standard
   input to the process.  If this is a file descriptor, it must
   be opened in read mode; its contents will be the process'
   standard input.  If this is a string, it names a file to
   be opened for reading.  The file name may be blank to indicate
   that the standard input should be inherited from the current
   process.  The file name may be prefixed with an exclamation
   point to open in binary mode; otherwise it is opened in
   normal (text) mode.  Any other value is evaluated as a
   boolean; true means that a pipe should be opened such that
   proc:write() writes to the process, and false means that
   no standard input should be provided at all (equivalent to
   UNIX /dev/null).
 * *stdout*: **file|string|boolean** (default = true) Specify the standard
   output for the process.  If this is a file descriptor, it must
   be opened in write mode; the process' standard output will be
   written to that file.  If this is a string, it names a file to
   be opened for appending.  The file name may be blank to indicate
   that the standard output should be inherited from the current
   process.  The file name may be prefixed with an exclamation
   point to open in binary mode; otherwise it is opened in
   normal (text) mode.  Any other value is evaluated as a
   boolean; true means that a pipe should be opened such that
   proc:read() reads from the process, and false means that
   standard output should be ignored.
 * *stderr*: **file|string|boolean** (default = "") Specify the standard
   error for the process.  See the *stdout* description for
   details; the only difference is in which functions are used
   to read from the pipe (read\_err and friends).
 * *env*: **table** Specify the environment for the process.  If this
   is not provided, the environment is inherited from the parent.
   Otherwise, all keys in the table correspond to variables, and
   the values correspond to those variables' values.  Only these
   variables will be set in the spawned process.
 * *path* **boolean** (default = true) If this is present and false, do
   not use the standard system search path to find the command
   to execute.
 * *chdir* **string** If this is present, change to the given directory
   when executing the commmand.  Otherwise, it will execute in the
   current working directory.
 * *cmd* **string** If this is present, and there are no array entries in
   this table, it is parsed as a shell command to construct the
   command to run.  Otherwise, it is the command to execute instead
   of the first element of the argument array.
@treturn process An object representing the process.
*/
static int glib_spawn(lua_State *L)
{
    FILE *inf, *outf, *errf;
    int newin = -1, newout = -1, newerr = 0;
    gboolean ipipe, opipe, epipe;
    const char *chdir = NULL;
    gboolean use_path = TRUE;
    const char *cmd = NULL;
    int nargs = 0;
    gchar **argv, **env = NULL;
    alloc_udata(L, st, spawn_state);
    ipipe = epipe = FALSE;
    opipe = TRUE;
    st->infd = st->outinfo[0].fd = st->outinfo[1].fd = -1;
    inf = outf = errf = NULL;
    if(lua_istable(L, 1)) {
	nargs = lua_objlen(L, 1);
	lua_getfield(L, 1, "path");
	if(!lua_isnil(L, -1))
	    use_path = lua_toboolean(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, 1, "chdir");
	if(!lua_isnil(L, -1)) {
	    if(!lua_isstring(L, -1))
		luaL_argerror(L, 1, "chdir must be a string");
	    chdir = lua_tostring(L, -1);
	}
	lua_pop(L, 1);
	lua_getfield(L, 1, "cmd");
	if(!lua_isnil(L, -1)) {
	    if(!lua_isstring(L, -1))
		luaL_argerror(L, 1, "cmd must be a string");
	    cmd = lua_tostring(L, -1);
	} else if(nargs == 0)
	    luaL_argerror(L, 1, "no command specified");
	lua_pop(L, 1);
	lua_getfield(L, 1, "env");
	if(!lua_isnil(L, -1)) {
	    int nenv;
	    if(!lua_istable(L, -1))
		luaL_argerror(L, 1, "env must be a table");
	    lua_pushnil(L);
	    for(nenv = 0; lua_next(L, -2); nenv++)
		lua_pop(L, 1);
	    env = g_malloc((nenv + 1) * sizeof(*env));
	    lua_pushnil(L);
	    for(nenv = 0; lua_next(L, -2); nenv++) {
		lua_pushvalue(L, -2);
		lua_pushliteral(L, "=");
		lua_pushvalue(L, -3);
		lua_concat(L, 3);
		env[nenv] = g_strdup(lua_tostring(L, -1));
		lua_pop(L, 2);
	    }
	    env[nenv] = NULL;
	}
	lua_pop(L, 1);
	lua_getfield(L, 1, "stdin");
	if(lua_isuserdata(L, -1)) {
	    /* the only type of userdata supported is FILE, so check. */
	    FILE **inf = luaL_checkudata(L, -1, LUA_FILEHANDLE);
	    if(*inf)
		newin = fileno(*inf);
	    /* else inf == /dev/null */
	} else if(lua_isstring(L, -1)) {
	    const char *fn = lua_tostring(L, -1);
	    if(*fn) {
		gboolean isb = *fn == '!';
		inf = fopen(isb ? fn + 1 : fn, isb ? "rb" : "r");
		if(!inf) {
		    g_strfreev(env);
		    lua_pushnil(L);
		    lua_pushliteral(L, "Can't open stdin ");
		    lua_pushvalue(L, -3);
		    lua_pushliteral(L, ": ");
		    lua_pushstring(L, strerror(errno));
		    lua_concat(L, 3);
		    return 2;
		}
		newin = fileno(inf);
	    } else
		newin = 0;
	} else
	    ipipe = lua_toboolean(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, 1, "stdout");
	if(lua_isuserdata(L, -1)) {
	    /* the only type of userdata supported is FILE, so check. */
	    FILE **outf = luaL_checkudata(L, -1, LUA_FILEHANDLE);
	    opipe = FALSE;
	    if(*outf)
		newout = fileno(*outf);
	    /* else outf == /dev/null */
	} else if(lua_isstring(L, -1)) {
	    const char *fn = lua_tostring(L, -1);
	    opipe = FALSE;
	    if(*fn) {
		gboolean isb = *fn == '!';
		outf = fopen(isb ? fn + 1 : fn, isb ? "ab" : "a");
		if(!outf) {
		    g_strfreev(env);
		    if(inf)
			fclose(inf);
		    lua_pushnil(L);
		    lua_pushliteral(L, "Can't open stdout ");
		    lua_pushvalue(L, -3);
		    lua_pushliteral(L, ": ");
		    lua_pushstring(L, strerror(errno));
		    lua_concat(L, 3);
		    return 2;
		}
		newout = fileno(outf);
	    } else
		newout = 0;
	} else if(!lua_isnil(L, -1))
	    opipe = lua_toboolean(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, 1, "stderr");
	if(lua_isuserdata(L, -1)) {
	    /* the only type of userdata supported is FILE, so check. */
	    FILE **errf = luaL_checkudata(L, -1, LUA_FILEHANDLE);
	    if(*errf)
		newerr = fileno(*errf);
	    /* else errf == /dev/null */
	} else if(lua_isstring(L, -1)) {
	    const char *fn = lua_tostring(L, -1);
	    if(*fn) {
		gboolean isb = *fn == '!';
		errf = fopen(isb ? fn + 1 : fn, isb ? "ab" : "a");
		if(!errf) {
		    g_strfreev(env);
		    if(inf)
			fclose(inf);
		    if(outf)
			fclose(outf);
		    lua_pushnil(L);
		    lua_pushliteral(L, "Can't open stdin ");
		    lua_pushvalue(L, -3);
		    lua_pushliteral(L, ": ");
		    lua_pushstring(L, strerror(errno));
		    lua_concat(L, 3);
		    return 2;
		}
		newerr = fileno(errf);
	    } else
		newerr = 0;
	} else if(!lua_isnil(L, -1))
	    epipe = lua_toboolean(L, -1);
	lua_pop(L, 1);
    } else
	cmd = luaL_checkstring(L, 1);
    if(cmd && !nargs) {
	gint argc;
	if(!g_shell_parse_argv(cmd, &argc, &argv, NULL)) {
	    argv = g_malloc(2 * sizeof(*argv));
	    argv[0] = g_strdup(cmd);
	    argv[1] = NULL;
	}
    } else {
	int i, j;
	argv = g_malloc((nargs + 1 + (cmd ? 1 : 0)) * sizeof(*argv));
	i = 0;
	if(cmd)
	    argv[i++] = g_strdup(cmd);
	for(j = 0; j < nargs; j++, i++) {
	    lua_pushinteger(L, j + 1);
	    lua_gettable(L, 1);
	    argv[i] = g_strdup(lua_tostring(L, -1));
	    if(!argv[i])
		argv[i] = g_strdup("");
	    lua_pop(L, 1);
	}
	argv[i] = NULL;
    }
    {
	GError *err = NULL;
	int oldin = -1, oldout = -1, olderr = -1;
	GSpawnFlags fl = G_SPAWN_DO_NOT_REAP_CHILD;
	if(use_path)
	    fl |= G_SPAWN_SEARCH_PATH;
	if(!opipe && newout < 0)
	    fl |= G_SPAWN_STDOUT_TO_DEV_NULL;
	if(!epipe && newerr < 0)
	    fl |= G_SPAWN_STDERR_TO_DEV_NULL;
	if(newin >= 0)
	    fl |= G_SPAWN_CHILD_INHERITS_STDIN;
	if(nargs && cmd)
	    fl |= G_SPAWN_FILE_AND_ARGV_ZERO;
	/**** saving/restoring file descriptors is not thread-safe ****/
	/* it'd be nice if glib required init of in/out/err fds */
	/* and only set up pipes if fd < 0 */
	if(newin > 0) {
	    oldin = dup(0);
	    dup2(newin, 0);
	}
	if(newout > 0) {
	    oldout = dup(1);
	    dup2(newout, 1);
	}
	if(newerr > 0) {
	    olderr = dup(2);
	    dup2(newerr, 2);
	}
	g_spawn_async_with_pipes(chdir, argv, env, fl, NULL, NULL, &st->pid,
				 ipipe ? &st->infd : NULL,
				 opipe ? &st->outinfo[0].fd : NULL,
				 epipe ? &st->outinfo[1].fd : NULL,
				 &err);
	if(newin > 0) {
	    dup2(oldin, 0);
	    close(oldin);
	}
	if(newout > 0) {
	    dup2(oldout, 1);
	    close(oldout);
	}
	if(newerr > 0) {
	    dup2(olderr, 2);
	    close(olderr);
	}
	if(err) {
	    g_strfreev(env);
	    g_strfreev(argv);
	    if(inf)
		fclose(inf);
	    if(outf)
		fclose(outf);
	    if(errf)
		fclose(errf);
	    lua_pushnil(L);
	    lua_pushstring(L, err->message);
	    return 2;
	}
	st->mctx = g_main_context_new();
	st->reaper = g_child_watch_source_new(st->pid);
	g_source_set_callback(st->reaper, (GSourceFunc)proc_reap, st, NULL);
	g_source_attach(st->reaper, st->mctx);
	g_mutex_init(&st->lock);
	g_cond_init(&st->signal);
	if(ipipe)
	    st->it = g_thread_new("in", in_thread, st);
	if(opipe)
	    st->ot = g_thread_new("out", out_thread, st);
	if(epipe)
	    st->et = g_thread_new("err", err_thread, st);
    }
    g_strfreev(env);
    g_strfreev(argv);
    if(inf)
	fclose(inf);
    if(outf)
	fclose(outf);
    if(errf)
	fclose(errf);
    return 1;
}

/***
@type process
*/
static int read_ready(lua_State *L, spawn_state *st, int whichout)
{
    int nargs = lua_gettop(L) - 1, i;
    struct outinfo_t *oi = &st->outinfo[whichout];
    gsize offset = 0;

    g_mutex_lock(&st->lock);
    if(oi->req == -1) {
	g_mutex_unlock(&st->lock);
	lua_pushboolean(L, TRUE);
	return 1;
    }
    if(!nargs) {
	if(oi->buf.len > 0 && memchr(oi->buf.str, '\n', oi->buf.len)) {
	    g_mutex_unlock(&st->lock);
	    lua_pushboolean(L, TRUE);
	    return 1;
	}
	if(!oi->req) {
	    oi->offset = 0;
	    oi->req = -2;
	    g_cond_broadcast(&st->signal);
	}
	g_mutex_unlock(&st->lock);
	lua_pushboolean(L, FALSE);
	return 1;
    }
    for(i = 0; i < nargs; i++) {
	if(lua_type(L, i + 2) == LUA_TNUMBER) {
	    gsize bsize = lua_tointeger(L, i + 2);
	    if(oi->buf.len - offset >= bsize)
		offset += bsize;
	    else {
		if(!oi->req) {
		    oi->offset = offset;
		    oi->req = bsize;
		    g_cond_broadcast(&st->signal);
		}
		g_mutex_unlock(&st->lock);
		lua_pushboolean(L, FALSE);
		return 1;
	    }
	} else {
	    const char *s = lua_tostring(L, i + 2);
	    if(!s || *s != '*') {
		g_mutex_unlock(&st->lock);
		luaL_argerror(L, i + 2, "invalid option");
	    }
	    if(s[1] == 'l') {
		const char *eol = memchr(oi->buf.str + offset, '\n',
					 oi->buf.len - offset);
		if(eol) {
		    offset = (int)(eol - oi->buf.str) + 1;
		    continue;
		}
		if(!oi->req) {
		    oi->offset = offset;
		    oi->req = -2;
		    g_cond_broadcast(&st->signal);
		}
		g_mutex_unlock(&st->lock);
		lua_pushboolean(L, FALSE);
		return 1;
	    } else if(s[1] == 'n') {
		gsize p;
		for(p = offset; p < oi->buf.len; p++)
		    if(!isspace(oi->buf.str[p]))
			break;
		for(++p; p < oi->buf.len; p++)
		    if(isspace(oi->buf.str[p]))
			break;
		if(p < oi->buf.len) {
		    int epos, nconv;
		    lua_Number n;
		    char c = oi->buf.str[p];
		    oi->buf.str[p] = 0;
		    nconv = sscanf(oi->buf.str + offset, LUA_NUMBER_SCAN "%n",
				   &n, &epos);
		    oi->buf.str[p] = c;
		    if(nconv > 0)
			offset += epos;
		    /* lua will abort the read at this point, anyway */
		    else {
			g_mutex_unlock(&st->lock);
			lua_pushboolean(L, TRUE);
			return 1;
		    }
		} else {
		    if(!oi->req) {
			oi->offset = offset;
			oi->req = -3;
			g_cond_broadcast(&st->signal);
		    }
		    g_mutex_unlock(&st->lock);
		    lua_pushboolean(L, FALSE);
		    return 1;
		}
	    } else if(s[1] == 'a') {
		if(!oi->req) {
		    oi->offset = 0;
		    oi->req = -4;
		    g_cond_broadcast(&st->signal);
		}
		g_mutex_unlock(&st->lock);
		lua_pushboolean(L, FALSE);
		return 1;
	    } else {
		g_mutex_unlock(&st->lock);
		luaL_argerror(L, i + 2, "invalid format");
	    }
	}
    }
    g_mutex_unlock(&st->lock);
    lua_pushboolean(L, TRUE);
    return 1;
}

/***
Wait for input to be available from process standard output.
This function is to support non-blocking input from the process.  It
takes the same parameters as read, and returns true if that read would
succeed without blocking.  It accomplishes this by attempting the requested
read in a background thread, and returning success when the data has actually
been read.  Due to buffer sizes used and other issues, the reader thread
might hang waiting for input even when enough data is available, depending
on operating system.  On Linux, at least, it should never hang.
@function process:read_ready
@see process:read
@tparam string|number ... See read for details.  For the '*n' format, since
it is difficult to tell how much input will be required, the thread will
read until it finds a non-blank word.
@treturn boolean True if reading using the given format(s) will succeed
 without blocking.
*/
static int out_ready(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    if(!st->ot) {
	lua_pushnil(L);
	lua_pushliteral(L, "Output channel not open");
	return 2;
    }
    return read_ready(L, st, 0);
}

/***
Wait for input to be available from process standard error.
See the documentation for read\_ready for details.
@function process:read_err_ready
@see process:read_ready
@tparam string|number ... See read\_ready for details. 
@treturn boolean True if reading using the given format(s) will succeed
 without blocking.
*/
static int err_ready(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    if(!st->et) {
	lua_pushnil(L);
	lua_pushliteral(L, "Output channel not open");
	return 2;
    }
    return read_ready(L, st, 1);
}

static int read_pipe(lua_State *L, spawn_state *st, int whichout,
		     int (*ready)(lua_State *L))
{
    struct outinfo_t *oi = &st->outinfo[whichout];
    int nargs = lua_gettop(L) - 1, i;
    gsize offset = 0;
    while(1) {
	if(ready(L) == 2) /* error == nil + msg */
	    return 2;
	if(lua_toboolean(L, -1)) {
	    lua_pop(L, 1);
	    break;
	}
	lua_pop(L, 1);
	g_mutex_lock(&st->lock);
	while(oi->req && oi->req != -1)
	    g_cond_wait(&st->signal, &st->lock);
	g_mutex_unlock(&st->lock);
    }
    g_mutex_lock(&st->lock);
    if(!oi->buf.len) {
	g_mutex_unlock(&st->lock);
	lua_pushnil(L);
	return 1;
    }
    if(!nargs) {
	const char *s = memchr(oi->buf.str, '\n', oi->buf.len);
	if(s && s < oi->buf.str + oi->buf.len - 1) {
	    gsize len = (gsize)(s - oi->buf.str);
	    /* since we're reading lines, it's safe to strip trailing CR */
	    if(len && s[-1] == '\r')
		len--;
	    lua_pushlstring(L, oi->buf.str, len);
	    oi->buf.len -= len;
	    memmove(oi->buf.str, s + 1, oi->buf.len);
	} else {
	    gsize len = oi->buf.len;
	    if(len && oi->buf.str[len - 1] == '\r')
		len--;
	    lua_pushlstring(L, oi->buf.str, len);
	    oi->buf.len = 0;
	}
	g_mutex_unlock(&st->lock);
	return 1;
    }
    for(i = 0; i < nargs; i++) {
	if(offset == oi->buf.len) {
	    g_mutex_unlock(&st->lock);
	    lua_pushnil(L);
	    return i + 1;
	}
	if(lua_type(L, i + 2) == LUA_TNUMBER) {
	    gsize bsize = lua_tointeger(L, i + 2);
	    if(oi->buf.len - offset < bsize)
		bsize = oi->buf.len - offset;
	    lua_pushlstring(L, oi->buf.str + offset, bsize);
	    offset += bsize;
	} else {
	    const char *s = lua_tostring(L, i + 2);
	    if(s[1] == 'l') {
		const char *eol = memchr(oi->buf.str + offset, '\n',
					 oi->buf.len - offset);
		if(eol) {
		    gsize len = (gsize)(eol - oi->buf.str) - offset;
		    /* since we're reading lines, it's safe to strip trailing CR */
		    if(eol > oi->buf.str + offset && eol[-1] == '\r')
			len--;
		    lua_pushlstring(L, oi->buf.str + offset, len);
		    offset = (int)(eol - oi->buf.str) + 1;
		} else {
		    gsize len = oi->buf.len - offset;
		    /* even though it's at EOF, it's still safe to remove \r */
		    if(oi->buf.len > offset && oi->buf.str[oi->buf.len - 1] == '\r')
			len--;
		    lua_pushlstring(L, oi->buf.str + offset, len);
		    offset = oi->buf.len;
		}
	    } else if(s[1] == 'n') {
		int epos, nconv;
		lua_Number n;
		
		gsize p;
		for(p = offset; p < oi->buf.len; p++)
		    if(!isspace(oi->buf.str[p]))
			break;
		for(++p; p < oi->buf.len; p++)
		    if(isspace(oi->buf.str[p]))
			break;
		if(p == oi->buf.allocated_len) {
		    GString *copy = g_string_sized_new(p - offset);
		    memcpy(copy->str, oi->buf.str + offset, p - offset);
		    copy->str[p - offset] = 0;
		    nconv = sscanf(copy->str, LUA_NUMBER_SCAN "%n", &n, &epos);
		    g_string_free(copy, TRUE);
		} else
		    nconv = sscanf(oi->buf.str + offset, LUA_NUMBER_SCAN "%n",
				   &n, &epos);
		if(nconv < 1) {
		    lua_pushnil(L);
		    ++i;
		    break;
		}
		lua_pushnumber(L, n);
		offset += epos;
	    } else if(s[1] == 'a') {
		lua_pushlstring(L, oi->buf.str + offset, oi->buf.len - offset);
		offset = oi->buf.len;
	    }
	}
    }
    if(offset != oi->buf.len && oi->buf.len && offset)
	memmove(oi->buf.str, oi->buf.str + offset, oi->buf.len);
    oi->buf.len -= offset;
    g_mutex_unlock(&st->lock);
    return i;
}

/***
Read data from a process' standard output.
This function is a clone of the standard Lua file:read function.  It defers
actual I/O to the read\_ready routine, which in turn lets a background
thread do all of the reading.  It will block until read\_ready is true, and
then read directly from the buffer read by the thread.
@function process:read
@see process:read_ready
@tparam string|number ... If no paramters are given, read a single
 newline-terminated line from input, and strip the trailing newline.
 Otherwsie, for each parameter, until failure, a result is read and
 returned based on the parameter.  If the parameter is a number, then
 that many bytes (or fewer) are read.  Otherwise, the parameter must
 be a string containing one of the following:

 * '*l' -- read a line in the same way as the empty argument list does.
 * '*n' -- read a number from input; in this case, a number is returned
           instead of a string.
 * '*a' -- read the entire remainder of the input.  Note that if this
           is given as a format to read\_ready, all standard output
           from the process will be read as soon as it is available.

@treturn string|number|nil... For each parameter (or for the line read by the
 empty parameter list), the results of reading that format are returned.
 If an error occurred for any parameter, nil is returned for that parameter
 and no further parameters are processed.
*/
static int out_read(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    return read_pipe(L, st, 0, out_ready);
}

/***
Read data from a process' standard error.
See the read function documentation for details.
@function process:read_err
@see process:read
@tparam string|number ... See the read function documentation for details.
@treturn string|number|nil... See the read function documentation for details.
*/
static int err_read(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    return read_pipe(L, st, 1, err_ready);
}

static int out_lines_iter(lua_State *L)
{
    lua_settop(L, 1);
    return out_read(L);
}

/***
Return an iterator which reads lines from the process' standard output.
On each iteration, this returns the result of read().
@function process:lines
@see process:read
@treturn function The iterator.
*/
static int out_lines(lua_State *L)
{
    luaL_checkudata(L, 1, "glib.spawn_state");
    lua_pushcfunction(L, out_lines_iter);
    lua_pushvalue(L, 1);
    return 2;
}

static int err_lines_iter(lua_State *L)
{
    lua_settop(L, 1);
    return err_read(L);
}

/***
Return an iterator which reads lines from the process' standard error.
On each iteration, this returns the result of read\_err().
@function process:lines_err
@see process:read_err
@treturn function The iterator.
*/
static int err_lines(lua_State *L)
{
    luaL_checkudata(L, 1, "glib.spawn_state");
    lua_pushcfunction(L, err_lines_iter);
    lua_pushvalue(L, 1);
    return 2;
}

/***
Check if writing to the process' standard input will block.
Writing to a process' standard input is made non-blocking by running
writes in a background thread.
@function process:write_ready
@see process:write
@treturn boolean True if a call to process:write() will not block.
*/
static int in_ready(lua_State *L)
{
    gboolean ready;
    get_udata(L, 1, st, spawn_state);
    if(!st->it) {
	lua_pushnil(L);
	lua_pushliteral(L, "Input channel not open");
	return 2;
    }
    g_mutex_lock(&st->lock);
    ready = st->inreq <= 0;
    g_mutex_unlock(&st->lock);
    lua_pushboolean(L, ready);
    return 1;
}

/* FIXME: should the string be ref'd? */
/***
Write to a process' standard input.
Writes all arguments to the process' standard input.  It does this using a
background writer thread that writes each argument before allowing the next
write.  In other words, the first write will not block, but subsequent
writes will bock until the previous write has completed.
@function process:write
@see process:write_ready
@tparam string... ... All strings are written, in the order given.
@treturn boolean Returns true on success.  However, since the write has
 not truly completed until the background writer has finished, the only
 way to ensure that writing was complete and successful is to use
 write\_ready or io\_wait.
*/
static int in_write(lua_State *L)
{
    int nargs = lua_gettop(L) - 1, i;
    get_udata(L, 1, st, spawn_state);
    if(!st->it) {
	lua_pushnil(L);
	lua_pushliteral(L, "Input channel not open");
	return 2;
    }
    for(i = 0; i < nargs; i++) {
	GString ns;
	size_t l;
	const char *s;
	char *p;
	if(lua_type(L, i + 2) == LUA_TNUMBER) {
	    memset(&ns, 0, sizeof(ns));
	    g_string_printf(&ns, LUA_NUMBER_FMT, lua_tonumber(L, i + 2));
	    p = ns.str;
	    l = ns.len;
	} else {
	    s = luaL_checklstring(L, i + 2, &l);
	    if(!l)
		continue;
	    p = g_strdup(s);
	}
	g_mutex_lock(&st->lock);
	while(st->inreq > 0)
	    g_cond_wait(&st->signal, &st->lock);
	if(st->inreq == -1) {
	    g_mutex_unlock(&st->lock);
	    g_free(p);
	    lua_pushnil(L);
	    return 1;
	}
	st->inreq = l;
	st->in_buf = p;
	g_cond_broadcast(&st->signal);
	g_mutex_unlock(&st->lock);
    }
    /* can't really be sure write succeded until next time */
    lua_pushboolean(L, TRUE);
    return 0;
}

/***
Close the process' standard input channel.
This function flushes any pending writes and closes the input channel.
Many processes which take input from standard input need this to detect
the end of input in order to continue processing.
@function process:close
*/
static int in_close(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    if(st->it) {
	g_mutex_lock(&st->lock);
	while(st->inreq > 0)
	    g_cond_wait(&st->signal, &st->lock);
	st->inreq = -1;
	g_cond_broadcast(&st->signal);
	g_mutex_unlock(&st->lock);
	g_thread_join(st->it);
	st->it = NULL;
    }
    return 0;
}

/* FIXME: make this blocking */
/* that will require turning the i/o threads into glib sources to signal */
/* the child reaper */
/***
Check for process activity.
Check to see if I/O is in progress or the process has died.
@function process:io_wait
@tparam boolean check_in (optional) Return a flag indicating if the
 background standard input writer is idle.
@tparam boolean check_out (optional) Return a flag indicating if the
 background standard output reader is idle.
@tparam boolean check_err (optional) Return a flag indicating if the
 background standard error reader is idle.
@treturn boolean True if the standard input thread is idle; only returned if
 requested
@treturn boolean True if the standard output thread is idle; only returned if
 requested
@treturn boolean True if the standard error thread is idle; only returned if
 requested
@treturn boolean True if the process is no longer running.
*/
static int proc_iowait(lua_State *L)
{
    gboolean check_in = lua_toboolean(L, 2);
    gboolean check_out = lua_toboolean(L, 3);
    gboolean check_err = lua_toboolean(L, 4);
    get_udata(L, 1, st, spawn_state);
    g_main_context_iteration(st->mctx, FALSE);
    g_mutex_lock(&st->lock);
    if(check_in)
	lua_pushboolean(L, st->inreq <= 0 && st->pid);
    if(check_out)
	lua_pushboolean(L, st->outinfo[0].req == 0 ||
			   st->outinfo[0].req == -1);
    if(check_err)
	lua_pushboolean(L, st->outinfo[1].req == 0 ||
			   st->outinfo[1].req == -1);
    lua_pushboolean(L, !st->pid);
    g_mutex_unlock(&st->lock);
    return 1 + (check_in ? 1 : 0) + (check_out ? 1 : 0) + (check_err ? 1 : 0);
}

/***
Return the glib process ID for the process.
Using the process ID for anything other than printing debug messages is
non-portable.
@function process:pid
@treturn number GLib process ID (which may or may not correspond to
 a system process ID)
*/
static int proc_pid(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    lua_pushnumber(L, st->pid);
    return 1;
}

/***
Return the status of the running process.
@function process:status
@treturn string|number If the process is running, the string 'running'
 is returned.  Otherwise, the numeric exit code from the process
 is returned.
*/
static int proc_status(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    g_main_context_iteration(st->mctx, FALSE);
    g_mutex_lock(&st->lock);
    if(st->pid)
	lua_pushstring(L, "running");
    else
	lua_pushnumber(L, st->status);
    g_mutex_unlock(&st->lock);
    return 1;
}

static void ready_all(lua_State *L, spawn_state *st, int whichout)
{
    struct outinfo_t *oi = &st->outinfo[whichout];
    g_mutex_lock(&st->lock);
    while(oi->req != -1 && oi->req != 0)
	g_cond_wait(&st->signal, &st->lock);
    if(oi->req == 0) {
	oi->req = -4;
	g_cond_broadcast(&st->signal);
    }
    g_mutex_unlock(&st->lock);
}

static void read_all(lua_State *L, spawn_state *st, int whichout)
{
    struct outinfo_t *oi = &st->outinfo[whichout];
    g_mutex_lock(&st->lock);
    while(oi->req != -1 && oi->req != 0)
	g_cond_wait(&st->signal, &st->lock);
    lua_pushlstring(L, oi->buf.str, oi->buf.len);
    g_mutex_unlock(&st->lock);
}

/***
Wait for process termination and clean up.
This function starts background reads for all data on the standard output
and standard error channels, and flushes and closes the standard input
channel.  It then waits for the process to finish.  Once finished, the
result code from the process and any gathered standard input and standard
error are returned.
@function process:wait
@treturn number Result code from the process
@treturn string If a standard output pipe was in use, this is the remaining
 data on the pipe.
@treturn string If a standard error pipe was in use, this is the reaming
 data on the pipe.
*/
static int proc_finish(lua_State *L)
{
    int nret = 1;
    get_udata(L, 1, st, spawn_state);
    /* first, receive all pending output in background */
    if(st->ot)
	ready_all(L, st, 0);
    if(st->et)
	ready_all(L, st, 1);
    /* then, flush pending input */
    in_close(L);
    /* then, wait for process to finish */
#if 0 /* this won't work, because context_iteration needs to be called */
    g_mutex_lock(&st->lock);
    while(st->pid)
	g_cond_wait(&st->signal, &st->lock);
#else
    while(st->pid)
	g_main_context_iteration(st->mctx, TRUE);
#endif
    lua_pushinteger(L, st->status);
    /* finally, get all remaing data from output channels */
    if(st->ot) {
	read_all(L, st, 0);
	g_thread_join(st->ot);
	st->ot = NULL;
	++nret;
    }
    if(st->et) {
	read_all(L, st, 1);
	g_thread_join(st->et);
	st->et = NULL;
	++nret;
    }
    return nret;
}

static int free_spawn_state(lua_State *L)
{
    get_udata(L, 1, st, spawn_state);
    lua_pop(L, proc_finish(L));
    if(st->reaper) {
	g_source_destroy(st->reaper);
	st->reaper = NULL;
    }
    if(st->mctx) {
	g_main_context_unref(st->mctx);
	st->mctx = NULL;
    }
    if(st->in_buf)
	g_free(st->in_buf);
    if(st->outinfo[0].buf.str)
	g_free(st->outinfo[0].buf.str);
    if(st->outinfo[1].buf.str)
	g_free(st->outinfo[1].buf.str);
    if(st->pid)
	g_spawn_close_pid(st->pid);
    memset(st, 0, sizeof(*st));
    return 0;
}

static int proc_kill(lua_State *L)
{
    GPid pid;
    get_udata(L, 1, st, spawn_state);
    pid = st->pid;
    if(pid) {
#ifdef G_OS_WIN32
	/* GLib claims pid is a handle */
	lua_pushboolean(L, TerminateProcess(pid, -1)); /* SIGKILL, basically */
#else
	int sig = SIGTERM; /* friendlier than SIGKILL above */
	if(lua_gettop(L) > 1)
	    sig = luaL_checknumber(L, 2);
	lua_pushboolean(L, !kill(pid, sig));
#endif
    }
    return 1;
}

luaL_Reg spawn_state_funcs[] = {
    {"read", out_read},
    {"read_err", err_read},
    {"read_ready", out_ready},
    {"read_err_ready", err_ready},
    {"kill", proc_kill},
    {"lines", out_lines},
    {"lines_err", err_lines},
    {"write", in_write},
    {"write_ready", in_ready},
    {"close", in_close},
    {"io_wait", proc_iowait},
    {"pid", proc_pid},
    {"status", proc_status},
    {"wait", proc_finish},
    {"__gc", free_spawn_state},
    {NULL, NULL}
};

/***
File Utilities
@section File Utilities
*/

#define file_test(t, n) \
    static int glib_##n(lua_State *L) \
    { \
	lua_pushboolean(L, g_file_test(luaL_checkstring(L, 1), G_FILE_TEST_##t)); \
	return 1; \
    } \

/***
Test if the given path points to a file.
This function is a wrapper for `g_file_test()`
@function is_file
@tparam string name The path name to test
@treturn boolean true if *name* names a plain file
*/
file_test(IS_REGULAR, is_file)
/***
Test if the given path points to a directory.
This function is a wrapper for `g_file_test()`
@function is_dir
@tparam string name The path name to test
@treturn boolean true if *name* names a plain file
*/
file_test(IS_DIR, is_dir)
/***
Test if the given path points to a symbolic link.
This function is a wrapper for `g_file_test()`.  Note that if this returns
true, the is\_dir and is\_file tests may still return true as well, since
they follow symbolic links.
@function is_symlink
@tparam string name The path name to test
@treturn boolean true if *name* names a symbolic link
*/
file_test(IS_SYMLINK, is_symlink)
/***
Test if the given path points to an executable file.
This function is a wrapper for `g_file_test()`.  Note that GLib uses
heuristics on Windows, since there is no executable bit to test.
@function is_exec
@tparam string name The path name to test
@treturn boolean true if *name* names an executable file.
*/
file_test(IS_EXECUTABLE, is_exec)
/***
Test if the given path points to a file or directory.
This function is a wrapper for `g_file_test()`.  Note that invalid symbolic
links return false, even though they actually exist.
@function exists
@tparam string name The path name to test
@treturn boolean true if *name* exists in the file system
*/
file_test(EXISTS, exists)

static int fileclose(lua_State *L)
{
    FILE **f = luaL_checkudata(L, 1, LUA_FILEHANDLE);
    int ret = *f ? fclose(*f) : -1;
    *f = NULL;
    if(!ret) {
	lua_pushboolean(L, 1);
	return 1;
    } else {
	int en = errno;
	lua_pushnil(L);
	lua_pushstring(L, strerror(en));
	lua_pushinteger(L, en);
	return 3;
    }
}

static FILE **mkfile(lua_State *L)
{
    FILE **ret = lua_newuserdata(L, sizeof(FILE *));
    *ret = NULL;
    luaL_getmetatable(L, LUA_FILEHANDLE);
    lua_setmetatable(L, -2);
    return ret;
}

/***
Create a temporary file from a pattern.
This function is a wrapper for `g_mkstemp()`.  It creates a new, unique
file by replacing the X characters in a template with six consecutive X
characters.
@function mkstemp
@tparam string tmpl The template.
@treturn nil|file Returns nil on error, and a file descriptor on success.
The file is open for reading and writing (w+b).
@treturn string Returns an error message on error, or the modified file
name.
*/
static int glib_mkstemp(lua_State *L)
{
    char *s = g_strdup(luaL_checkstring(L, 1));
    FILE **f = mkfile(L);
    gint ret = g_mkstemp(s);
    if(ret < 0) {
	lua_pop(L, 1);
	lua_pushnil(L);
	lua_pushstring(L, strerror(errno));
	g_free(s);
	return 2;
    }
    *f = fdopen(ret, "w+b");
    if(*f) {
	lua_pushstring(L, s);
	g_free(s);
	return 2;
    }
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    lua_pushinteger(L, errno);
    close(ret);
    g_remove(s);
    g_free(s);
    return 3;
}

/***
Create a temporary file in the standard temporary directory from a pattern.
This function is a wrapper for `g_file_open_tmp()`.  It creates a new,
unique file by replacing the X characters in a template with six consecutive
X characters.  The template may contain no directory separators.
@function open_tmp
@tparam string tmpl The template.
@treturn nil|file Returns nil on error, and a file descriptor on success.
The file is open for reading and writing (w+b).
@treturn string returns an error message on error, or the full path to the
newly created file.
*/
static int glib_open_tmp(lua_State *L)
{
    FILE **f = mkfile(L);
    gchar *s;
    GError *err = NULL;
    gint ret = g_file_open_tmp(luaL_checkstring(L, 1), &s, &err);

    if(ret < 0) {
	lua_pop(L, 1);
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	return 2;
    }
    *f = fdopen(ret, "w+b");
    if(*f) {
	lua_pushstring(L, s);
	g_free(s);
	return 2;
    }
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    lua_pushinteger(L, errno);
    close(ret);
    g_remove(s);
    g_free(s);
    return 3;
}

typedef struct dir_state {
    GDir *dir;
} dir_state;

static int free_dir_state(lua_State *L)
{
    get_udata(L, 1, st, dir_state);
    if(st->dir) {
	g_dir_close(st->dir);
	st->dir = NULL;
    }
    return 0;
}

static int glib_diriter(lua_State *L)
{
    get_udata(L, lua_upvalueindex(1), st, dir_state);
    if(!st->dir) {
	lua_pushnil(L);
	return 1;
    }
    lua_pushstring(L, g_dir_read_name(st->dir));
    if(lua_isnil(L, -1)) {
	g_dir_close(st->dir);
	st->dir = NULL;
    }
    return 1;
}

/***
Returns an iterator which lists entries in a directory.
This function wraps `g_dir_open()` and friends.
@function dir
@tparam string d The directory to list
@treturn nil|function Iterator or nil on error
@treturn string Error message on error
*/
static int glib_dir(lua_State *L)
{
    GError *err = NULL;
    alloc_udata(L, st, dir_state);
    st->dir = g_dir_open(luaL_checkstring(L, 1), 0, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	return 2;
    }
    lua_pushcclosure(L, glib_diriter, 1);
    return 1;
}

/* gchar *g_file_read_linke(const char *fn, GError **err) */
/* gint g_mkdir_with_parents(const gchar *fn, gint mode) */
/* gchar *g_mkdtemp(gchar *tmpl) */
/* gchar *g_dir_make_tmp(const gchar *tmpl, GError **err) */
/* gboolean g_file_set_contents(const gchar *fn, const gchar *cont, gssize len, GError **err) */


#if 0
static int glib_fs_chdir(lua_State *L) {
    gsize path_len;
    const gchar *path = luaL_checklstring(L, 1, &path_len);
    if(g_chdir(path)) {
        lua_pushnil(L);
        lua_pushfstring(L, "Could not change the working directory to %s", path);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int perm_from_char(gchar permc, int place, int wperm, int rperm) {
    int perm = 0;
    int permi = 0;

    if(permc == 'r') {
        permi = rperm;
    } else if(permc == 'w') {
        permi = wperm;
    } else if(permc == '-') {
        permi = 0;
    } else {
        permi = (int) permc - 48;
        if(permi < 0 || permi > 9) permi = 0;
    }

    if(permi > 0) {
        if((permi & 4) == 4) perm |= 4 << ((2 - place) * 3);
        if((permi & 2) == 2) perm |= 2 << ((2 - place) * 3);
        if((permi & 1) == 1) perm |= 1 << ((2 - place) * 3);
    }

    return perm;
}

static int perms_from_luaarg(lua_State *L, int pos, int wperm, int rperm) {
    gsize perm_len;
    const gchar *lua_perms;
    int perms = 0;

    switch(lua_type(L, 2)) {
        case LUA_TNIL:
        case LUA_TNONE:
            perms = 0755;
            break;
        case LUA_TSTRING:
            lua_perms = lua_tolstring(L, 2, &perm_len);
            if(perm_len == 0) {
                perms = 0755;
            } else {
                if(perm_len != 3) {
                    return -1;
                }
                int i;
                for(i=0; i<=2; i++) {
                    perms |= perm_from_char(lua_perms[i], i, wperm, rperm);
                }
            }
            break;
        default:
            return -1;
    }

    return perms;
}

static int glib_fs_mkdir(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    int perms = perms_from_luaarg(L, 2, 7, 5);
    if(perms == -1) {
        return luaL_argerror(L, 2, "Permissions must be nil or a three-character string of octal unix permissions");
    }
    
    if(g_mkdir(path, perms)) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    return 0;
}

static int glib_fs_mktree(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    int perms = perms_from_luaarg(L, 2, 7, 5);
    if(perms == -1) {
        return luaL_argerror(L, 2, "Permissions must be nil or a three-character string of octal unix permissions");
    }

    if(g_mkdir_with_parents(path, perms)) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    return 0;
}

static int glib_fs_chmod(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    int perms = -1;

    if(g_file_test(path, G_FILE_TEST_IS_DIR) || (g_file_test(path, G_FILE_TEST_EXISTS) && lua_toboolean(L, 3))) {
        perms = perms_from_luaarg(L, 2, 7, 5);
    } else if(g_file_test(path, G_FILE_TEST_EXISTS)) {
        perms = perms_from_luaarg(L, 2, 6, 4);
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "An error occurred accessing %s: file/directory does not exist", path);
        return 2;
    }
    
    if(perms == -1) {
        return luaL_argerror(L, 2, "Permissions must be nil or a three-character string of octal unix permissions");
    }

    if(g_chmod(path, perms)) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int glib_fs_chown(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);

    int uid = -1;
    int gid = -1;
    struct passwd *pw;
    struct group *gr;
    struct stat finfo;

    if(lua_type(L, 2) == LUA_TNUMBER) {
        uid = lua_tointeger(L, 2);
    } else if(lua_type(L, 2) == LUA_TSTRING) {
        pw = getpwnam(lua_tostring(L, 2));
        if(!pw) {
            lua_pushnil(L);
            lua_pushfstring(L, "User %s does not exist", lua_tostring(L, 2));
            return 2;
        }

        uid = (int) (pw->pw_uid);
    }

    if(lua_type(L, 3) == LUA_TNUMBER) {
        gid = lua_tointeger(L, 3);
    } else if(lua_type(L, 3) == LUA_TSTRING) {
        gr = getgrnam(lua_tostring(L, 3));
        if(!gr) {
            lua_pushnil(L);
            lua_pushfstring(L, "Group %s does not exist", lua_tostring(L, 3));
            return 2;
        }

        gid = (int) (gr->gr_gid);
    }

    if(!g_file_test(path, G_FILE_TEST_EXISTS)) {
        lua_pushnil(L);
        lua_pushfstring(L, "Path %s does not exist", path);
        return 2;
    }

    int rc = g_stat(path, &finfo);
    if(rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "An error occurred getting info for %s", path);
        return 2;
    }

    if(chown(path, uid == -1 ? finfo.st_uid : uid, gid == -1 ? finfo.st_gid : gid)) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

static int glib_fs_rmdir(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    if(g_rmdir(path)) {
        lua_pushnil(L);
        lua_pushfstring(L, "Unable to delete directory %s", path);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int recursive_remove(const gchar *path) {
    if(!g_file_test(path, G_FILE_TEST_EXISTS)) {
        return -1;
    }

    if(g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        return g_remove(path);
    }

    if(g_file_test(path, G_FILE_TEST_IS_DIR)) {
        GDir *dir = g_dir_open(path, 0, NULL);
        if(dir == NULL) return -1;

        const gchar *entry;
        gchar *entry_path;

        while((entry = g_dir_read_name(dir)) != NULL) {
            entry_path = g_build_filename(path, entry, NULL);
            if(recursive_remove(entry_path)) {
                g_free(entry_path);
                g_dir_close(dir);
                return -1;
            }
            g_free(entry_path);
        }

        g_dir_close(dir);
        return g_remove(path);
    }

    return 0;
}

static int glib_fs_rmtree(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    if(recursive_remove(path)) {
        lua_pushnil(L);
        lua_pushfstring(L, "Unable to delete directory tree %s", path);
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int glib_fs_join(lua_State *L) {
    int len = lua_gettop(L);
    if(!len) {
        return 0;
    }

    gchar **file_stack = (gchar **) g_malloc(sizeof(gchar *) * (len + 1));
    int i = 0;
    for(i=0; i<len; i++) {
        file_stack[i] = (gchar *) lua_tostring(L, i + 1);
    }
    file_stack[len] = NULL;

    gchar *joined = g_build_filenamev(file_stack);
    lua_pushstring(L, joined);
    g_free(joined);
    g_free(file_stack);

    return 1;
}

static int glib_fs_gfiletest(lua_State *L, GFileTest test) {
    const gchar *path = luaL_checkstring(L, 1);
    if(g_file_test(path, test) == TRUE) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

static int glib_fs_exists(lua_State *L) {
    return glib_fs_gfiletest(L, G_FILE_TEST_EXISTS);
}

static int glib_fs_isdir(lua_State *L) {
    return glib_fs_gfiletest(L, G_FILE_TEST_IS_DIR);
}

static int glib_fs_isfile(lua_State *L) {
    return glib_fs_gfiletest(L, G_FILE_TEST_IS_REGULAR);
}

static int glib_fs_listdir(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    GDir *dir = g_dir_open(path, 0, NULL);
    if(dir == NULL) {
        lua_pushnil(L);
        lua_pushfstring(L, "Error opening directory %s", path);
        return 2;
    }

    const gchar *entry;
    lua_newtable(L);

    int i = 0;
    while((entry = g_dir_read_name(dir)) != NULL) {
        lua_pushstring(L, entry);
        lua_rawseti(L, -2, ++i);
    }

    g_dir_close(dir);
    return 1;
}

static int glib_fs_glob(lua_State *L) {
    const gchar *pat = luaL_checkstring(L, 1);
    glob_t buf;
    buf.gl_offs = 0;
    int err = glob(pat, GLOB_NOSORT | GLOB_BRACE | GLOB_TILDE_CHECK, NULL, &buf);
    if(err == GLOB_NOSPACE) {
        return luaL_error(L, "out of memory");
    } else if(err == GLOB_ABORTED) {
        return luaL_error(L, "read aborted");
    } else if(err == GLOB_NOMATCH) {
        lua_newtable(L);
        return 1;
    } else if(err) {
        return luaL_error(L, strerror(errno));
    }

    lua_createtable(L, buf.gl_pathc, 0);
    int i;
    for(i=1; i<=buf.gl_pathc; i++) {
        lua_pushstring(L, buf.gl_pathv[i-1]);
        lua_rawseti(L, -2, i);
    }
    
    globfree(&buf);
    return 1;
}

static int glib_fs_walk_close(lua_State *L) {
    GQueue *queue = *(GQueue **) luaL_checkudata(L, lua_upvalueindex(1), "glib.fs.queue");
    const gchar *old_root = luaL_checkstring(L, lua_upvalueindex(3));
    int tblindex = lua_upvalueindex(2);
    lua_pushnil(L);
    while(lua_next(L, tblindex) != 0) {
        g_queue_push_tail(queue, (gpointer) g_build_filename(old_root, lua_tostring(L, -1), NULL));
        lua_pop(L, 1);
        lua_pushvalue(L, -1);
        lua_pushnil(L);
        lua_settable(L, tblindex);
    }
    
    if(g_queue_is_empty(queue)) return 0;

    gchar *root = (gchar *) g_queue_pop_head(queue);
    gchar *path;
    lua_pushstring(L, root);
    lua_replace(L, lua_upvalueindex(3));
    lua_pushstring(L, root);
    lua_pushvalue(L, tblindex);
    lua_newtable(L);

    GDir *iter = g_dir_open(root, 0, NULL);
    if(iter != NULL) {
        const gchar *entry;

        int dirsi = 0;
        int filesi = 0;
        while((entry = g_dir_read_name(iter)) != NULL) {
            path = g_build_filename(root, entry, NULL);
            lua_pushstring(L, entry);

            if(g_file_test(path, G_FILE_TEST_IS_DIR)) {
                lua_rawseti(L, tblindex, ++dirsi);
            } else {
                lua_rawseti(L, -2, ++filesi);
            }
            g_free(path);
        }

        g_dir_close(iter);
    }

    g_free(root);
    return 3;
}

static int glib_fs_walk(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    GQueue **queue = (GQueue **) lua_newuserdata(L, sizeof(GQueue *));
    luaL_getmetatable(L, "glib.fs.queue");
    lua_setmetatable(L, -2);

    *queue = g_queue_new();
    g_queue_push_tail(*queue, (gpointer) g_strdup(path));
    
    lua_newtable(L);
    lua_pushstring(L, path);
    lua_pushcclosure(L, glib_fs_walk_close, 3);
    return 1;
}

static int glib_fs_copyfile(const gchar *from, const gchar *to, gchar **errstr) {
    FILE *in, *out;
    int stat;
    char buf[LGLIB_IO_BUFSIZE];
    size_t bytes_read;
    size_t bytes_written;

    if(g_file_test(from, G_FILE_TEST_IS_REGULAR) != TRUE) {
        *errstr = g_strdup_printf("%s must be a regular file", from);
        return -1;
    }

    in = g_fopen(from, "r");
    if(in == NULL) {
        *errstr = g_strdup_printf("Unable to open file %s: %s", from, strerror(errno));
        return -1;
    }

    out = g_fopen(to, "w");
    if(out == NULL) {
        *errstr = g_strdup_printf("Unable to open file %s: %s", to, strerror(errno));
        return -1;
    }
    
    while(!feof(in)) {
        bytes_read = fread(buf, 1, LGLIB_IO_BUFSIZE, in);
        if(ferror(in)) {
            *errstr = g_strdup_printf("Unable to read from %s", from);
            return -1;
        }

        if(!bytes_read) continue;
        bytes_written = fwrite(buf, 1, bytes_read, out);

        if(!bytes_written || ferror(out)) {
            *errstr = g_strdup_printf("Unable to write to %s", to);
            return -1;
        }
    }

    fclose(in);
    fclose(out);
    /* If the original file is executable, change the permissions of the copied file to 0755 */
    if(g_file_test(from, G_FILE_TEST_IS_EXECUTABLE) == TRUE) {
        g_chmod(to, 0755);
    }
    return 0;
}

static int glib_fs_copy(lua_State *L) {
    const gchar *from = luaL_checkstring(L, 1);
    const gchar *to = luaL_checkstring(L, 2);
    gchar *err = NULL;

    if(glib_fs_copyfile(from, to, &err)) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        g_free(err);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int recursive_copy(const gchar *from, const gchar *to, gchar **errstr) {
    if(g_file_test(from, G_FILE_TEST_IS_REGULAR)) {
        return glib_fs_copyfile(from, to, errstr);
    } else if(g_file_test(from, G_FILE_TEST_IS_DIR)) {
        GDir *dir = g_dir_open(from, 0, NULL);
        if(dir == NULL) {
            *errstr = g_strdup_printf("An error occurred copying %s to %s: Could not open directory", from, to);
            return -1;
        }

        g_mkdir(to, 0755);

        const gchar *entry;
        gchar *from_entry, *to_entry;

        while((entry = g_dir_read_name(dir)) != NULL) {
            from_entry = g_build_filename(from, entry, NULL);
            to_entry = g_build_filename(to, entry, NULL);
            if(recursive_copy(from_entry, to_entry, errstr)) {
                g_free(from_entry);
                g_free(to_entry);
                g_dir_close(dir);
                return -1;
            }
            g_free(from_entry);
            g_free(to_entry);
        }

        g_dir_close(dir);
    }

    return 0;
}

static int glib_fs_copytree(lua_State *L) {
    const gchar *from = luaL_checkstring(L, 1);
    const gchar *to = luaL_checkstring(L, 2);
    gchar *err = NULL;

    if(!g_file_test(from, G_FILE_TEST_EXISTS)) {
        lua_pushnil(L);
        lua_pushfstring(L, "An error occurred copying %s: file/directory does not exist", from);
        return 2;
    }

    if(recursive_copy(from, to, &err)) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        g_free(err);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int glib_fs_dir_gc(lua_State *L) {
    GDir *dir = *(GDir **) luaL_checkudata(L, 1, "glib.fs.dir");
    if(dir) g_dir_close(dir);
    return 0;
}

static int glib_fs_queue_gc(lua_State *L) {
    GQueue *queue = *(GQueue **) luaL_checkudata(L, 1, "glib.fs.queue");
    if(queue) {
        guint len = g_queue_get_length(queue);
        if(len > 0) {
            guint i = 0;
            gpointer item;
            for(i = 0; i < len; i++) {
                item = g_queue_pop_tail(queue);
                g_free(item);
            }
        }
        g_queue_free(queue);
    }

    return 0;
}

static int glib_fs_stat(lua_State *L) {
    const gchar *path = luaL_checkstring(L, 1);
    if(!g_file_test(path, G_FILE_TEST_EXISTS)) {
        lua_pushnil(L);
        lua_pushfstring(L, "An error occurred getting info for %s: file/directory does not exist", path);
        return 2;
    }
    
    struct stat finfo;
    int rc = g_stat(path, &finfo);
    if(rc) {
        lua_pushnil(L);
        lua_pushfstring(L, "An error occurred getting info for %s", path);
        return 2;
    }

    lua_newtable(L);
    lua_pushnumber(L, (lua_Number) finfo.st_size);
    lua_setfield(L, -2, "size");

    lua_pushnumber(L, (lua_Number) finfo.st_ino);
    lua_setfield(L, -2, "inode");

    lua_pushnumber(L, (lua_Number) finfo.st_uid);
    lua_setfield(L, -2, "uid");

    lua_pushnumber(L, (lua_Number) finfo.st_gid);
    lua_setfield(L, -2, "gid");

    lua_pushnumber(L, (lua_Number) finfo.st_atime);
    lua_setfield(L, -2, "atime");

    lua_pushnumber(L, (lua_Number) finfo.st_ctime);
    lua_setfield(L, -2, "ctime");

    lua_pushnumber(L, (lua_Number) finfo.st_mtime);
    lua_setfield(L, -2, "mtime");

    const char *type = "file";
    if(S_ISLNK(finfo.st_mode)) {
        type = "symlink";
    } else if(S_ISREG(finfo.st_mode)) {
        type = "file";
    } else if(S_ISDIR(finfo.st_mode)) {
        type = "directory";
    }
    lua_pushstring(L, type);
    lua_setfield(L, -2, "type");

    lua_pushnumber(L, (lua_Number) finfo.st_mode);
    lua_setfield(L, -2, "mode");

    char perms[4] = "---";
    if(finfo.st_mode & S_IWUSR) {
        perms[0] = 'w';
    } else if(finfo.st_mode & S_IRUSR) {
        perms[0] = 'r';
    }

    if(finfo.st_mode & S_IWGRP) {
        perms[1] = 'w';
    } else if(finfo.st_mode & S_IRGRP) {
        perms[1] = 'r';
    }

    if(finfo.st_mode & S_IWOTH) {
        perms[2] = 'w';
    } else if(finfo.st_mode & S_IROTH) {
        perms[2] = 'r';
    }
    lua_pushstring(L, (const char *) perms);
    lua_setfield(L, -2, "perms");

    return 1;
}

static const struct luaL_Reg glib_fs_reg [] = {
    {"getcwd", glib_fs_getcwd},
    {"chdir", glib_fs_chdir},
    {"gettmpdir", glib_fs_gettmpdir},
    {"gethomedir", glib_fs_gethomedir},
    {"getenvdir", glib_fs_getenvdir},
    {"mkdir", glib_fs_mkdir},
    {"mktree", glib_fs_mktree},
    {"rmdir", glib_fs_rmdir},
    {"rmtree", glib_fs_rmtree},
    {"copy", glib_fs_copy},
    {"copytree", glib_fs_copytree},
    {"join", glib_fs_join},
    {"exists", glib_fs_exists},
    {"isdir", glib_fs_isdir},
    {"isfile", glib_fs_isfile},
    {"listdir", glib_fs_listdir},
    {"iterdir", glib_fs_iterdir},
    {"glob", glib_fs_glob},
    {"walk", glib_fs_walk},
    {"dirname", glib_fs_dirname},
    {"basename", glib_fs_basename},
    {"stat", glib_fs_stat},
    {"chown", glib_fs_chown},
    {"chmod", glib_fs_chmod},
    {NULL, NULL}
};

int luaopen_glib_fs(lua_State *L) {
    luaL_newmetatable(L, "glib.fs.dir");
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, glib_fs_dir_gc);
    lua_settable(L, -3);

    luaL_newmetatable(L, "glib.fs.queue");
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, glib_fs_queue_gc);
    lua_settable(L, -3);

    luaL_register(L, "glib.fs", glib_fs_reg);

    lua_getglobal(L, "os");
    lua_pushliteral(L, "remove");
    lua_getfield(L, -2, "remove");
    lua_settable(L, -4);

    lua_pushliteral(L, "move");
    lua_getfield(L, -2, "rename");
    lua_settable(L, -4);
    lua_pop(L, 1);
    return 1;
}

#endif

#define fent(n) {#n, glib_##n}
static luaL_Reg lua_funcs[] = {
    /* no support for Atomic Operations */
    /* no support for The Main Event Loop */
    /* no support for Threads/Thread Pools/Asynchronous Queues */
    /* no support for Dynamic Loading of Modules (use lua) */
    /* no support for Memory Allocation/Memory Slices */
    /* no support for IO Channels */
    /* no direct support for Error Reporting/Message Output and Debugging Functions */
    /* Message Logging */
    fent(log),
    /* no support for replacing handler or setting fatality (yet) */
    /* no support for String Utility Functions */
    /* Character Set Conversion */
    fent(convert),
    /* Unicode Manipulation */
    fent(validate),
    fent(isalpha),
    fent(iscntrl),
    fent(isdefined),
    fent(isdigit),
    fent(isgraph),
    fent(islower),
    fent(ismark),
    fent(isprint),
    fent(ispunct),
    fent(isspace),
    fent(istitle),
    fent(isupper),
    fent(isxdigit),
    fent(iswide),
    fent(iswide_cjk),
    fent(iszerowidth),
    fent(toupper),
    fent(tolower),
    fent(totitle),
    fent(digit_value),
    fent(xdigit_value),
    /* compose, decompose, fully_decompose are too low-level */
    /* use g_utf8_normalize() instead */
    /* combining class is questionable, so don't support that either */
    /* canonical_ordering does it in-place; again, just use normalize */
    fent(type),
    fent(break_type),
    fent(get_mirror_char),
    /* if people really need script, they can deal with the ISO code */
    fent(get_script),
    /* the proper way to support the rest would be to make unicode strings */
    /* a first-class type.  Not happening here, though */
    /* instead, most are just dropped */
    fent(utf8_sub),
    fent(utf8_len),
    fent(utf8_validate),
    fent(utf8_strup),
    fent(utf8_strdown),
    fent(utf8_casefold),
    fent(utf8_normalize),
    fent(utf8_collate),
    fent(utf8_collate_key),
    fent(utf8_collate_key_for_filename),
    fent(utf8_to_utf16),
    fent(utf8_to_ucs4),
    fent(utf16_to_ucs4),
    fent(utf16_to_utf8),
    fent(ucs4_to_utf16),
    fent(ucs4_to_utf8),
    fent(to_utf8),
    /* Base64 Encoding */
    fent(base64_encode),
    fent(base64_decode),
    /* Data Checksums */
    fent(md5sum),
    fent(sha1sum),
    fent(sha256sum),
    /* Secure HMAC Digests */
    fent(md5hmac),
    fent(sha1hmac),
    fent(sha256hmac),
    /* Internationalization */
    /* macros are in global */
    /* local helper functions are not supported */
    fent(get_locale_variants),
    /* Date and Time Functions */
    fent(sleep),
    fent(usleep),
    /* no support for other functions; use os.date()/os.time() instead */
    /* or pure-lua packages for date/time management */
    /* get_monotinic_time seems useful, but it's not really monotonic */
    /* no support for GTimeZone */
    /* no support for GDateTime */
    /* Random Numbers */
    fent(random),
    /* random_set_seed() not supported; use rand_new() instead */
    /* in fact, you can just: glib.random = glib.rand_new(seed) */
    fent(rand_new),
    /* no support for Hook Functions (better to implement in pure lua) */
    /* for i, f in ipairs(hooks) do f() end */
    /* Miscellaneous Utility Functions */
    fent(application_name),
    fent(prgname),
    fent(getenv),
    fent(setenv),
    fent(unsetenv),
    fent(listenv),
    fent(get_user_name),
    fent(get_real_name),
    fent(get_dir_name),
    fent(get_host_name),
    fent(get_current_dir),
    fent(path_is_absolute),
    fent(path_split_root),
    fent(path_get_basename),
    fent(path_get_dirname),
    fent(build_filename),
    fent(build_path),
    fent(format_size),
    fent(find_program_in_path),
    /* bit ops not worth supporting; better to add them to lua-bit */
    /* g_spaced_primes_closest seems too internal-use */
    /* atext is unsafe/deprecated */
    /* g_parse_debug_string is also somewhat internal-use */
    fent(qsort),
    fent(cmp),
    /* Lexical Scanner is not supported */
    /* it is fairly unconfigurable and hard to bind to Lua */
    /* just use a made-for-lua scanner like lpeg anyway */
    /* Timers */
    fent(timer_new),
    /* Spawning Processes */
    fent(spawn),
    /* File Utilities */
    fent(is_dir),
    fent(is_file),
    fent(is_symlink),
    fent(is_exec),
    fent(exists),
    fent(mkstemp),
    fent(open_tmp),
    fent(dir),
    {NULL, NULL}
};

int luaopen_glib(lua_State *L)
{
    luaL_register(L, "glib", lua_funcs);

/* there does not appear to be a way to move this up to the top in ldoc */
/***
Version Information.
@section Version Information
*/

/***
GLib version string.
@table version
Version of running glib (not the one it was compiled against)
*/
    {
	char ver[80];
	snprintf(ver, 80, "%u.%u.%u", glib_major_version, glib_minor_version,
		 glib_micro_version);
	lua_pushstring(L, ver);
	lua_setfield(L, -2, "version");
    }
/***
Standard Macros.
@section Standard Macros
*/
/***
Operating system.
@table os
A string representing the operating system: 'win32', 'beos', 'unix',
 'unknown'
*/
    {
#ifdef G_OS_WIN32
	lua_pushliteral(L, "win32");
#else
#ifdef G_OS_BEOS
	lua_pushliteral(L, "beos");
#else
#ifdef G_OS_UNIX
	lua_pushliteral(L, "unix");
#else
	lua_pushliteral(L, "unknown");
#endif
#endif
#endif
	lua_setfield(L, -2, "os");
    }
/***
Directory separator.
@table dir_separator
Unlike GLib's directory separator, this includes both valid values under
Win32.
*/
    {
	char sep[3];

	sep[0] = G_DIR_SEPARATOR;
	if(G_DIR_SEPARATOR != '/' && G_IS_DIR_SEPARATOR('/')) {
	    sep[1] = '/';
	    sep[2] = 0;
	} else
	    sep[1] = 0;
	lua_pushstring(L, sep);
	lua_setfield(L, -2, "dir_separator");
    }
/***
Path list separator
@table searchpath_separator
*/
    lua_pushstring(L, G_SEARCHPATH_SEPARATOR_S);
    lua_setfield(L, -2, "searchpath_separator");
    /* no support for other standard macros */
    /* no support for Type Conversion Macros */
    /* no support for Byte Order Macros */
    /* no support for Numerical Definitions */
    /* no support for Miscellaneous Macros */
    /* see lua_funcs[] for rest */

    /* data types */
#define newt(t) do { \
    luaL_newmetatable(L, "glib."#t); \
    lua_pop(L, 1); \
} while(0)
#define newt_free(t) do { \
    luaL_newmetatable(L, "glib."#t); \
    lua_pushcfunction(L, free_##t); \
    lua_setfield(L, -2, "__gc"); \
    lua_pop(L, 1); \
} while(0)
#define newt_tab(t) do { \
    luaL_newmetatable(L, "glib."#t); \
    luaL_register(L, NULL, t##_funcs); \
    lua_pushvalue(L, -1); \
    lua_setfield(L, -1, "__index"); \
    lua_pop(L, 1); \
} while(0)
    newt_free(convert_state);
    newt(base64_state);
    newt_free(sumstate);
    newt_free(hmacstate);
    newt_free(rand_state);
    newt_tab(timer_state);
    newt_tab(spawn_state);
    newt_free(dir_state);

    /* Internationalization */
    /* gettext macros are globals */
    lua_register(L, "_", glib_gettext);
    lua_register(L, "Q_", glib_dpgettext0);
    lua_register(L, "C_", glib_dpgettext4);
    lua_register(L, "N_", glib_ngettext);
    lua_register(L, "NC_", glib_ndpgettext4);

    /* lua does special magic to allow pclose/fclose to be the same */
    /* create special fenv */
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, fileclose);
    lua_setfield(L, -2, "__close");
    /* set for mkstemp */
    lua_getfield(L, -2, "mkstemp");
    lua_pushvalue(L, -2);
    lua_setfenv(L, -2);
    lua_pop(L, 1);
    /* set for open_tmp */
    lua_getfield(L, -2, "open_tmp");
    lua_pushvalue(L, -2);
    lua_setfenv(L, -2);
    lua_pop(L, 2);
    return 1;
}

#if 0
static int glib_fs_basename(lua_State *L) {
    gchar *base;
    base = g_path_get_basename(luaL_checkstring(L, 1));
    if(strcmp(base, ".") == 0 || strcmp(base, "/") == 0 || strcmp(base, "\\") == 0) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, base);
    }
    free(base);
    return 1;
}

#endif
