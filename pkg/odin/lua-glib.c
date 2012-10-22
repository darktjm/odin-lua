/***
This package wraps the GLib library fairly thinly.
Unlike other lua-glib implementations, this is not meant to be a
stepping stone for GTK+ support.  It is meant to be a portability
and utility library.  There are other projects which provide
support for GTK+ and other various derivatives of GLib.  The specific
version against this was developed was 2.32.4; I have not fully scanned
the documentation to determine what functions should be guarded using
the GLib version, so it may require a very recent (but not more)
version of GLib.

This package was originally part of another proect with a weird
build system, so no build support is provided in this standalone
version, at least for now.  Just compile with appropriate flags
to create a shared library linked against the Lua and GLib libraries.
This documentation was built as follows:

     ldoc.lua -p lua-glib -o lua-glib -d /tmp -f markdown -t 'Lua GLib Library' lua-glib.c

No support is provided for Type Conversion Macros, Byte Order Macros,
Numerical Definitions, or Miscellaneous Macros, as they are all
either too low-level or C-specific.
No support is provided for Atomic Operations, Threads, Thread Pools, or
Asynchronous queues; use a Lua-specific threading module instead
(such as Lanes).  No support is provided for The Main Event Loop, as
its utility versus implementation effort seems low.  No support is
provided for Dynamic Loading of Modules, because lua already does
that.  No support is provided for Memory Allocation or Memory Slices,
since you shouldn't be doing that in Lua.  IO Channels also seem
below my minimum utility-to-effort ratio.  There is no direct support
for the Error Reporting, Message Output, and Debugging Functions.
There is no support for replacing the log handlers or setting the
fatality flags in log messages, although that may come some day.
The String Utility Functions are C-specific, and not supported.
The Hook Functions seem useless for Lua.  The Lexical Scanner is
not very configurable and hard to bind to Lua; use a made-for-Lua
scanner instead like Lpeg.  No support is provided for the Commandline
option parser, as adapting it to Lua would remove most of its convenience
and there are plenty of other similar libraries for lua.  The Glob-style
pattern matching is too simplistic and can easily be emulated using
regex-style patterns.  The UNIX-specific and Windows-specific functions
are not supported, since such blatantly non-portable functions should
not be used.  The Testing framework is not supported, because a
Lua-specific framework would be more suited to the task.  None of the
GLib Data Types are supported, although there may be merit in eventually
supporting the GVariant type somewhat.

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
File Utilities (except a few MS library compatibility functions and
memory mapped files), URI Functions, Hostname Utilities, Shell-related
Utilities, Perl-compatible regular expressions, Simle XML Subset Parser
(without vararg functions since building varargs generically is not possible),
Key-value file parser, and Bookmark file parser.

Eventually, this may be split into multiple modules, so that uselses stuff
can be removed.  However, removing useless stuff from GLib is not easy,
so having Lua bindings for what's there is not necessarily harmful.
@module glib
*/

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
#include <fcntl.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifdef G_OS_UNIX
#include <pwd.h>
#include <grp.h>
#endif
/* note: Windows sticks this in sys/utime.h for some reason */
#include <utime.h>

/* this is the only exported function: */
int luaopen_glib(lua_State *L);

#ifdef _MSC_VER
#include <io.h>
/* MS VS 2005 deprecatesregular versions in favor of _ versions */
#if _MSC_VER >= 1400
#undef dup
#define dup _dup
#undef dup2
#define dup2 _dup2
#undef write
#define write _write
#undef read
#define read _read
#undef close
#define close _close
#undef umask
#define umask win_umask
static int win_umask(int m)
{
    _umask_s(m, &m);
    return m;
}
#endif
#else
#include <unistd.h>
#endif

/* 5.1/5.2 compatibility */
#if LUA_VERSION_NUM <= 501
#define lua_len lua_objlen
#define luaL_setfuncs(L, f, n) luaL_register(L, NULL, f)
#define lua_setuservalue(L, n) lua_setfenv(L, n)
#define lua_getuservalue(L, n) lua_getfenv(L, n)
#else
#define lua_lessthan(L, a, b) lua_compare(L, a, b, LUA_OPLT)
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

/* there does not appear to be a way to move this up to the top in ldoc */
/* this documents variables defined below */
/*********************************************************************/
/***
Version Information.
@section Version Information
*/

/***
GLib version string.
@table version
Version of running glib (not the one it was compiled against)
*/

/*********************************************************************/
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

/***
Directory separator.
@table dir_separator
Unlike GLib's directory separator, this includes both valid values under
Win32.
*/

/***
Path list separator
@table searchpath_separator
*/

/*********************************************************************/
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
Log a message, GLib-style.
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

/*********************************************************************/
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
	g_error_free(err);
	return 2;
    } else {
	lua_pushlstring(L, ret, retlen);
	g_free(ret);
	return 1;
    }
}

/*********************************************************************/
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
	g_error_free(err); \
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

/*********************************************************************/
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

/*********************************************************************/
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

/*********************************************************************/
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

/*********************************************************************/
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

/*********************************************************************/
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

/*********************************************************************/
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
	size_t len = lua_len(L, 1);
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

/*********************************************************************/
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
	size_t len = lua_len(L, skip);
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
    nind = lua_len(L, 1);
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

/*********************************************************************/
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

static luaL_Reg timer_state_funcs[] = {
    {"start", timer_start},
    {"stop", timer_stop},
    {"continue", timer_continue},
    {"elapsed", timer_elapsed},
    {"__gc", free_timer_state},
    {NULL, NULL}
};

/*********************************************************************/
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
	nargs = lua_len(L, 1);
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
#if LUA_VERSION_NUM <= 501
	    FILE **inf = luaL_checkudata(L, -1, LUA_FILEHANDLE);
#else
	    luaL_Stream *str = luaL_checkudata(L, -1, LUA_FILEHANDLE);
	    FILE **inf = &str->f;
#endif
	    if(*inf)
		newin = fileno(*inf);
	    /* else inf == /dev/null */
	} else if(lua_isstring(L, -1)) {
	    const char *fn = lua_tostring(L, -1);
	    if(*fn) {
		gboolean isb = *fn == '!';
		inf = fopen(isb ? fn + 1 : fn, isb ? "rb" : "r");
		if(!inf) {
		    int en = errno;

		    g_strfreev(env);
		    lua_pushnil(L);
		    lua_pushliteral(L, "Can't open stdin ");
		    lua_pushvalue(L, -3);
		    lua_pushliteral(L, ": ");
		    lua_pushstring(L, strerror(en));
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
#if LUA_VERSION_NUM <= 501
	    FILE **outf = luaL_checkudata(L, -1, LUA_FILEHANDLE);
#else
	    luaL_Stream *str = luaL_checkudata(L, -1, LUA_FILEHANDLE);
	    FILE **outf = &str->f;
#endif
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
		    int en = errno;

		    g_strfreev(env);
		    if(inf)
			fclose(inf);
		    lua_pushnil(L);
		    lua_pushliteral(L, "Can't open stdout ");
		    lua_pushvalue(L, -3);
		    lua_pushliteral(L, ": ");
		    lua_pushstring(L, strerror(en));
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
#if LUA_VERSION_NUM <= 501
	    FILE **errf = luaL_checkudata(L, -1, LUA_FILEHANDLE);
#else
	    luaL_Stream *str = luaL_checkudata(L, -1, LUA_FILEHANDLE);
	    FILE **errf = &str->f;
#endif	    
	    if(*errf)
		newerr = fileno(*errf);
	    /* else errf == /dev/null */
	} else if(lua_isstring(L, -1)) {
	    const char *fn = lua_tostring(L, -1);
	    if(*fn) {
		gboolean isb = *fn == '!';
		errf = fopen(isb ? fn + 1 : fn, isb ? "ab" : "a");
		if(!errf) {
		    int en = errno;

		    g_strfreev(env);
		    if(inf)
			fclose(inf);
		    if(outf)
			fclose(outf);
		    lua_pushnil(L);
		    lua_pushliteral(L, "Can't open stderr ");
		    lua_pushvalue(L, -3);
		    lua_pushliteral(L, ": ");
		    lua_pushstring(L, strerror(en));
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
	    g_error_free(err);
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

static luaL_Reg spawn_state_funcs[] = {
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

/*********************************************************************/
/***
File Utilities
@section File Utilities
*/

/***
Return contents of a file as a string.
This function is a wrapper for `g_file_get_contents()`.
It is mostly equivalent to io.open(*name*):read('\*a').
@function file_get
@tparam string name Name of file to read
@treturn string|nil Contents of file, or nil if there was an error.
@treturn string Error message if there was an error.
*/
static int glib_file_get(lua_State *L)
{
    const char *f = luaL_checkstring(L, 1);
    gsize len;
    gchar *cont;
    GError *err = NULL;
    if(g_file_get_contents(f, &cont, &len, &err)) {
	lua_pushlstring(L, cont, len);
	g_free(cont);
	return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, err->message);
    g_error_free(err);
    return 2;
}

/***
Set contents of file to a string.
This function is a wrapper for `g_file_set_contents()`.  Rather than
write directly to a file, it writes to a temporary first and then moves
the result into place.
@function file_set
@tparam string name Name of file to write
@tparam string contents Contents to write
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int glib_file_set(lua_State *L)
{
    size_t len;
    const char *f = luaL_checkstring(L, 1);
    const char *cont = luaL_checklstring(L, 2, &len);
    GError *err = NULL;
    gboolean ok = g_file_set_contents(f, cont, len, &err);
    lua_pushboolean(L, ok);
    if(!ok) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

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
#if LUA_VERSION_NUM <= 501
    FILE **f = luaL_checkudata(L, 1, LUA_FILEHANDLE);
#else
    luaL_Stream *str = luaL_checkudata(L, -1, LUA_FILEHANDLE);
    FILE **f = &str->f;
#endif
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
#if LUA_VERSION_NUM <= 501
    FILE **ret = lua_newuserdata(L, sizeof(FILE *));
#else
    luaL_Stream *str = lua_newuserdata(L, sizeof(luaL_Stream));
    FILE **ret = &str->f;
    str->closef = fileclose;
#endif
    *ret = NULL;
    luaL_getmetatable(L, LUA_FILEHANDLE);
    lua_setmetatable(L, -2);
    return ret;
}

static int getmode(lua_State *L, int arg, int def, int isdir)
{
    if(lua_isnumber(L, arg))
	return lua_tointeger(L, arg) & 07777;
    else if(lua_isstring(L, arg)) {
	const char *s = lua_tostring(L, arg);
	while(*s) {
	    int mask = 07777; /* all */
	    int nowho = 1;
	    char op = '=';
	    int what = 0;
	    /* parse "who" */
	    while(*s && *s != ',') {
		switch(*s) {
		  case 'u':
		    if(nowho)
			mask = 01000;
		    mask |= 04700;
		    nowho = 0;
		    s++;
		    continue;
		  case 'g':
		    if(nowho)
			mask = 01000;
		    mask |= 02070;
		    nowho = 0;
		    s++;
		    continue;
		  case 'o':
		    if(nowho)
			mask = 01000;
		    mask |= 0007;
		    nowho = 0;
		    s++;
		    continue;
		  case 'a':
		    mask = 07777;
		    nowho = 0;
		    s++;
		    continue;
		}
		break;
	    }
	    /* parse "op" */
	    if(*s == '-' || *s == '=' || *s == '+')
		op = *s++;
	    else
		/* silently ignore errors */
		return def;
	    if(*s == 'u') {
		what = def & 0700;
		what |= (what >> 3) | (what >> 6);
		what |= (def & 04000) | ((def & 04000) >> 1);
	    } else if(*s == 'g') {
		what = def & 0070;
		what |= (what << 3) | (what >> 3);
		what |= (def & 02000) | ((def & 02000) << 1);
	    } else if(*s == 'o') {
		what = def & 0007;
		what |= (what << 6) | (what << 3);
	    } else {
		while(*s && *s != ',') {
		    switch(*s) {
		      case 'r':
			what |= 0444;
			++s;
			continue;
		      case 'w':
			what |= 0222;
			++s;
			continue;
		      case 'x':
			what |= 0111;
			++s;
			continue;
		      case 'X':
			if(isdir || (def & 0111))
			    what |= 0111;
			++s;
			continue;
		      case 's':
			what |= 06000;
			++s;
			continue;
		      case 't':
			what |= 01000;
			++s;
			continue;
		      default:
			/* silently ignore errors */
			return def;
		    }
		}
	    }
	    if(op == '+')
		def |= what & mask;
	    else if(op == '-')
		def &= ~(what & mask);
	    else
		def = (def & ~mask) | (what & mask);
	    if(*s == ',')
		++s;
	}
	return def;
    } else
	return def;
}

/***
Change default file and directory creation permissions mask.
This is a wrapper for the system `umask()` function, since there is no
equivalent in GLib.
@function umask
@tparam string|number mask (Optional) The permissions mask.  Permissions set in
 this mask are forced off in any newly created files and directories.
 Either a numeric permissions mask or a POSIX-sytle mode string (e.g.
 'og=w', which is the default if this parameter is unspecified).
@treturn number The previous mask.
*/
static int glib_umask(lua_State *L)
{
    lua_pushnumber(L, umask(getmode(L, 1, 0022, 0)));
    return 1;
}

/***
Create a unique temporary file from a pattern.
This function is a wrapper for `g_mkstemp()`.  It creates a new, unique
file by replacing the X characters in a template with six consecutive X
characters.
@function mkstemp
@tparam string tmpl The template.
@tparam string|number perm (Optional) File creation permissions.  Either a
 numeric permission mask or a POSIX-style mode string (e.g. 'ug=rw').
@treturn nil|file Returns nil on error, and a file descriptor on success.
The file is open for reading and writing (w+b).
@treturn string Returns an error message on error, or the modified file
name.
*/
static int glib_mkstemp(lua_State *L)
{
    int mode = getmode(L, 2, 0544, 0);
    int gotmode = lua_isnoneornil(L, 2);
    FILE **f = mkfile(L);
    char *s = g_strdup(luaL_checkstring(L, 1));
    gint ret;
    if(gotmode)
	ret = g_mkstemp(s);
    else
	ret = g_mkstemp_full(s, O_RDWR | O_BINARY, mode);
    if(ret < 0) {
	int en = errno;

	lua_pop(L, 1);
	lua_pushnil(L);
	lua_pushstring(L, strerror(en));
	g_free(s);
	return 2;
    }
    *f = fdopen(ret, "w+b");
    if(*f) {
	lua_pushstring(L, s);
	g_free(s);
	return 2;
    }
    {
	int en = errno;

	lua_pushnil(L);
	lua_pushstring(L, strerror(en));
	lua_pushinteger(L, en);
    }
    close(ret);
    g_remove(s);
    g_free(s);
    return 3;
}

/***
Create a unique temporary file in the standard temporary directory.
This function is a wrapper for `g_file_open_tmp()`.  It creates a new,
unique file by replacing the X characters in a template with six consecutive
X characters.  The template may contain no directory separators.
@function open_tmp
@tparam string tmpl (Optional) The template.  If unspecified, a default
 template will be used.
@treturn nil|file Returns nil on error, and a file descriptor on success.
The file is open for reading and writing (w+b).
@treturn string returns an error message on error, or the full path to the
newly created file.
*/
static int glib_open_tmp(lua_State *L)
{
    const char *t = lua_isnoneornil(L, 1) ? NULL : luaL_checkstring(L, 1);
    FILE **f = mkfile(L);
    gchar *s;
    GError *err = NULL;
    gint ret = g_file_open_tmp(t, &s, &err);

    if(ret < 0) {
	lua_pop(L, 1);
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    *f = fdopen(ret, "w+b");
    if(*f) {
	lua_pushstring(L, s);
	g_free(s);
	return 2;
    }
    {
	int en = errno;

	lua_pushnil(L);
	lua_pushstring(L, strerror(en));
	lua_pushinteger(L, en);
    }
    close(ret);
    g_remove(s);
    g_free(s);
    return 3;
}

/***
Read the contents of a soft link.
This is a wrapper for `g_file_read_link()`
@function read_link
@tparam string name Link to read
@treturn string|nil Link contents, or nil on error
@treturn string Error message on error
*/
static int glib_read_link(lua_State *L)
{
    const char *f = luaL_checkstring(L, 1);
    GError *err = NULL;
    char *ret = g_file_read_link(f, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Create a directory and any required parent directories.
This is a wrapper for `g_mkdir_with_parents()`
@function mkdir_with_parents
@tparam string name Name of directory to create.
@tparam string|number mode (Optional) File permissions.  Either a numeric
 creation mode or a POSIX-style mode string (e.g. 'ug=rw').  If unspecified,
 'a=rx,u=w' is used (octal 755).
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int glib_mkdir_with_parents(lua_State *L)
{
    const char *n = luaL_checkstring(L, 1);
    int mode = getmode(L, 2, 0755, 1);
    int ret, en;

    ret = g_mkdir_with_parents(n, mode);
    en = errno;
    lua_pushboolean(L, !ret);
    if(ret) {
	lua_pushstring(L, strerror(en));
	return 2;
    }
    return 1;
}

/***
Create a unique tempoarary directory from a pattern.
This is a wrapper for `g_mkdtemp`.  It replaces 6 consecutive Xs in the
pattern with a unique string and creates a directory by that name.
@function mkdtemp
@tparam string tmpl The file name pattern
@tparam string|number mode (Optional) File permissions.  Either a numeric
 creation mode or a POSIX-style mode string (e.g. 'ug=rw').
@treturn string|nil The name of the created directory, or nil on error.
@treturn string The error message if an error occured.
*/
static int glib_mkdtemp(lua_State *L)
{
    const char *t = luaL_checkstring(L, 1);
    int mode = getmode(L, 2, 0755, 1);
    char *ret = g_strdup(t);
    int en;

    if(lua_isnoneornil(L, 2))
	ret = g_mkdtemp(ret);
    else
	ret = g_mkdtemp_full(ret, mode);
    en = errno;
    lua_pushstring(L, ret);
    if(!ret)
	lua_pushstring(L, strerror(en));
    return ret ? 1 : 2;
}

/***
Create a unique temporary directory in the standard temporary directory.
This is a wrapper for `g_dir_make_tmp()`.  It creates a new, unique
directory in the standard temporary directory by replacing 6 consecutive
Xs in the template.
@function dir_make_tmp
@tparam string tmpl (Optional) The template.  If unspecified, a default
 template will be used.
@treturn string|nil The name of the created directory, or nil on error.
@treturn string An error message on error.
*/
static int glib_dir_make_tmp(lua_State *L)
{
    const char *t = lua_isnoneornil(L, 1) ? NULL : luaL_checkstring(L, 1);
    GError *err = NULL;
    char *ret = g_dir_make_tmp(t, &err);

    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
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
	g_error_free(err);
	return 2;
    }
    lua_pushcclosure(L, glib_diriter, 1);
    return 1;
}

/***
Rename a file sytem entity.
This is a wrapper for `g_rename()`
@function rename
@tparam string old The old name
@tparam string new The new name
@treturn boolean ok True on success.
@treturn string msg Error message on failure.
*/
static int glib_rename(lua_State *L)
{
    int ret = g_rename(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
    if(ret < 0) {
	int en = errno;
	lua_pushboolean(L, 0);
	lua_pushstring(L, strerror(en));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/***
Create a directory.
This is a wrapper for `g_mkdir()`
@function mkdir
@tparam string name The name of the directory.
@tparam string|number mode (Optional) File permissions.  Either a numeric
 creation mode or a POSIX-style mode string (e.g. 'ug=rw').  The default
 is 'a=rx,u=w' (octal 755).
*/
static int glib_mkdir(lua_State *L)
{
    int mode = getmode(L, 2, 0755, 1);
    int ret = g_mkdir(luaL_checkstring(L, 1), mode);

    if(ret < 0) {
	int en = errno;
	lua_pushboolean(L, 0);
	lua_pushstring(L, strerror(en));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static void push_type(lua_State *L, int mode)
{
    switch(mode & S_IFMT) {
      case S_IFREG:
	lua_pushliteral(L, "file");
	break;
      case S_IFDIR:
	lua_pushliteral(L, "dir");
	break;
      case S_IFIFO:
	lua_pushliteral(L, "pipe");
	break;
      case S_IFSOCK:
	lua_pushliteral(L, "socket");
	break;
      case S_IFLNK:
	lua_pushliteral(L, "link");
	break;
      case S_IFBLK:
	lua_pushliteral(L, "block");
	break;
      case S_IFCHR:
	lua_pushliteral(L, "char");
	break;
      default:
	lua_pushnil(L);
	break;
    }
}

static void push_owner(lua_State *L, int uid)
{
    struct passwd *pw = getpwuid(uid);
    if(pw)
	lua_pushstring(L, pw->pw_name);
    else
	lua_pushnumber(L, uid);
}

static void push_group(lua_State *L, int gid)
{
    struct group *gr = getgrgid(gid);
    if(gr)
	lua_pushstring(L, gr->gr_name);
    else
	lua_pushnumber(L, gid);
}

static void push_mode(lua_State *L, int mode)
{
    char buf[10];
    switch(mode & S_IFMT) {
      case S_IFDIR:
	buf[0] = 'd';
	break;
      case S_IFIFO:
	buf[0] = 'p';
	break;
      case S_IFSOCK:
	buf[0] = 's';
	break;
      case S_IFLNK:
	buf[0] = 'l';
	break;
      case S_IFBLK:
	buf[0] = 'b';
	break;
      case S_IFCHR:
	buf[0] = 'c';
	break;
      case S_IFREG:
      default:
	buf[0] = '-';
	break;
    }
    buf[1] = (mode & 0400) ? 'r' : '-';
    buf[2] = (mode & 0200) ? 'w' : '-';
    if(mode & 04000)
	buf[3] = (mode & 0100) ? 's' : 'S';
    else
	buf[3] = (mode & 0100) ? 'x' : '-';
    buf[4] = (mode & 0040) ? 'r' : '-';
    buf[5] = (mode & 0020) ? 'w' : '-';
    if(mode & 02000)
	buf[6] = (mode & 0010) ? 's' : 'S';
    else
	buf[6] = (mode & 0010) ? 'x' : '-';
    buf[7] = (mode & 0004) ? 'r' : '-';
    buf[8] = (mode & 0002) ? 'w' : '-';
    if(mode & 01000)
	buf[9] = (mode & 0001) ? 't' : 'T';
    else
	buf[9] = (mode & 0001) ? 'x' : '-';
    lua_pushlstring(L, buf, 10);
}

/***
Retrieve information on a file system entry.
This is a wrapper for `g_stat()`.  If a table is given to specify fields,
only those fields are returned (nil if not present) as multiple results.
Otherwise, a table is returned with all known fields and their values.
If the field named 'link' is requested, it is not an actual field, but an
indicator to return information about a soft link rather than what it
points to.
@function stat
@tparam string name The file system entry to query
@tparam {string,...}|string,... fields (Optional) The fields to query, in
 order.
@treturn table A table whose keys are field names and whose values are
 the value for that field, if either no specific values are requested or
 the only value requested is the psuedo-value 'link'.
@treturn number|string|nil The value of each field; multiple values may be
 returned.
@treturn nil First returned value if there was an error
@treturn string Second returned value on error: an error message.
*/
/* FIXME: support nanosecond time stamps (no portable way to detect) */
static int glib_stat(lua_State *L)
{
    const char *n = luaL_checkstring(L, 1);
    gboolean is_link = FALSE;
    int nret = 0, actret = 0;
    GStatBuf sbuf;
    int pstart;
    int ok;

    if(lua_istable(L, 2)) {
	int i;
	nret = lua_len(L, 2);
	pstart = lua_gettop(L) + 1;
	lua_checkstack(L, nret);
	for(i = 1; i <= nret; i++) {
	    lua_pushinteger(L, i);
	    lua_gettable(L, 2);
	    if(!strcasecmp(luaL_checkstring(L, -1), "link")) {
		lua_pop(L, 1);
		is_link = TRUE;
	    } else
		actret++;
	}
	nret = actret;
    } else {
	int i;
	pstart = 2;
	nret = lua_gettop(L) - 1;
	for(i = 0; i < nret; i++) {
	    const char *s = luaL_checkstring(L, 2 + i);
	    if(!strcasecmp(s, "link"))
		is_link = TRUE;
	    else
		++actret;
	}
    }
    if(is_link)
	ok = g_lstat(n, &sbuf) == 0;
    else
	ok = g_stat(n, &sbuf) == 0;
    if(!ok) {
	int en = errno;
	lua_pushnil(L);
	lua_pushstring(L, strerror(en));
	return 2;
    }
    if(!actret) {
	lua_createtable(L, 0, 17);
	lua_pushnumber(L, sbuf.st_atime);
	lua_setfield(L, -2, "atime");
	lua_pushnumber(L, sbuf.st_ctime);
	lua_setfield(L, -2, "ctime");
	lua_pushnumber(L, sbuf.st_dev);
	lua_setfield(L, -2, "dev");
	lua_pushnumber(L, sbuf.st_ino);
	lua_setfield(L, -2, "ino");
	push_mode(L, sbuf.st_mode);
	lua_setfield(L, -2, "mode");
	push_type(L, sbuf.st_mode);
	lua_setfield(L, -2, "type");
	lua_pushnumber(L, sbuf.st_mode & 0x7777);
	lua_setfield(L, -2, "perm");
	lua_pushnumber(L, sbuf.st_mtime);
	lua_setfield(L, -2, "mtime");
	lua_pushnumber(L, sbuf.st_nlink);
	lua_setfield(L, -2, "nlink");
	lua_pushnumber(L, sbuf.st_rdev);
	lua_setfield(L, -2, "rdev");
	lua_pushnumber(L, sbuf.st_size);
	lua_setfield(L, -2, "size");
	lua_pushnumber(L, sbuf.st_uid); /* always 0 on Win */
	lua_setfield(L, -2, "uid");
	lua_pushnumber(L, sbuf.st_gid); /* always 0 on Win */
	lua_setfield(L, -2, "gid");
#ifdef G_OS_UNIX
	push_owner(L, sbuf.st_uid);
	lua_setfield(L, -2, "owner");
	push_group(L, sbuf.st_gid);
	lua_setfield(L, -2, "group");
	lua_pushnumber(L, sbuf.st_blksize);
	lua_setfield(L, -2, "blksize");
	lua_pushnumber(L, sbuf.st_blocks);
	lua_setfield(L, -2, "blocks");
#endif
	return 1;
    }
    /* annoyingly inefficient, but for now that's OK */
    {
	int i;
	int popit = lua_istable(L, 2);

	for(i = 0; i < nret; i++) {
	    const char *s = lua_tostring(L, pstart + i);
	    if(!strcasecmp(s, "size"))
		lua_pushnumber(L, sbuf.st_size);
	    else if(!strcasecmp(s, "mtime"))
		lua_pushnumber(L, sbuf.st_mtime);
	    else if(!strcasecmp(s, "ctime"))
		lua_pushnumber(L, sbuf.st_ctime);
	    else if(!strcasecmp(s, "atime"))
		lua_pushnumber(L, sbuf.st_atime);
	    else if(!strcasecmp(s, "mode"))
		push_mode(L, sbuf.st_mode);
	    else if(!strcasecmp(s, "type"))
		push_type(L, sbuf.st_mode);
	    else if(!strcasecmp(s, "perm"))
		lua_pushnumber(L, sbuf.st_mode & 0x7777);
	    else if(!strcasecmp(s, "dev"))
		lua_pushnumber(L, sbuf.st_dev);
	    else if(!strcasecmp(s, "ino"))
		lua_pushnumber(L, sbuf.st_ino);
	    else if(!strcasecmp(s, "nlink"))
		lua_pushnumber(L, sbuf.st_nlink);
	    else if(!strcasecmp(s, "rdev"))
		lua_pushnumber(L, sbuf.st_rdev);
	    else if(!strcasecmp(s, "uid")) /* always 0 on Win */
		lua_pushnumber(L, sbuf.st_uid);
	    else if(!strcasecmp(s, "gid")) /* always 0 on Win */
		lua_pushnumber(L, sbuf.st_gid);
#ifdef G_OS_UNIX
	    else if(!strcasecmp(s, "blksize"))
		lua_pushnumber(L, sbuf.st_blksize);
	    else if(!strcasecmp(s, "blocks"))
		lua_pushnumber(L, sbuf.st_blocks);
	    else if(!strcasecmp(s, "owner"))
		push_owner(L, sbuf.st_uid);
	    else if(!strcasecmp(s, "group"))
		push_group(L, sbuf.st_gid);
#endif
	    else if(!strcasecmp(s, "link"))
		; /* skip */
	    else
		lua_pushnil(L);
	    if(popit)
		lua_remove(L, -nret - 1);
	}
    }
    return actret;
}

/***
Remove a file sytem entity.
This is a wrapper for `g_remove()`
@function rename
@tparam string name The entity to remove
@treturn boolean ok True on success.
@treturn string msg Error message on failure.  Note that the error message
 is probably invalid if the entity was not a directory.
*/
static int glib_remove(lua_State *L)
{
    int ret = g_remove(luaL_checkstring(L, 1));
    if(ret < 0) {
	int en = errno;
	lua_pushboolean(L, 0);
	lua_pushstring(L, strerror(en));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/***
Change filesystem entry permissions.
This is a wrapper for `g_chmod()`.  In addition, it will query the original
file for information if a string-style permission is used.
@function chmod
@tparam string name The filesystem entry to modify
@tparam string|number perm Permissions to set.
@treturn boolean True if no error
@treturn string Message if error
*/
static int glib_chmod(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    int mode;

    if(lua_isnumber(L, 2))
	mode = lua_tonumber(L, 2);
    else {
	luaL_checkstring(L, 2);
	GStatBuf sbuf;
	if(g_stat(s, &sbuf) < 0) {
	    int en = errno;
	    lua_pushboolean(L, 0);
	    lua_pushstring(L, strerror(en));
	    return 2;
	}
	mode = getmode(L, 2, sbuf.st_mode & 07777, S_ISDIR(sbuf.st_mode));
    }
    if(g_chmod(s, mode) < 0) {
	int en = errno;
	lua_pushboolean(L, 0);
	lua_pushstring(L, strerror(en));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/***
Test if fileystem object can be read.
This is a wrapper for `g_access()`.  Note that this function does not
necessarily check access control lists, so it may be better to just
test open/read the file.
@function can_read
@tparam string name Name of object to test
@treturn boolean true if *name* can be read.
@treturn string message if *name* cannot be read.
@treturn number error number if *name* cannot be read.
*/
static int glib_can_read(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    gboolean ok = g_access(s, R_OK) == 0;
    int en = errno;
    lua_pushboolean(L, ok);
    if(!ok) {
	lua_pushstring(L, strerror(en));
	lua_pushnumber(L, en);
	return 3;
    }
    return 1;
}

/***
Test if fileystem object can be written to.
This is a wrapper for `g_access()`.  Note that this function does not
necessarily check access control lists, so it may be better to just
test open/write the file.
@function can_write
@tparam string name Name of object to test
@treturn boolean true if *name* can be written to.
@treturn string message if *name* cannot be written to.
@treturn number error number if *name* cannot be written to.
*/
static int glib_can_write(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    gboolean ok = g_access(s, W_OK) == 0;
    int en = errno;
    lua_pushboolean(L, ok);
    if(!ok) {
	lua_pushstring(L, strerror(en));
	lua_pushnumber(L, en);
	return 3;
    }
    return 1;
}

/***
Change current working directory.
This is a wrapper for `g_chdir()`
@function chdir
@tparam string dir Name of directory to change into
@treturn boolean true if successful
@treturn string message if unsuccessful
*/
static int glib_chdir(lua_State *L)
{
    gboolean ok = g_chdir(luaL_checkstring(L, 1)) == 0;
    if(!ok) {
	int en = errno;
	lua_pushboolean(L, 0);
	lua_pushstring(L, strerror(en));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/***
Change timestamp on filesystem object.
This is a wrapper for `g_utime()`
@function utime
@tparam string name Name of entry to touch
@tparam number|nil mtime (Optional) modification time; current time is used
 if not present and *atime* not present; left alone if only *atime* is
 present.
@tparam number|nil atime (Optional) access time; modification time is
 used if not present.
@treturn boolean true on success
@treturn string error message on failure
*/
/* FIXME: this function is pretty useless:
 *   - it doesn't support even microsecond resolution (utimes)
 *   - it doesn't define what the time stamps mean (seconds since when?)
 *   - it doesn't create the file if it doesn't exist (i.e., not "touch")
 */
static int glib_utime(lua_State *L)
{
    struct utimbuf t;
    const char *s = luaL_checkstring(L, 1);
    gboolean has_mtime = !lua_isnoneornil(L, 2),
	     has_atime = !lua_isnoneornil(L, 3);
    int ret;

    if(has_mtime)
	t.modtime = lua_tonumber(L, 2);
    else if(has_atime) {
	GStatBuf sbuf;
	if(!g_stat(s, &sbuf))
	    t.modtime = sbuf.st_mtime;
	else
	    t.modtime = lua_tonumber(L, 3);
    }
    if(has_atime)
	t.actime = lua_tonumber(L, 3);
    else
	t.actime = t.modtime;
    if(has_atime || has_mtime)
	ret = g_utime(s, &t);
    else
	ret = g_utime(s, NULL);
    if(ret < 0) {
	int en = errno;
	lua_pushboolean(L, 0);
	lua_pushstring(L, strerror(en));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/*********************************************************************/
/***
URI Functions
@section URI Functions
*/

/* defined below */
/***
Allowed characters in a path.
@table uri_reserved_chars_allowed_in_path
*/

/***
Allowed characters in path elements.
@table uri_reserved_chars_allowed_in_path_element
*/

/***
Allowed characters in userinfo (RFC 3986).
@table uri_reserved_chars_allowed_in_userinfo
*/

/***
Generic delimiter characters (RFC 3986).
@table uri_reserved_chars_generic_delimiters
*/

/***
Subcomponent delimiter characters (RFC 3986).
@table uri_reserved_chars_subcomponent_delimiters
*/

/***
Extract scheme from URI.
@function uri_parse_scheme
@tparam string uri The valid URI
@treturn string|nil The scheme, or nil on error.
*/
static int glib_uri_parse_scheme(lua_State *L)
{
    lua_pushstring(L, g_uri_parse_scheme(luaL_checkstring(L, 1)));
    return 1;
}

/***
Escapes a string for use in a URI.
@function uri_escape_string
@tparam string s The string to esacape; nuls are not allowed.
@tparam string allow (Optional) Reserved characters to leave unescaped anyway.
@tparam boolean utf8 (Optional) Allow UTF-8 characters in result.
@treturn string The escaped string.
*/
static int glib_uri_escape_string(lua_State *L)
{
    const char *allow = lua_isnoneornil(L, 2) ? NULL : luaL_checkstring(L, 2);
    gboolean utf8 = lua_toboolean(L, 3);
    char *res = g_uri_escape_string(luaL_checkstring(L, 1), allow, utf8);
    lua_pushstring(L, res);
    g_free(res);
    return 1;
}

/***
Unescapes an escaped string.
@function uri_unescape_string
@tparam string s The string to unescape
@tparam string illegal (Optional) Characters which may not appear in the
 result; nul characters are automatically illegal.
@treturn string The unescaped string.
*/
static int glib_uri_unescape_string(lua_State *L)
{
    const char *ill = lua_isnoneornil(L, 2) ? NULL : luaL_checkstring(L, 2);
    char *res = g_uri_unescape_string(luaL_checkstring(L, 1), ill);
    lua_pushstring(L, res);
    g_free(res);
    return 1;
}

/***
Splits an URI list conforming to the text/uri-list MIME type (RFC 2483).
@function uri_list_extract_uris
@tparam string list The URI list
@treturn {string,...} A table containing the URIs
*/
static int glib_uri_list_extract_uris(lua_State *L)
{
    gchar **l = g_uri_list_extract_uris(luaL_checkstring(L, 1)), **p;
    int nent;
    for(nent = 0, p = l; *p; ++p, ++nent);
    lua_createtable(L, nent, 0);
    for(nent = 0, p = l; *p; ++p, ++nent) {
	lua_pushstring(L, *p);
	lua_rawseti(L, -2, nent + 1);
    }
    g_strfreev(l);
    return 1;
}

/***
Converts an ASCII-encoded URI to a local filename.
@function filename_from_uri
@tparam string uri The URI
@treturn string|nil The file name, or nil on error.
@treturn string|nil The host name, or nil if none, or error message if
 error
*/
static int glib_filename_from_uri(lua_State *L)
{
    gchar *host;
    GError *err = NULL;
    gchar *n = g_filename_from_uri(luaL_checkstring(L, 1), &host, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, n);
    g_free(n);
    lua_pushstring(L, host);
    if(host)
	g_free(host);
    return 2;
}

/***
Converts an absolute filename to an escaped ASCII-encoded URI.
@function filename_to_uri
@tparam string file The filename
@tparam string host (Optional) The host name
@treturn string|nil The URI, or nil on error.
@treturn string The error message on error.
*/
static int glib_filename_to_uri(lua_State *L)
{
    GError *err = NULL;
    gchar *uri = g_filename_to_uri(luaL_checkstring(L, 1),
				   lua_isnoneornil(L, 2) ? NULL :
				     luaL_checkstring(L, 2), &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, uri);
    g_free(uri);
    return 1;
}

/*********************************************************************/
/***
Hostname Utilities.
@section Hostname Utilities
*/

/***
Convert a host name to its canonical ASCII form.
@function hostname_to_ascii
@tparam string hostname The name to convert
@treturn string|nil The converted name, or nil on error
*/
static int glib_hostname_to_ascii(lua_State *L)
{
    char *ret = g_hostname_to_ascii(luaL_checkstring(L, 1));
    lua_pushstring(L, ret);
    if(ret)
	g_free(ret);
    return 1;
}

/***
Convert a host name to its canonical Unicode form.
@function hostname_to_unicode
@tparam string hostname The name to convert
@treturn string|nil The converted name, or nil on error
*/
static int glib_hostname_to_unicode(lua_State *L)
{
    char *ret = g_hostname_to_unicode(luaL_checkstring(L, 1));
    lua_pushstring(L, ret);
    if(ret)
	g_free(ret);
    return 1;
}

/***
Check if a host name contains Unicode characters.
@function hostname_is_non_ascii
@tparam string hostname The name to check
@treturn boolean True if the host name needs to be converted to ASCII for
 non-IDN-aware contexts.
*/
static int glib_hostname_is_non_ascii(lua_State *L)
{
    lua_pushboolean(L, g_hostname_is_non_ascii(luaL_checkstring(L, 1)));
    return 1;
}

/***
Check if a host name contains ASCII-encoded Unicode characters.
@function hostname_is_ascii_encoded
@tparam string hostname The name to check
@treturn boolean True if the host name needs to be converted to Unicode for
 presentation and IDN-aware contexts.
*/
static int glib_hostname_is_ascii_encoded(lua_State *L)
{
    lua_pushboolean(L, g_hostname_is_ascii_encoded(luaL_checkstring(L, 1)));
    return 1;
}

/***
Check if a string is an IPv4 or IPv6 numeric address.
@function hostname_is_ip_address
@tparam string hostname The name to check
@treturn boolean True if the host name is actually an IP address
*/
static int glib_hostname_is_ip_address(lua_State *L)
{
    lua_pushboolean(L, g_hostname_is_ip_address(luaL_checkstring(L, 1)));
    return 1;
}

/*********************************************************************/
/***
Shell-related Utilities.
@section Shell-related Utilities
*/

/***
Parse a command line into an argument vector, but without variable and
glob pattern expansion.
@function shell_parse_argv
@tparam string cmdline The command line to parse
@treturn {string,...}|nil A table containing the command-line arguments, or
 nil on error
@treturn string An error message on error
*/
static int glib_shell_parse_argv(lua_State *L)
{
    GError *err = NULL;
    gint argc;
    gchar **argv, **ap;
    if(!g_shell_parse_argv(luaL_checkstring(L, 1), &argc, &argv, &err)) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_createtable(L, argc, 0);
    for(ap = argv, argc = 1; *ap; ap++, argc++) {
	lua_pushstring(L, *ap);
	lua_rawseti(L, -2, argc);
    }
    return 1;
}

/***
Quote a string so it is interpreted unmodified as a shell argument.
@function shell_quote
@tparam string s The string to quote
@treturn string The quoted string
*/
static int glib_shell_quote(lua_State *L)
{
    char *ret = g_shell_quote(luaL_checkstring(L, 1));
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Unquote a string quoted for use as a shell argument.
@function shell_unquote
@tparam string s The string to unquote
@treturn string|nil The unquoted string, or nil on error.
@treturn string An error message on error
*/
static int glib_shell_unquote(lua_State *L)
{
    GError *err = NULL;
    char *ret = g_shell_unquote(luaL_checkstring(L, 1), &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/*********************************************************************/
/***
Perl-compatible Regular Expressions
@section Perl-compatible Regular Expressions
*/

typedef struct regex_state {
    GRegex *rex;
    gboolean do_all;
} regex_state;

typedef struct rex_flag {
    const char *name;
    int flag;
} rex_flag;

static rex_flag comp_flags[] = {
    { "anchored", G_REGEX_ANCHORED },
    { "caseless", G_REGEX_CASELESS },
    { "dollar_endonly", G_REGEX_DOLLAR_ENDONLY },
    { "dotall", G_REGEX_DOTALL },
    { "dupnames", G_REGEX_DUPNAMES },
    { "extended", G_REGEX_EXTENDED },
    { "multiline", G_REGEX_MULTILINE },
    { "newline_cr", G_REGEX_NEWLINE_CR },
    { "newline_crlf", G_REGEX_NEWLINE_CRLF },
    { "newline_lf", G_REGEX_NEWLINE_LF },
    { "no_auto_capture", G_REGEX_NO_AUTO_CAPTURE },
    { "optimize", G_REGEX_OPTIMIZE },
    { "raw", G_REGEX_RAW },
    { "ungreedy", G_REGEX_UNGREEDY }
};
#define NUM_COMP_FLAGS (sizeof(comp_flags)/sizeof(comp_flags[0]))

static rex_flag exec_flags[] = {
    { "all", -1 },
    { "anchored", G_REGEX_MATCH_ANCHORED },
    { "newline_any", G_REGEX_MATCH_NEWLINE_ANY },
    { "newline_cr", G_REGEX_MATCH_NEWLINE_CR },
    { "newline_crlf", G_REGEX_MATCH_NEWLINE_CRLF },
    { "newline_lf", G_REGEX_MATCH_NEWLINE_LF },
    { "notbol", G_REGEX_MATCH_NOTBOL },
    { "notempty", G_REGEX_MATCH_NOTEMPTY },
    { "noteol", G_REGEX_MATCH_NOTEOL },
    { "partial", G_REGEX_MATCH_PARTIAL }
};
#define NUM_EXEC_FLAGS (sizeof(exec_flags)/sizeof(exec_flags[0]))

static gboolean get_mfl(lua_State *L, int index, GRegexMatchFlags *mfl,
			gboolean *do_all)
{
    *mfl = 0;
    if(!lua_isnoneornil(L, index)) {
	int i;
	luaL_checktype(L, index, LUA_TTABLE);
	for(i = lua_len(L, index); i > 0; --i) {
	    const char *s;
	    rex_flag *rf;
	    lua_pushinteger(L, i);
	    lua_gettable(L, index);
	    s = luaL_checkstring(L, -1);
	    rf = bsearch(s, exec_flags, NUM_EXEC_FLAGS, sizeof(exec_flags[0]), ns_cmp);
	    if(rf && rf->flag != -1)
		*mfl |= rf->flag;
	    else if(rf)
		*do_all = TRUE;
	    else {
		lua_pushnil(L);
		lua_pushliteral(L, "Unknown exec flag: ");
		lua_pushvalue(L, -3);
		lua_concat(L, 2);
		lua_remove(L, -3);
		lua_remove(L, -3);
		return FALSE;
	    }
	    lua_pop(L, 1);
	}
    }
    return TRUE;
}

/***
Compile a regular expression for use in matching functions.
@function regex_new
@tparam string pattern The regular expression.  Note that embedded NUL
 characters are supported.
@tparam {string,...} cflags (Optional) Compile flags: 'caseless',
 'multiline', 'dotall', 'extended', 'anchored', 'dollar\_endonly',
 'ungreedy', 'raw', 'no\_auto\_capture', 'optimize', 'dupnames',
 'newline\_cr', 'newline\_lf', 'newline\_crlf'
@tparam {string,...} mflags (Optional) Match flags:
 'anchored', 'notbol', 'noteol', 'notempty', 'partial', 'newline\_cr',
 'newline\_lf', 'newline\_crlf', 'newline\_any', 'all'.  'all' changes the
 behavior of matches from returning captures to returning all potential
 matches.
@treturn regex|nil The compiled regular expression, or nil on error
@treturn string The error message on error
*/
static int glib_regex_new(lua_State *L)
{
    GError *err = NULL;
    GRegexCompileFlags cfl = 0;
    GRegexMatchFlags mfl = 0;
    size_t len;
    const char *rex;
    int stop = lua_gettop(L);
    alloc_udata(L, st, regex_state);
    rex = luaL_checklstring(L, 1, &len);
    if(stop > 1 && !lua_isnoneornil(L, 2)) {
	int i;
	luaL_checktype(L, 2, LUA_TTABLE);
	for(i = lua_len(L, 2); i > 0; --i) {
	    const char *s;
	    rex_flag *rf;
	    lua_pushinteger(L, i);
	    lua_gettable(L, 2);
	    s = luaL_checkstring(L, -1);
	    rf = bsearch(s, comp_flags, NUM_COMP_FLAGS, sizeof(comp_flags[0]), ns_cmp);
	    if(rf)
		cfl |= rf->flag;
	    else {
		lua_pushnil(L);
		lua_pushliteral(L, "Unknown comp flag: ");
		lua_pushvalue(L, -3);
		lua_concat(L, 2);
		lua_remove(L, -3);
		lua_remove(L, -3);
		return 2;
	    }
	    lua_pop(L, 1);
	}
    }
    if(stop > 2 && !get_mfl(L, 3, &mfl, &st->do_all))
	return 2;
    if(memchr(rex, 0, len)) {
	gchar *esc = g_regex_escape_nul(rex, len);
	st->rex = g_regex_new(esc, cfl, mfl, &err);
	g_free(esc);
    } else
	st->rex = g_regex_new(rex, cfl, mfl, &err);
    if(err) {
	lua_pop(L, 1);
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Escapes a string so it is a literal in a regular expression.
@function regex_escape_string
@tparam string s The string to escape
@treturn string The escaped string.
*/
static int glib_regex_escape_string(lua_State *L)
{
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    gchar *ret = g_regex_escape_string(s, len);
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
@type regex
*/

static int free_regex_state(lua_State *L)
{
    get_udata(L, 1, st, regex_state);
    if(st->rex) {
	g_regex_unref(st->rex);
	st->rex = NULL;
    }
    return 0;
}

/***
Get the pattern string used to create this regex.
@function regex:get_pattern
@treturn string The pattern string
*/
static int regex_get_pattern(lua_State *L)
{
    get_udata(L, 1, st, regex_state);
    lua_pushstring(L, g_regex_get_pattern(st->rex));
    return 1;
}

/***
Get the highest back reference in the pattern.
@function regex:get_max_backref
@treturn number The number of the highest backreference, or 0 if there are
 none.
*/
static int regex_get_max_backref(lua_State *L)
{
    get_udata(L, 1, st, regex_state);
    lua_pushnumber(L, g_regex_get_max_backref(st->rex));
    return 1;
}

/***
Get the number of capturing subpatterns in the pattern.
@function regex:get_capture_count
@treturn number The number of capturing subpatterns.
*/
static int regex_get_capture_count(lua_State *L)
{
    get_udata(L, 1, st, regex_state);
    lua_pushnumber(L, g_regex_get_capture_count(st->rex));
    return 1;
}

/***
Get the number of the capturing subexpression with the given name.
@function regex:get_string_number
@tparam string name The subexpression name
@treturn number The subexpression number, or -1 if *name* does not exist
*/
static int regex_get_string_number(lua_State *L)
{
    get_udata(L, 1, st, regex_state);
    lua_pushnumber(L, g_regex_get_string_number(st->rex, luaL_checkstring(L, 2)));
    return 1;
}

/***
Get the names of all compile flags set when regex was created.
@function regex:get_compile_flags
@treturn {string,...} The names of any flags set when regex was created.
*/
static int regex_get_compile_flags(lua_State *L)
{
    GRegexCompileFlags cf;
    int nfl, i;
    get_udata(L, 1, st, regex_state);
    cf = g_regex_get_compile_flags(st->rex);
    for(i = nfl = 0; i < NUM_COMP_FLAGS; i++)
	if(comp_flags[i].flag & cf)
	    ++nfl;
    lua_createtable(L, nfl, 0);
    for(i = nfl = 0; i < NUM_COMP_FLAGS; i++)
	if(comp_flags[i].flag & cf) {
	    ++nfl;
	    lua_pushstring(L, comp_flags[i].name);
	    lua_rawseti(L, -2, nfl);
	}
    return 1;
}

/***
Get the names of all matching flags set when regex was created.
@function regex:get_match_flags
@treturn {string,...} The names of any flags set when regex was created.
*/
static int regex_get_match_flags(lua_State *L)
{
    GRegexMatchFlags mf;
    int nfl, i;
    get_udata(L, 1, st, regex_state);
    mf = g_regex_get_match_flags(st->rex);
    for(i = nfl = 0; i < NUM_COMP_FLAGS; i++)
	if(exec_flags[i].flag & mf)
	    ++nfl;
    lua_createtable(L, nfl, 0);
    for(i = nfl = 0; i < NUM_COMP_FLAGS; i++)
	if(exec_flags[i].flag & mf) {
	    ++nfl;
	    lua_pushstring(L, exec_flags[i].name);
	    lua_rawseti(L, -2, nfl);
	}
    return 1;
}

static int regex_search(lua_State *L, gboolean *matched, gboolean *partial,
			GMatchInfo **mi, const char **s)
{
    GError *err = NULL;
    size_t len;
    int sp = lua_isnoneornil(L, 3) ? 0 : luaL_checkinteger(L, 3) - 1;
    GRegexMatchFlags mfl;
    gboolean do_all;
    get_udata(L, 1, st, regex_state);

    *s = luaL_checklstring(L, 2, &len);
    do_all = st->do_all;
    if(!get_mfl(L, 4, &mfl, &do_all))
	return 2;
    *partial = (mfl & G_REGEX_MATCH_PARTIAL) ||
	       (g_regex_get_match_flags(st->rex) & G_REGEX_MATCH_PARTIAL);
    if(do_all)
	*matched = g_regex_match_all_full(st->rex, *s, len, sp, mfl, mi, &err);
    else
	*matched = g_regex_match_full(st->rex, *s, len, sp, mfl, mi, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    if(!*matched && *partial) {
	if(!g_match_info_is_partial_match(*mi)) {
	    g_match_info_free(*mi);
	    lua_pushnil(L);
	    return 1;
	}
    } else if(!*matched) {
	g_match_info_free(*mi);
	lua_pushnil(L);
	return 1;
    }
    return 0;
}

/***
Search for a match in a string.
@function regex:find
@see string.find
@tparam string s The string to search
@tparam number start (Optional) The start position.
@tparam {string,...} mflags (Optional) Additional match flags:
 'anchored', 'notbol', 'noteol', 'notempty', 'partial', 'newline\_cr',
 'newline\_lf', 'newline\_crlf', 'newline\_any', 'all'.  'all' changes the
 behavior of matches from returning captures to returning all potential
 matches.
@treturn number|nil The location of the start of the first match, or nil
 if there are no matches or an error occurred.
@treturn number|string The location of the last character of the first
 match, or an error message if an error occurred.
@treturn boolean Present only if the 'partial' flag is set: true if the
 match was full; false if only partial.
@treturn string,... On a successful match, all captures are returned as
 well
*/
static int regex_find(lua_State *L)
{
    gboolean matched, partial;
    GMatchInfo *mi;
    const char *s;
    int nret = regex_search(L, &matched, &partial, &mi, &s);
    if(nret)
	return nret;
    {
	gint start, end;

	g_match_info_fetch_pos(mi, 0, &start, &end);
	lua_pushinteger(L, start + 1);
	lua_pushinteger(L, end);
    }
    if(partial)
	lua_pushboolean(L, matched);
    {
	int nmatch = g_match_info_get_match_count(mi), i;
	for(i = 1; i < nmatch; i++) {
	    gint start, end;
	    g_match_info_fetch_pos(mi, i, &start, &end);
	    lua_pushlstring(L, s + start, end - start);
	}
	g_match_info_free(mi);
	return nmatch + 1 + (partial ? 1 : 0);
    }
}

/***
Search for a match in a string.
@function regex:match
@see string.match
@tparam string s The string to search
@tparam number start (Optional) The start position.
@tparam {string,...} mflags (Optional) Additional match flags:
 'anchored', 'notbol', 'noteol', 'notempty', 'partial', 'newline\_cr',
 'newline\_lf', 'newline\_crlf', 'newline\_any', 'all'.  'all' changes the
 behavior of matches from returning captures to returning all potential
 matches.
@treturn boolean Present only if the 'partial' flag is set: true if the
 match was full; false if only partial.
@treturn string|nil The first capture, or the full match if there are no
 captures, or nil if there are no matches or an error occurred.
@treturn string|nil  The second caputre, or an error message if an error
 occurred.
@treturn string,... On a successful match, all remaining captures are
 returned as well
*/
static int regex_match(lua_State *L)
{
    gboolean matched, partial;
    GMatchInfo *mi;
    const char *s;
    int nret = regex_search(L, &matched, &partial, &mi, &s);
    if(nret)
	return nret;
    if(partial)
	lua_pushboolean(L, matched);
    {
	int nmatch = g_match_info_get_match_count(mi), i;
	if(nmatch > 1) {
	    for(i = 1; i < nmatch; i++) {
		gint start, end;
		g_match_info_fetch_pos(mi, i, &start, &end);
		lua_pushlstring(L, s + start, end - start);
	    }
	    g_match_info_free(mi);
	    return nmatch - 1 + (partial ? 1 : 0);
	} else {
	    gint start, end;

	    g_match_info_fetch_pos(mi, 0, &start, &end);
	    lua_pushlstring(L, s + start, end - start);
	    g_match_info_free(mi);
	    return 1;
	}
    }
}

typedef struct regex_iter_state {
    GMatchInfo *mi;
    const char *s;
    gboolean partial;
} regex_iter_state;

static int free_regex_iter_state(lua_State *L)
{
    get_udata(L, 1, st, regex_iter_state);
    if(st->mi) {
	g_match_info_free(st->mi);
	st->mi = NULL;
    }
    return 0;
}

static int regex_iter(lua_State *L)
{
    gboolean matched;
    get_udata(L, lua_upvalueindex(1), st, regex_iter_state);
    if(!st->mi) {
	lua_pushnil(L);
	return 1;
    }
    matched = g_match_info_matches(st->mi);
    if(!matched && (!st->partial || !g_match_info_is_partial_match(st->mi))) {
	g_match_info_free(st->mi);
	st->mi = NULL;
	lua_pushnil(L);
	return 1;
    }
    if(st->partial)
	lua_pushboolean(L, matched);
    {
	int nmatch = g_match_info_get_match_count(st->mi), i;
	if(nmatch > 1) {
	    for(i = 1; i < nmatch; i++) {
		gint start, end;
		g_match_info_fetch_pos(st->mi, i, &start, &end);
		lua_pushlstring(L, st->s + start, end - start);
	    }
	    g_match_info_next(st->mi, NULL);
	    return nmatch - 1 + (st->partial ? 1 : 0);
	} else {
	    gint start, end;

	    g_match_info_fetch_pos(st->mi, 0, &start, &end);
	    lua_pushlstring(L, st->s + start, end - start);
	    g_match_info_next(st->mi, NULL);
	    return 1;
	}
    }
}
    
/***
Search for all matches in a string.
@function regex:gmatch
@see string.gmatch
@see regex:match
@tparam string s The string to search
@tparam number start (Optional) The start position.
@tparam {string,...} mflags (Optional) Additional match flags:
 'anchored', 'notbol', 'noteol', 'notempty', 'partial', 'newline\_cr',
 'newline\_lf', 'newline\_crlf', 'newline\_any', 'all'.  'all' changes the
 behavior of matches from returning captures to returning all potential
 matches.
@treturn function An iterator function which, on each iteration,
 returns the same as regex:match would have for the next match in the
 string.
*/
static int regex_gmatch(lua_State *L)
{
    gboolean matched, partial;
    GMatchInfo *mi;
    const char *s;
    int nret = regex_search(L, &matched, &partial, &mi, &s);
    if(nret > 1)
	return nret;
    {
	alloc_udata(L, st, regex_iter_state);
	lua_pushvalue(L, 1); /* save string reference */
	lua_pushcclosure(L, regex_iter, 2);
	if(nret)
	    return 1;
	st->partial = partial;
	st->mi = mi;
	st->s = s;
	return 1;
    }
}

/***
Split a string with a regular expression separator.
@function regex:split
@tparam string s The string to split
@tparam number start (Optional) The start position.
@tparam {string,...} mflags (Optional) Additional match flags:
 'anchored', 'notbol', 'noteol', 'notempty', 'newline\_cr',
 'newline\_lf', 'newline\_crlf', 'newline\_any'
@tparam number max (Optional) The maximum number of elements to return.  If
 unspecified or less than 1, all elements are returned.
@treturn {string,...}  The elements separated by the regular expression.
 Each element is separated by any capture strings from the separator, if
 present.  For example:

     rx = glib.regex_new('\\|(.?)\\|')
     spl = rx:split('abc||def|!|ghi')
      -- { 'abc', '', 'def', '!', 'ghi' }
*/
static int regex_split(lua_State *L)
{
    GError *err = NULL;
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);
    int sp = lua_isnoneornil(L, 3) ? 0 : luaL_checkinteger(L, 3) - 1;
    GRegexMatchFlags mfl;
    gboolean do_all, partial;
    int max = lua_isnoneornil(L, 5) ? 0 : luaL_checkinteger(L, 5);
    gchar **ret;
    int nret;
    get_udata(L, 1, st, regex_state);

    do_all = st->do_all;
    if(!get_mfl(L, 4, &mfl, &do_all))
	return 2;
    if(do_all) {
	lua_pushnil(L);
	lua_pushliteral(L, "all mode not supported for split");
	return 2;
    }
    partial = (mfl & G_REGEX_MATCH_PARTIAL) ||
	      (g_regex_get_match_flags(st->rex) & G_REGEX_MATCH_PARTIAL);
    if(partial) {
	lua_pushnil(L);
	lua_pushliteral(L, "partial mode not supported for split");
	return 2;
    }
    ret = g_regex_split_full(st->rex, s, len, sp, mfl, max, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    for(nret = 0; ret[nret]; ++nret);
    lua_createtable(L, nret, 0);
    for(nret = 0; ret[nret]; ++nret) {
	lua_pushstring(L, ret[nret]);
	lua_rawseti(L, -2, nret + 1);
    }
    g_strfreev(ret);
    return 1;
}

typedef struct repl_state {
    lua_State *L;
    int tr, tn;
    int nmatch, nsub;
} repl_state;

static gboolean regex_repl(const GMatchInfo *mi, GString *res, gpointer data)
{
    repl_state *rs = data;
    lua_State *L = rs->L;
    int tr = rs->tr;
    int tn = rs->tn;
    int nmatch = g_match_info_get_match_count(mi);
    gint start, end;
    const char *s = g_match_info_get_string(mi);

    ++rs->nmatch;
    switch(tr) {
      case LUA_TTABLE:
	g_match_info_fetch_pos(mi, nmatch > 1, &start, &end);
	lua_pushlstring(L, s + start, end - start);
	lua_gettable(L, 3);
	break;
      case LUA_TFUNCTION:
	lua_pushvalue(L, 3);
	if(nmatch > 1) {
	    int i;
	    for(i = 1; i < nmatch; i++) {
		gint start, end;
		g_match_info_fetch_pos(mi, i, &start, &end);
		lua_pushlstring(L, s + start, end - start);
	    }
	    lua_call(L, nmatch - 1, 1);
	} else {
	    gint start, end;

	    g_match_info_fetch_pos(mi, 0, &start, &end);
	    lua_pushlstring(L, s + start, end - start);
	    lua_call(L, 1, 1);
	}
      default: {
	  const char *rt = lua_tostring(L, 3);
	  gchar *repl = g_match_info_expand_references(mi, rt, NULL);
	  if(!repl)
	      lua_pushliteral(L, "");
	  else {
	      lua_pushstring(L, repl);
	      g_free(repl);
	  }
	  break;
      }
    }
    if(tn == LUA_TFUNCTION) {
	gboolean do_repl;
	lua_pushvalue(L, 5);
	g_match_info_fetch_pos(mi, 0, &start, &end);
	lua_pushinteger(L, start + 1);
	lua_pushinteger(L, end);
	lua_pushvalue(L, -4);
	lua_call(L, 3, 2);
	if(lua_isboolean(L, -2) || lua_isnil(L, -2))
	    do_repl = lua_toboolean(L, -2);
	else {
	    lua_pushvalue(L, -2);
	    lua_replace(L, -4);
	    do_repl = TRUE;
	}
	if(lua_isnumber(L, -1)) {
	    lua_pushvalue(L, -1);
	    lua_replace(L, 5);
	    rs->tn = LUA_TNUMBER;
	} else {
	    if(lua_toboolean(L, -1))
		rs->tn = LUA_TNIL;
	}
	lua_pop(L, 2);
	if(!do_repl) {
	    lua_pop(L, 1);
	    lua_pushboolean(L, 0);
	}
    }
    {
	int rt = lua_type(L, -1);
	if(rt == LUA_TSTRING || rt == LUA_TNUMBER) {
	    size_t len;
	    const char *reps = lua_tolstring(L, -1, &len);
	    g_string_append_len(res, reps, len);
	    ++rs->nsub;
	} else if(!lua_toboolean(L, -1)) {
	    g_match_info_fetch_pos(mi, 0, &start, &end);
	    g_string_append_len(res, s + start, end - start);
	} else
	    /* else what?  presumably replace w/ blank */
	    ++rs->nsub;
	lua_pop(L, 1);
    }
    if(tn != LUA_TNONE && tn != LUA_TNIL && tn != LUA_TFUNCTION) {
	int max = lua_tonumber(L, 5);
	if(max > 0) {
	    if(--max) {
		lua_pushinteger(L, max);
		lua_replace(L, 5);
	    } else
		return TRUE;
	}
    }
    return FALSE;
}

/***
Replace occurrences of regular expression in string.
@function regex:gsub
@tparam string s The string to modify.  Note that zeroes are not supported
 in the final result string, whether they stem from *s* or *repl*.
@tparam string|table|function repl The replacement.  If this is a string,
 the string is the replacement text, which supports backslash-escapes for
 capture substitution and case conversion.  If it is a table, the first
 capture (or entire match if there are no captures) is used as a key into
 the table; if the value is a string or number, it is the literal replacement
 text; otherwise, if the value is faluse or nil, no substitution is made.
 If it is a function, the function is called for every match, with all
 captures (or the entire match if there are no captures) as arguments;
 the return value is treated like the table entries.  For literal
 interpretation of a string, call regex\_escape\_string on it first.
@see regex_escape_string
@tparam number start (Optional) The start position.
@tparam number|function n (Optional) The maximum number of replacements, if
 specified as a number greater than 0.  If specified as a function, the
 function is called after determining the potential replacement text.  Its
 parameters are the full match start position, the full match end position,
 and the potential replacement text (or false/nil if no replacement is to be
 made).  The function must return two values:  the first is the replacement
 operation:  true if normal replacement is to be made, false if no replacement
 is to be made, and a string if an alternate replacement is to be made.
 The second is the conintuation flag:  if absent, nil, or false, continue
 replacement, and if true, continue globally without checking *n*, and if
 a number, continue that many iterations maximum without checking *n*.
@tparam {string,...} mflags (Optional) Additional match flags:
 'anchored', 'notbol', 'noteol', 'notempty', 'newline\_cr',
 'newline\_lf', 'newline\_crlf', 'newline\_any'
@treturn string|nil The substituted string, if successful; nil if not.
@treturn number|string The number of matches, if successful; an error
 message if not.
@treturn number The number of substitutions, if successful
*/
/* note: function interpretation of n parameter is stolen from lrexlib */
/* note: nsub return value also stolen from lrexlib */
static int regex_gsub(lua_State *L)
{
    GError *err = NULL;
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);
    gchar *ret;
    repl_state rs;
    int sp = lua_isnoneornil(L, 4) ? 0 : luaL_checkinteger(L, 4) - 1;
    GRegexMatchFlags mfl;
    gboolean do_all, partial;
    get_udata(L, 1, st, regex_state);

    rs.L = L;
    rs.tr = lua_type(L, 3);
    rs.tn = lua_type(L, 5);
    rs.nmatch = rs.nsub = 0;
    if(rs.tr != LUA_TTABLE && rs.tr != LUA_TFUNCTION) {
	g_regex_check_replacement(luaL_checkstring(L, 3), NULL, &err);
	if(err) {
	    lua_pushnil(L);
	    lua_pushstring(L, err->message);
	    g_error_free(err);
	    return 2;
	}
	rs.tr = LUA_TSTRING;
    }
    if(rs.tn != LUA_TFUNCTION && rs.tn != LUA_TNONE && rs.tn != LUA_TNIL)
	luaL_checknumber(L, 5);
    do_all = st->do_all;
    if(!get_mfl(L, 4, &mfl, &do_all))
	return 2;
    if(do_all) {
	lua_pushnil(L);
	lua_pushliteral(L, "all mode not supported for gsub");
	return 2;
    }
    partial = (mfl & G_REGEX_MATCH_PARTIAL) ||
	      (g_regex_get_match_flags(st->rex) & G_REGEX_MATCH_PARTIAL);
    if(partial) {
	lua_pushnil(L);
	lua_pushliteral(L, "partial mode not supported for gsub");
	return 2;
    }
    ret = g_regex_replace_eval(st->rex, s, len, sp, mfl, regex_repl, &rs, &err);
    if(!ret) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    lua_pushinteger(L, rs.nmatch);
    lua_pushinteger(L, rs.nsub);
    return 3;
}

static luaL_Reg regex_state_funcs[] = {
    { "get_pattern", regex_get_pattern },
    { "get_max_backref", regex_get_max_backref },
    { "get_capture_count", regex_get_capture_count },
    { "get_string_number", regex_get_string_number },
    { "get_compile_flags", regex_get_compile_flags },
    { "get_match_flags", regex_get_match_flags },
    { "find", regex_find },
    { "match", regex_match },
    { "gmatch", regex_gmatch },
    { "split", regex_split },
    { "gsub", regex_gsub },
    { "__gc", free_regex_state },
    { NULL, NULL }
};

/*********************************************************************/
/***
Simple XML Subset Parser.
The following functions use varargs, and are not supported:

g\_markup\_\*printf\_escaped: emulate using string concat and renaming of
 escaper to shorter name:
     e = glib.markup_escape_text
     output = '<purchase>' ..
              '<store>' .. e(store) .. '</store>' ..
              '<item>' .. e(item) .. '</item>' ..
              '</purchase>'

g\_markup\_collect\_attributes: the only useful bit here that is for
some reason not exposed independently is the boolean parser.  Otherwise,
it's best to emulate using lua:
    -- usage:
    attrs, msg = collect_attrs(attr_names, attr_vals,
                               {a='str', b='bool', c='?bool', ...})
    if not attrs then print("error: " .. msg) end

    -- only allocate/parse once
    local bool_rx = glib.regex_new('^(false|f|no|n|0)|(true|t|yes|y|1)$', {'caseless'})
    -- could pass in el_name for better error message
    function collect_attrs(attr_names, attr_vals, req_parms)
      -- return attr=value table
      local attrs = {}
      -- put attrs i table, checking for duplicates
      local i, v
      for i, v in ipairs(attr_names) do
        if attrs[v] then return nil, "duplicate attribute " .. v end
        attrs[v] = attr_vals[i]
      end
      -- parse values, ensuring they are in req_parms as well
      local n
      for n, v in pairs(attrs) do
        local isreq = req_parms[n]
        if not isreq then return nil, "unknown attribute " .. n end
        if isreq:sub(1, 1) == '?' then isreq = isreq:sub(2) end
        if req_parms[n] == 'bool' then
          local f = bool_rx:match(v)
          if not f then return nil, "invalid boolean " .. n end
          attrs[n] = f == ''
        end
      end
      -- ensure that only optional values are missing
      for n, v in pairs(req_parms) do
        if v:sub(1, 1) ~= '?' and attrs[n] == nil then
          return nil, "missing attr " .. n
        end
      end
      return attrs
    end
@section Simple XML Subset Parser
*/

/***
Escape text so GMarkup XML parsing will return it to original.
@function markup_escape_text
@tparam string s The string to escape
@treturn string The escaped string
*/
static int glib_markup_escape_text(lua_State *L)
{
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    char *ret = g_markup_escape_text(s, len);
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

typedef struct markup_parse_state {
    GMarkupParseContext *ctx;
    lua_State *L;
    struct markup_parse_state *next;
} markup_parse_state;

static gboolean pop_parser(markup_parse_state *st, gboolean is_end)
{
    if(st->next) {
	lua_State *L = st->L;
	/* recursive call should never be needed, but better to do it than to
	 * accidentially leak memory */
	pop_parser(st->next, FALSE);
	g_markup_parse_context_pop(st->ctx);
	g_free(st->next);
	st->next = NULL;
	lua_getfield(L, -1, "next");
	lua_pushvalue(L, -1);
	lua_setuservalue(L, 1);
	if(is_end) { /* leave pop on stack */
	    lua_getfield(L, -2, "pop");
	    lua_remove(L, -3);
	} else
	    lua_remove(L, -2);
	return TRUE;
    }
    return FALSE;
}

/***
The function called when an opening tag of an element is seen.
@function _gmarkup_start_element_
Pass a function like this in the *start\_element* parameter for a parser.
@tparam markup_parse_context ctx The context in which this was called
@tparam string name The name of the element being started
@tparam {string,...} attr_names The names of the attributes in this tag,
 in order
@tparam {string,...} attr_values The values of the attributes in this tag,
 in the same order as the names.
@treturn nil|string Nothing or nil on success; an error message on error.
@see markup_parse_context_new
@see markup_parse_context:push
*/
static void gmp_start_element(GMarkupParseContext *ctx,
			      const gchar *element_name,
			      const gchar **attr_names,
			      const gchar **attr_values,
			      gpointer user_data,
			      GError **error)
{
    int nattr;
    markup_parse_state *st = user_data;
    lua_State *L = st->L;
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "start_element");
    lua_pushstring(L, element_name);
    for(nattr = 0; attr_names[nattr]; nattr++);
    lua_createtable(L, nattr, 0);
    lua_createtable(L, nattr, 0);
    for(nattr = 0; attr_names[nattr]; nattr++) {
	lua_pushstring(L, attr_names[nattr]);
	lua_rawseti(L, -3, nattr + 1);
	lua_pushstring(L, attr_values[nattr]);
	lua_rawseti(L, -2, nattr + 1);
    }
    lua_call(L, 3, 1);
    if(!lua_isnil(L, -1)) {
	const char *msg = lua_tostring(L, -1);
	/* FIXME: maybe support error codes in the future... */
	*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
			     "%s", msg);
    }
    lua_pop(L, 1);
}

/***
The function called when an ending tag of an element is seen.
@function _gmarkup_end_element_
Pass a function like this in the *end\_element* parameter for a parser.
@tparam markup_parse_context ctx The context in which this was called
@tparam string name The name of the element being started
@tparam any pop If the context was previously pushed, and this is called
 for the terminating element of that context, this is the `pop` element
 which was passed in to `markup_parse_context:push`.
@treturn nil|string Nothing or nil on success; an error message on error.
@see markup_parse_context_new
@see markup_parse_context:push
*/
static void gmp_end_element(GMarkupParseContext *ctx,
			    const gchar *element_name,
			    gpointer user_data,
			    GError **error)
{
    markup_parse_state *st = user_data;
    lua_State *L = st->L;
    gboolean did_pop;
    lua_getuservalue(L, 1);
    did_pop = pop_parser(st, TRUE);
    lua_getfield(L, did_pop ? -2 : -1, "end_element");
    if(lua_isnil(L, -1)) {
	lua_pop(L, 2);
	return;
    }
    lua_pushstring(L, element_name);
    if(did_pop) {
	lua_pushvalue(L, -3);
	lua_remove(L, -4);
	lua_call(L, 2, 1);
    } else
	lua_call(L, 1, 1);
    if(!lua_isnil(L, -1)) {
	const char *msg = lua_tostring(L, -1);
	/* FIXME: maybe support error codes in the future... */
	*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
			     "%s", msg);
    }
    lua_pop(L, 1);
}

/***
The function called when text within an element is seen.
@function _gmarkup_text_
Pass a function like this in the *text* parameter for a parser.
@tparam markup_parse_context ctx The context in which this was called
@tparam string text The text.
@treturn nil|string Nothing or nil on success; an error message on error.
@see markup_parse_context_new
@see markup_parse_context:push
*/
static void gmp_text(GMarkupParseContext *ctx,
		     const gchar *text,
		     gsize text_len,
		     gpointer user_data,
		     GError **error)
{
    markup_parse_state *st = user_data;
    lua_State *L = st->L;
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "text");
    lua_pushlstring(L, text, text_len);
    lua_call(L, 1, 1);
    if(!lua_isnil(L, -1)) {
	const char *msg = lua_tostring(L, -1);
	/* FIXME: maybe support error codes in the future... */
	*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
			     "%s", msg);
    }
    lua_pop(L, 1);
}

/***
The function called when unprocessed text is seen.
@function _gmarkup_passthrough_
Pass a function like this in the *passthrough* parameter for a parser.
When copying XML, this text is intended to be output literally, assuming
all other function calls output their tags immediately.
@tparam markup_parse_context ctx The context in which this was called
@tparam string text The text.
@treturn nil|string Nothing or nil on success; an error message on error.
@see markup_parse_context_new
@see markup_parse_context:push
*/
static void gmp_passthrough(GMarkupParseContext *ctx,
			    const gchar *text,
			    gsize text_len,
			    gpointer user_data,
			    GError **error)
{
    markup_parse_state *st = user_data;
    lua_State *L = st->L;
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "passthrough");
    lua_pushlstring(L, text, text_len);
    lua_call(L, 1, 1);
    if(!lua_isnil(L, -1)) {
	const char *msg = lua_tostring(L, -1);
	/* FIXME: maybe support error codes in the future... */
	*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
			     "%s", msg);
    }
    lua_pop(L, 1);
}

/***
The function called when an error occurs during parsing.
@function _gmarkup_error_
Pass a function like this in the *error* parameter for a parser.
@tparam markup_parse_context ctx The context in which this was called
@tparam string text The error text.
@see markup_parse_context_new
@see markup_parse_context:push
*/
static void gmp_error(GMarkupParseContext *ctx,
		      GError *error,
		      gpointer user_data)
{
    markup_parse_state *st = user_data;
    lua_State *L = st->L;
    lua_getuservalue(L, 1);
    pop_parser(st, FALSE);
    lua_getfield(L, -1, "error");
    if(lua_isnil(L, -1)) {
	lua_pop(L, 2);
	return;
    }
    lua_pushstring(L, error->message);
    lua_call(L, 1, 0);
}

/***
Create GMarkup parser.
@function markup_parse_context_new
@tparam {string=value,...} options Context creation options:

   * **start\_element**: a function like `_gmarkup_start_element_`.
   * **end\_element**: a function like `_gmarkup_end_element_`.
   * **text**: a function like `_gmarkup_text_`.
   * **passthrough**: a function like `_gmarkup_passthrough_`.
   * **error**: a function like `_gmarkup_error_`.
   * **treat\_cdata\_as\_text**: If set and not false, return CDATA sections
     as text instead of passthrough.
   * **prefix\_error\_position**: If set and not false, prefix error messages
     returned by the above functions with line and column information.
@see _gmarkup_start_element_
@see _gmarkup_end_element_
@see _gmarkup_text_
@see _gmarkup_passthrough_
@see _gmarkup_error_
*/
static int glib_markup_parse_context_new(lua_State *L)
{
    GMarkupParser parser;
    GMarkupParseFlags fl = 0;

    luaL_checktype(L, 1, LUA_TTABLE);
    {
	alloc_udata(L, st, markup_parse_state);
	memset(&parser, 0, sizeof(parser));
	lua_newtable(L);
#define getfun(n, p) do { \
    lua_getfield(L, p, #n); \
    if(!lua_isnil(L, -1)) { \
	luaL_checktype(L, -1, LUA_TFUNCTION); \
	parser.n = gmp_##n; \
	lua_setfield(L, -2, #n); \
    } \
} while(0)
	getfun(start_element, 1);
	getfun(end_element, 1);
	getfun(text, 1);
	getfun(passthrough, 1);
	getfun(error, 1);
	/* to support push/pop, end_element and error always present */
	parser.end_element = gmp_end_element;
	parser.error = gmp_error;
	lua_setuservalue(L, -2);
	lua_getfield(L, 1, "treat_cdata_as_text");
	if(lua_toboolean(L, -1))
	    fl |= G_MARKUP_TREAT_CDATA_AS_TEXT;
	lua_getfield(L, 1, "prefix_error_position");
	if(lua_toboolean(L, -1))
	    fl |= G_MARKUP_PREFIX_ERROR_POSITION;
	lua_pop(L, 2);
	st->ctx = g_markup_parse_context_new(&parser, fl, L, NULL);
	return 1;
    }
}

/***
@type markup_parse_context
*/

/***
Finish parsing.
@function markup_parse_context:end_parse
Call this after all data has been passed to the parser to finish parsing.
@treturn boolean True if successful.
@treturn string Error message if unsuccessful.
*/
static int markup_end_parse(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, markup_parse_state);
    lua_pushboolean(L, g_markup_parse_context_end_parse(st->ctx, &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Obtain current position in source text.
@function markup_parse_context:get_position
@treturn number The line number
@treturn number The column number
*/
static int markup_get_position(lua_State *L)
{
    gint l, c;
    get_udata(L, 1, st, markup_parse_state);
    g_markup_parse_context_get_position(st->ctx, &l, &c);
    lua_pushinteger(L, l);
    lua_pushinteger(L, c);
    return 2;
}

/***
Obtain the name of the current element being processed.
@function markup_parse_context:get_element
@treturn string The element name.
*/
static int markup_get_element(lua_State *L)
{
    get_udata(L, 1, st, markup_parse_state);
    lua_pushstring(L, g_markup_parse_context_get_element(st->ctx));
    return 1;
}

/***
Obtain the complete path of element names to the current element being
processed.
@function markup_parse_context:get_element_stack
@treturn {string,...} The element names, starting with the most deeply nested.
*/
static int markup_get_element_stack(lua_State *L)
{
    guint nel;
    const GSList *els, *p;
    int i;
    get_udata(L, 1, st, markup_parse_state);
    els = g_markup_parse_context_get_element_stack(st->ctx);
    nel = g_slist_length((GSList *)els); /* why would els need to be writable??*/
    lua_createtable(L, nel, 0);
    for(p = els, i = 1; p; p = p->next, ++i) {
	lua_pushstring(L, p->data);
	lua_rawseti(L, -2, i);
    }
    return 1;
}

/***
Parse some text.
@function markup_parse_context:parse
Call this with chunks of text, in order, as often as needed to process
entire text.
@tparam string s The next chunk of text to process
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int markup_parse(lua_State *L)
{
    GError *err = NULL;
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);
    get_udata(L, 1, st, markup_parse_state);
    lua_pushboolean(L, g_markup_parse_context_parse(st->ctx, s, len, &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Push the parse context.
@function markup_parse_context:push
This may only be called from *start\_element* handler. It creates a new
parser just for the text from the started element to its end tag.  When
the end tag is found, the previous parser is restored, and the
*end\_element* handler receives one additional argument:  the *pop* parameter
to this function.
@tparam {string=value,...} options Context creation options:

   * **start\_element**: a function like `_gmarkup_start_element_`.
   * **end\_element**: a function like `_gmarkup_end_element_`.
   * **text**: a function like `_gmarkup_text_`.
   * **passthrough**: a function like `_gmarkup_passthrough_`.
   * **error**: a function like `_gmarkup_error_`.
   * **pop**: a value to pass to the parent's *end\_element* handler when finished.
     It may be any value, but a function is probably most suitable.
     There is no guarantee that this value will ever be used, since it
     requires that no errors occur and that the parent's *end\_element*
     handler actually uses it.
@see _gmarkup_start_element_
@see _gmarkup_end_element_
@see _gmarkup_text_
@see _gmarkup_passthrough_
@see _gmarkup_error_
*/
static int markup_push(lua_State *L)
{
    GMarkupParser parser;
    markup_parse_state *p;
    get_udata(L, 1, st, markup_parse_state);

    luaL_checktype(L, 2, LUA_TTABLE);
    memset(&parser, 0, sizeof(parser));
    lua_newtable(L);
    getfun(start_element, 2);
    getfun(end_element, 2);
    getfun(text, 2);
    getfun(passthrough, 2);
    getfun(error, 2);
    /* to support push/pop, end_element and error always present */
    parser.end_element = gmp_end_element;
    parser.error = gmp_error;
    lua_getfield(L, 2, "pop");
    lua_setfield(L, -2, "pop");
    lua_getuservalue(L, 1);
    lua_setfield(L, -2, "next");
    lua_setuservalue(L, 1);
    for(p = st; p->next; p = p->next);
    p->next = g_malloc(sizeof(*p));
    p = p->next;
    memcpy(p, st, sizeof(*p));
    p->next = NULL;
    g_markup_parse_context_push(st->ctx, &parser, p);
    return 0;
}

static int free_markup_parse_state(lua_State *L)
{
    markup_parse_state *p;
    get_udata(L, 1, st, markup_parse_state);
    while(st->next) {
	p = st->next;
	st->next = p->next;
	g_free(p);
    }
    if(st->ctx) {
	g_markup_parse_context_free(st->ctx);
	st->ctx = NULL;
    }
    return 0;
}

static luaL_Reg markup_parse_state_funcs[] = {
    {"end_parse", markup_end_parse},
    {"get_position", markup_get_position},
    {"get_element", markup_get_element},
    {"get_element_statck", markup_get_element_stack},
    {"parse", markup_parse},
    {"push", markup_push},
    {"__gc", free_markup_parse_state},
    {NULL, NULL}
};

/*********************************************************************/
/***
Key-value file parser.
@section Key-value file parser
*/

typedef struct key_file_state {
    GKeyFile *kf;
} key_file_state;

/***
Create a new, empty key file.
@function key_file_new
@treturn key_file The empty key file
*/
static int glib_key_file_new(lua_State *L)
{
    alloc_udata(L, st, key_file_state);
    st->kf = g_key_file_new();
    return 1;
}

/***
@type key_file
*/

static int free_key_file_state(lua_State *L)
{
    get_udata(L, 1, st, key_file_state);
    if(st->kf) {
	g_key_file_free(st->kf);
	st->kf = NULL;
    }
    return 0;
}

/***
Sets character to separate values in lists.
@function key_file:set_list_separator
@tparam string sep The separator (only the first byte is used).
*/
static int key_file_set_list_separator(lua_State *L)
{
    const char *s = luaL_checkstring(L, 2);
    get_udata(L, 1, st, key_file_state);
    g_key_file_set_list_separator(st->kf, *s);
    return 0;
}

/***
Load a file.
@function key_file:load_from_file
@tparam string f File name
@tparam boolean|{string,...} dirs (Optional) If true, search relative to
 standard configuration file directories.  If a table, search realtive to
 all directories in the table.  Otherwise, the file name is absolute or
 relative to the current directory.
@tparam boolean keep_com (Optional) If true, keep comments so they are
 written by `key_file:to_data`.
@tparam boolean keep_trans (Optional) If true, keep all translations so they
 are written by `key_file:to_data`.
@treturn boolean True if successful
@treturn string Error message if unsuccessful, or the actual file name if
 a directory search was done.
*/
static int key_file_load_from_file(lua_State *L)
{
    GError *err = NULL;
    GKeyFileFlags fl = 0;
    gboolean do_search = lua_toboolean(L, 3);
    const gchar **search_path = NULL;
    gchar *ret = NULL;
    const char *s = luaL_checkstring(L, 2);
    get_udata(L, 1, st, key_file_state);

    if(lua_istable(L, 3)) {
	int len = lua_len(L, 3);
	search_path = g_malloc((len + 1) * sizeof(*search_path));
	search_path[len] = NULL;
	while(len > 0) {
	    lua_pushinteger(L, len);
	    lua_gettable(L, 3);
	    search_path[--len] = lua_tostring(L, -1);
	    lua_pop(L, 1);
	}
    }
    if(lua_toboolean(L, 4))
	fl |= G_KEY_FILE_KEEP_COMMENTS;
    if(lua_toboolean(L, 5))
	fl |= G_KEY_FILE_KEEP_TRANSLATIONS;
    if(!do_search)
	lua_pushboolean(L, g_key_file_load_from_file(st->kf, s, fl, &err));
    else if(search_path)
	lua_pushboolean(L, g_key_file_load_from_dirs(st->kf, s, search_path,
						     &ret, fl, &err));
    else
	lua_pushboolean(L, g_key_file_load_from_data_dirs(st->kf, s, &ret, fl,
							  &err));
    if(search_path)
	g_free(search_path);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    if(do_search) {
	lua_pushstring(L, ret);
	g_free(ret);
	return 2;
    }
    return 1;
}

/***
Load data.
@function key_file:load_from_data
@tparam string s The data
@tparam boolean keep_com (Optional) If true, keep comments so they are
 written by `key_file:to_data`.
@tparam boolean keep_trans (Optional) If true, keep all translations so they
 are written by `key_file:to_data`.
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int key_file_load_from_data(lua_State *L)
{
    GError *err = NULL;
    GKeyFileFlags fl = 0;
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);
    get_udata(L, 1, st, key_file_state);

    if(lua_toboolean(L, 3))
	fl |= G_KEY_FILE_KEEP_COMMENTS;
    if(lua_toboolean(L, 4))
	fl |= G_KEY_FILE_KEEP_TRANSLATIONS;
    lua_pushboolean(L, g_key_file_load_from_data(st->kf, s, len, fl, &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Convert entire key file to a string.
@function key_file:to_data
@treturn string|nil The key file contents, if successful
@treturn string An error message if unsuccessful
*/
static int key_file_to_data(lua_State *L)
{
    GError *err = NULL;
    gsize len;
    gchar *ret;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_to_data(st->kf, &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	return 2;
    }
    lua_pushlstring(L, ret, len);
    g_free(ret);
    return 1;
}

/***
Get the start group of the key file.
@function key_file:get_start_group
@treturn string The name of the start group
*/
static int key_file_get_start_group(lua_State *L)
{
    gchar *ret;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_start_group(st->kf);
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Get the names of all groups in the key file.
@function key_file:get_groups
@treturn {string,...} A table containing the names of all the groups.
*/
static int key_file_get_groups(lua_State *L)
{
    gchar **ret;
    gsize len;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_groups(st->kf, &len);
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushstring(L, ret[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_strfreev(ret);
    return 1;
}

/***
Get the names of all keys in a group.
@function key_file:get_keys
@tparam string group The name of a group to query
@treturn {string,...}|nil A table containing the names of all keys in the
 *group*, or nil if unsuccessful
@treturn string An error message if unsuccessful
*/
static int key_file_get_keys(lua_State *L)
{
    GError *err = NULL;
    gchar **ret;
    gsize len;
    const char *gr = luaL_checkstring(L, 2);
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_keys(st->kf, gr, &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushstring(L, ret[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_strfreev(ret);
    return 1;
}

/***
Check if group exists.
@function key_file:has_group
@tparam string group The group name
@treturn boolean True if group exists in key file.
*/
static int key_file_has_group(lua_State *L)
{
    get_udata(L, 1, st, key_file_state);
    lua_pushboolean(L, g_key_file_has_group(st->kf, luaL_checkstring(L, 2)));
    return 1;
}

/***
Check if key exists.
@function key_file:has_key
@tparam string group The group name
@tparam string key The key name
@treturn boolean True if key exists in key file.
@treturn string An error message if an error occurred
*/
/* glib manual says "use ...get_value() to test", but ...get_value() does
   an unnecessary malloc, so that seems pointless. */
static int key_file_has_key(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, key_file_state);
    lua_pushboolean(L, g_key_file_has_key(st->kf, luaL_checkstring(L, 2),
					  luaL_checkstring(L, 3), &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Obtain raw value of a key.
@function key_file:raw_get
@tparam string group The group name
@tparam string key The key name
@treturn string|nil The value, or nil on error
@treturn string An error message on error
*/
static int key_file_raw_get(lua_State *L)
{
    GError *err = NULL;
    char *ret;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_value(st->kf, luaL_checkstring(L, 2),
			       luaL_checkstring(L, 3), &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Obtain value of a key.
@function key_file:get
@tparam string group The group name
@tparam string key The key
@tparam string locale (Optional) If specified, get value translated into this
 locale, if available
@treturn string|nil The parsed UTF-8 value, or nil on error
@treturn string An error message on error
*/
static int key_file_get(lua_State *L)
{
    GError *err = NULL;
    char *ret;
    const char *locale = lua_isstring(L, 4) ? lua_tostring(L, 4) : NULL;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_locale_string(st->kf, luaL_checkstring(L, 2),
				       luaL_checkstring(L, 3), locale, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Obtain value of a boolean key.
@function key_file:get_boolean
@tparam string group The group name
@tparam string key The key
@treturn boolean The value, or false on error
@treturn string An error message on error
*/
static int key_file_get_boolean(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, key_file_state);
    lua_pushboolean(L, g_key_file_get_boolean(st->kf,
					      luaL_checkstring(L, 2),
					      luaL_checkstring(L, 3),
					      &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Obtain value of a numeric key.
@function key_file:get_number
@tparam string group The group name
@tparam string key The key
@treturn number|nil The value, or nil on error
@treturn string An error message on error
*/
static int key_file_get_number(lua_State *L)
{
    GError *err = NULL;
    gdouble val;
    get_udata(L, 1, st, key_file_state);
    val = g_key_file_get_double(st->kf, luaL_checkstring(L, 2),
				luaL_checkstring(L, 3), &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushnumber(L, val);
    return 1;
}

/***
Obtain value of a string list key.
@function: key_file:get_list
@tparam string group The group name
@tparam string key The key
@tparam string locale (Optional) If specified, get value translated into this
 locale, if available
@treturn {string,...}|nil The list of parsed UTF-8 values, or nil on error.
@treturn string An error message on error.
*/
static int key_file_get_list(lua_State *L)
{
    GError *err = NULL;
    gsize len;
    char **ret;
    const char *locale = lua_isstring(L, 4) ? lua_tostring(L, 4) : NULL;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_locale_string_list(st->kf, luaL_checkstring(L, 2),
					    luaL_checkstring(L, 3), locale,
					    &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushstring(L, ret[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_strfreev(ret);
    return 1;
}

/***
Obtain value of a boolean list key.
@function: key_file:get_boolean_list
@tparam string group The group name
@tparam string key The key
@treturn {boolean,...}|nil The list of values, or nil on error.
@treturn string An error message on error.
*/
static int key_file_get_boolean_list(lua_State *L)
{
    GError *err = NULL;
    gsize len;
    gboolean *ret;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_boolean_list(st->kf, luaL_checkstring(L, 2),
				      luaL_checkstring(L, 3), &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushboolean(L, ret[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_free(ret);
    return 1;
}

/***
Obtain value of a number list key.
@function: key_file:get_boolean_list
@tparam string group The group name
@tparam string key The key
@treturn {number,...}|nil The list of values, or nil on error.
@treturn string An error message on error.
*/
static int key_file_get_number_list(lua_State *L)
{
    GError *err = NULL;
    gsize len;
    gdouble *ret;
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_double_list(st->kf, luaL_checkstring(L, 2),
				     luaL_checkstring(L, 3), &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushboolean(L, ret[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_free(ret);
    return 1;
}

/***
Obtain comment above a key or group.
@function key_file:get_comment
@tparam string group (Optional) The group name; if unspecified, the first
 group is used, and *key* is ignored.
@tparam string key (Optional) The key name; if specified, obtain comment
 above the key; otherwise, obtain comment above group
@treturn string|nil The comment string, or nil on error
@treturn string An error message on error
*/
static int key_file_get_comment(lua_State *L)
{
    GError *err = NULL;
    char *ret;
    const char *gr = lua_isnoneornil(L, 2) ? NULL : luaL_checkstring(L, 2);
    const char *key = !gr || lua_isnoneornil(L, 3) ? NULL : luaL_checkstring(L, 3);
    get_udata(L, 1, st, key_file_state);
    ret = g_key_file_get_comment(st->kf, gr, key, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, ret);
    g_free(ret);
    return 1;
}

/***
Set raw value of a key.
@function key_file:raw_set
@tparam string group The group name
@tparam string key The key name
@tparam string value The value
*/
static int key_file_raw_set(lua_State *L)
{
    get_udata(L, 1, st, key_file_state);
    g_key_file_set_value(st->kf, luaL_checkstring(L, 2),
			 luaL_checkstring(L, 3), luaL_checkstring(L, 4));
    return 0;
}

/***
Set value of a key.
@function key_file:set
@tparam string group The group name
@tparam string key The key
@tparam string value The value
@tparam string|boolean locale (Optional) If a string, set value translated
 into this locale.  Otherwise, if true, set value translated into current
 locale.
*/
static int key_file_set(lua_State *L)
{
    gboolean has_locale = lua_toboolean(L, 5);
    const char *locale = lua_isstring(L, 5) ? lua_tostring(L, 5) : NULL;
    get_udata(L, 1, st, key_file_state);
    if(has_locale)
	g_key_file_set_locale_string(st->kf, luaL_checkstring(L, 2),
				     luaL_checkstring(L, 3), locale,
				     luaL_checkstring(L, 4));
    else
	g_key_file_set_string(st->kf, luaL_checkstring(L, 2),
			      luaL_checkstring(L, 3), luaL_checkstring(L, 4));
    return 0;
}

/***
Set value of a boolean key.
@function key_file:set_boolean
@tparam string group The group name
@tparam string key The key
@tparam boolean value The value
*/
static int key_file_set_boolean(lua_State *L)
{
    get_udata(L, 1, st, key_file_state);
    g_key_file_set_boolean(st->kf, luaL_checkstring(L, 2),
			   luaL_checkstring(L, 3), lua_toboolean(L, 4));
    return 0;
}

/***
Set value of a numeric key.
@function key_file:set_number
@tparam string group The group name
@tparam string key The key
@tparam number value The value
*/
static int key_file_set_number(lua_State *L)
{
    get_udata(L, 1, st, key_file_state);
    g_key_file_set_double(st->kf, luaL_checkstring(L, 2),
			  luaL_checkstring(L, 3), luaL_checknumber(L, 4));
    return 0;
}

/***
Set value of a string list key.
@function: key_file:set_list
@tparam string group The group name
@tparam string key The key
@tparam {string,...} value The value
@tparam string|boolean locale (Optional) If a string, set value translated
 into this locale.  Otherwise, if true, set value translated into current
 locale.
*/
static int key_file_set_list(lua_State *L)
{
    const char *gr = luaL_checkstring(L, 2);
    const char *key = luaL_checkstring(L, 3);
    gsize len, i;
    const char **strs;
    gboolean has_locale = lua_toboolean(L, 5);
    const char *locale = lua_isstring(L, 5) ? lua_tostring(L, 5) : NULL;
    get_udata(L, 1, st, key_file_state);
    luaL_checktype(L, 4, LUA_TTABLE);
    len = lua_len(L, 4);
    strs = g_malloc((len + 1) * sizeof(*strs));
    strs[len] = NULL;
    for(i = 0; i < len; i++) {
	lua_pushinteger(L, i + 1);
	lua_gettable(L, 4);
	strs[i] = lua_tostring(L, -1);
	lua_pop(L, 1);
    }
    if(has_locale)
	g_key_file_set_locale_string_list(st->kf, gr, key, locale, strs, len);
    else
	g_key_file_set_string_list(st->kf, gr, key, strs, len);
    g_free(strs);
    return 0;
}

/***
Set value of a boolean list key.
@function: key_file:set_boolean_list
@tparam string group The group name
@tparam string key The key
@tparam {boolean,...} value The value
*/
static int key_file_set_boolean_list(lua_State *L)
{
    const char *gr = luaL_checkstring(L, 2);
    const char *key = luaL_checkstring(L, 3);
    gsize len, i;
    gboolean *bools;
    get_udata(L, 1, st, key_file_state);
    luaL_checktype(L, 4, LUA_TTABLE);
    len = lua_len(L, 4);
    bools = g_malloc(len * sizeof(*bools));
    for(i = 0; i < len; i++) {
	lua_pushinteger(L, i + 1);
	lua_gettable(L, 4);
	bools[i] = lua_toboolean(L, -1);
	lua_pop(L, 1);
    }
    g_key_file_set_boolean_list(st->kf, gr, key, bools, len);
    g_free(bools);
    return 0;
}

/***
Set value of a number list key.
@function: key_file:set_boolean_list
@tparam string group The group name
@tparam string key The key
*/
static int key_file_set_number_list(lua_State *L)
{
    const char *gr = luaL_checkstring(L, 2);
    const char *key = luaL_checkstring(L, 3);
    gsize len, i;
    gdouble *vals;
    get_udata(L, 1, st, key_file_state);
    luaL_checktype(L, 4, LUA_TTABLE);
    len = lua_len(L, 4);
    vals = g_malloc(len * sizeof(*vals));
    for(i = 0; i < len; i++) {
	lua_pushinteger(L, i + 1);
	lua_gettable(L, 4);
	vals[i] = lua_tonumber(L, -1);
	lua_pop(L, 1);
    }
    g_key_file_set_double_list(st->kf, gr, key, vals, len);
    g_free(vals);
    return 0;
}

/***
Set comment above a key or group.
@function key_file:set_comment
@tparam string comment The comment
@tparam string group (Optional) The group name; if unspecified, the first
 group is used, and *key* is ignored.
@tparam string key (Optional) The key name; if specified, obtain comment
 above the key; otherwise, obtain comment above group
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int key_file_set_comment(lua_State *L)
{
    GError *err = NULL;
    const char *gr = lua_isnoneornil(L, 3) ? NULL : luaL_checkstring(L, 3);
    const char *key = !gr || lua_isnoneornil(L, 4) ? NULL : luaL_checkstring(L, 4);
    get_udata(L, 1, st, key_file_state);
    lua_pushboolean(L, g_key_file_set_comment(st->kf, gr, key,
					      luaL_checkstring(L, 2), &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Remove a group or key.
@function key_file:remove
@tparam string group The group
@tparam string key (Optional) The key.  If specified, remove that key.
 Otherwise, remove the group *group*.
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int key_file_remove(lua_State *L)
{
    GError *err = NULL;
    const char *gr = luaL_checkstring(L, 2);
    const char *key = lua_isnoneornil(L, 3) ? NULL : luaL_checkstring(L, 3);
    get_udata(L, 1, st, key_file_state);
    if(key)
	lua_pushboolean(L, g_key_file_remove_key(st->kf, gr, key, &err));
    else
	lua_pushboolean(L, g_key_file_remove_group(st->kf, gr, &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Remove comment above a key or group.
@function key_file:remove_comment
@tparam string group (Optional) The group name; if unspecified, the first
 group is used, and *key* is ignored.
@tparam string key (Optional) The key name; if specified, obtain comment
 above the key; otherwise, obtain comment above group
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int key_file_remove_comment(lua_State *L)
{
    GError *err = NULL;
    const char *gr = lua_isnoneornil(L, 2) ? NULL : luaL_checkstring(L, 2);
    const char *key = !gr || lua_isnoneornil(L, 3) ? NULL : luaL_checkstring(L, 3);
    get_udata(L, 1, st, key_file_state);
    lua_pushboolean(L, g_key_file_remove_comment(st->kf, gr, key, &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

static luaL_Reg key_file_state_funcs[] = {
    {"set_list_separator", key_file_set_list_separator},
    {"load_from_file", key_file_load_from_file},
    {"load_from_data", key_file_load_from_data},
    {"to_data", key_file_to_data},
    {"get_start_group", key_file_get_start_group},
    {"get_groups", key_file_get_groups},
    {"get_keys", key_file_get_keys},
    {"has_group", key_file_has_group},
    {"has_key", key_file_has_key},
    {"raw_get", key_file_raw_get},
    {"get", key_file_get},
    {"get_boolean", key_file_get_boolean},
    {"get_number", key_file_get_number},
    {"get_list", key_file_get_list},
    {"get_boolean_list", key_file_get_boolean_list},
    {"get_number_list", key_file_get_number_list},
    {"get_comment", key_file_get_comment},
    {"raw_set", key_file_raw_set},
    {"set", key_file_set},
    {"set_boolean", key_file_set_boolean},
    {"set_number", key_file_set_number},
    {"set_list", key_file_set_list},
    {"set_boolean_list", key_file_set_boolean_list},
    {"set_number_list", key_file_set_number_list},
    {"set_comment", key_file_set_comment},
    {"remove", key_file_remove},
    {"remove_comment", key_file_remove_comment},
    {"__gc", free_key_file_state},
    {NULL, NULL}
};


/*********************************************************************/
/***
Bookmark file parser.
@section bookmark file parser
*/

typedef struct bookmark_file_state {
    GBookmarkFile *bmf;
} bookmark_file_state;

/***
Create a new, empty bookmark file.
@function bookmark_file_new
@treturn bookmark_file The empty bookmark file.
*/
static int glib_bookmark_file_new(lua_State *L)
{
    alloc_udata(L, st, bookmark_file_state);
    st->bmf = g_bookmark_file_new();
    return 1;
}

/***
@type bookmark_file
*/

static int free_bookmark_file_state(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    if(st->bmf) {
	g_bookmark_file_free(st->bmf);
	st->bmf = NULL;
    }
    return 0;
}

/***
Load a file.
@function bookmark_file:load_from_file
@tparam string f File name
@tparam boolean use_dirs (Optional) If true, search relative to
 standard configuration file directories.
@treturn boolean True if successful
@treturn string Error message if unsuccessful, or the actual file name if
 a directory search was done.
*/
static int bookmark_file_load_from_file(lua_State *L)
{
    GError *err = NULL;
    gboolean do_search = lua_toboolean(L, 3);
    gchar *ret = NULL;
    const char *s = luaL_checkstring(L, 2);
    get_udata(L, 1, st, bookmark_file_state);

    if(!do_search)
	lua_pushboolean(L, g_bookmark_file_load_from_file(st->bmf, s, &err));
    else
	lua_pushboolean(L, g_bookmark_file_load_from_data_dirs(st->bmf, s, &ret,
							  &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    if(do_search) {
	lua_pushstring(L, ret);
	g_free(ret);
	return 2;
    }
    return 1;
}

/***
Load data.
@function bookmark_file:load_from_data
@tparam string s The data
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_load_from_data(lua_State *L)
{
    GError *err = NULL;
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);
    get_udata(L, 1, st, bookmark_file_state);

    lua_pushboolean(L, g_bookmark_file_load_from_data(st->bmf, s, len, &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Convert bookmark file to a string.
@function bookmark_file:to_data
@treturn string|nil The bookmark file contents, if successful
@treturn string An error message if unsuccessful
*/
static int bookmark_file_to_data(lua_State *L)
{
    GError *err = NULL;
    gsize len;
    gchar *ret;
    get_udata(L, 1, st, bookmark_file_state);
    ret = g_bookmark_file_to_data(st->bmf, &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	return 2;
    }
    lua_pushlstring(L, ret, len);
    g_free(ret);
    return 1;
}

/***
Write bookmark file to a file.
@function bookmark_file:to_file
@tparam string f The file name
@treturn boolean True if successful
@treturn string An error message if unsuccessful
*/
static int bookmark_file_to_file(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    lua_pushboolean(L, g_bookmark_file_to_file(st->bmf, luaL_checkstring(L, 2),
					       &err));
    if(err) {
	lua_pushstring(L, err->message);
	return 2;
    }
    return 1;
}

/***
Check if bookmark file has given URI.
@function bookmark_file:has_item
@tparam string uri The URI to find
@treturn boolean True if present
*/
static int bookmark_file_has_item(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    lua_pushboolean(L, g_bookmark_file_has_item(st->bmf, luaL_checkstring(L, 2)));
    return 1;
}

/***
Check if bookmark file has given URI in a given group.
@function bookmark_file:has_group
@tparam string uri The URI to find
@tparam string group The group to find
@treturn boolean True if present in group
@treturn string Error message if error
*/
static int bookmark_file_has_group(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    lua_pushboolean(L, g_bookmark_file_has_group(st->bmf,
						 luaL_checkstring(L, 2),
						 luaL_checkstring(L, 3),
						 &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Check if bookmark file has given URI registered by a given application.
@function bookmark_file:has_application
@tparam string uri The URI to find
@tparam string app The application
@treturn boolean True if registed by application
*/
static int bookmark_file_has_application(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    lua_pushboolean(L, g_bookmark_file_has_application(st->bmf,
						       luaL_checkstring(L, 2),
						       luaL_checkstring(L, 3),
						       &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Get the number of bookmarks.
This is the standard Lua # operator, applied to a bookmark file.
@function bookmark_file:__len
@treturn number The number of bookmarks.
*/
static int bookmark_file_len(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    lua_pushnumber(L, g_bookmark_file_get_size(st->bmf));
    return 1;
}

/***
Get all URIs.
@function bookmark_file:uris
@treturn {string,...} All URIs.
*/
static int bookmark_file_uris(lua_State *L)
{
    gsize len;
    gchar **res;
    get_udata(L, 1, st, bookmark_file_state);
    res = g_bookmark_file_get_uris(st->bmf, &len);
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushstring(L, res[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_strfreev(res);
    return 1;
}

/***
Get title for URI
@function bookmark_file:title
@tparam string uri The URI
@treturn string|nil Its title if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_title(lua_State *L)
{
    GError *err = NULL;
    char *res;
    get_udata(L, 1, st, bookmark_file_state);
    res = g_bookmark_file_get_title(st->bmf, luaL_checkstring(L, 2), &err);
    lua_pushstring(L, res);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    g_free(res);
    return 1;
}

/***
Get description for URI
@function bookmark_file:description
@tparam string uri The URI
@treturn string|nil Its description if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_description(lua_State *L)
{
    GError *err = NULL;
    char *res;
    get_udata(L, 1, st, bookmark_file_state);
    res = g_bookmark_file_get_description(st->bmf, luaL_checkstring(L, 2), &err);
    lua_pushstring(L, res);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    g_free(res);
    return 1;
}

/***
Get MIME type for URI
@function bookmark_file:mime_type
@tparam string uri The URI
@treturn string|nil Its MIME type if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_mime_type(lua_State *L)
{
    GError *err = NULL;
    char *res;
    get_udata(L, 1, st, bookmark_file_state);
    res = g_bookmark_file_get_mime_type(st->bmf, luaL_checkstring(L, 2), &err);
    lua_pushstring(L, res);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    g_free(res);
    return 1;
}

/***
Get private flag for URI
@function bookmark_file:is_private
@tparam string uri The URI
@treturn boolean True if private flag set
@treturn string Error message on error
*/
static int bookmark_file_is_private(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    lua_pushboolean(L, g_bookmark_file_get_is_private(st->bmf,
						      luaL_checkstring(L, 2),
						      &err));
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Get icon for URI.
@function bookmark_file:icon
@tparam string uri The URI
@treturn string|nil The icon URL if successful
@treturn string The icon's MIME type if successful, or an error message if
 unsuccessful.
*/
static int bookmark_file_icon(lua_State *L)
{
    GError *err = NULL;
    char *res, *mt;
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_get_icon(st->bmf, luaL_checkstring(L, 2), &res, &mt, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, res);
    g_free(res);
    lua_pushstring(L, mt);
    g_free(mt);
    return 1;
}

/***
Get time URI was added.
@function bookmark_file:added
@tparam string uri The URI
@treturn number|nil The time stamp, as seconds from epoch, if successful.
@treturn string An error message if unsuccessful.
*/
static int bookmark_file_added(lua_State *L)
{
    GError *err = NULL;
    time_t t;
    get_udata(L, 1, st, bookmark_file_state);
    t = g_bookmark_file_get_added(st->bmf, luaL_checkstring(L, 2), &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushnumber(L, t);
    return 1;
}

/***
Get time URI was last modified.
@function bookmark_file:modified
@tparam string uri The URI
@treturn number|nil The time stamp, as seconds from epoch, if successful.
@treturn string An error message if unsuccessful.
*/
static int bookmark_file_modified(lua_State *L)
{
    GError *err = NULL;
    time_t t;
    get_udata(L, 1, st, bookmark_file_state);
    t = g_bookmark_file_get_modified(st->bmf, luaL_checkstring(L, 2), &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushnumber(L, t);
    return 1;
}

/***
Get time URI was last visited.
@function bookmark_file:visited
@tparam string uri The URI
@treturn number|nil The time stamp, as seconds from epoch, if successful.
@treturn string An error message if unsuccessful.
*/
static int bookmark_file_visited(lua_State *L)
{
    GError *err = NULL;
    time_t t;
    get_udata(L, 1, st, bookmark_file_state);
    t = g_bookmark_file_get_visited(st->bmf, luaL_checkstring(L, 2), &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushnumber(L, t);
    return 1;
}

/***
Get list of groups to which URI belongs.
@function bookmark_file:groups
@tparam string uri The URI
@treturn {string,...}|nil The list of groups, if successful
@treturn string An error message, if unsuccessful
*/
static int bookmark_file_groups(lua_State *L)
{
    GError *err = NULL;
    gsize len;
    gchar **res;
    get_udata(L, 1, st, bookmark_file_state);
    res = g_bookmark_file_get_groups(st->bmf, luaL_checkstring(L, 2), &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushstring(L, res[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_strfreev(res);
    return 1;
}

/***
Get list of applications which registered this URI.
@function bookmark_file:applications
@tparam string uri The URI
@treturn {string,...}|nil The list of applications, if successful
@treturn string An error message, if unsuccessful
*/
static int bookmark_file_applications(lua_State *L)
{
    GError *err = NULL;
    gsize len;
    gchar **res;
    get_udata(L, 1, st, bookmark_file_state);
    res = g_bookmark_file_get_applications(st->bmf, luaL_checkstring(L, 2),
					   &len, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_createtable(L, len, 0);
    while(len > 0) {
	lua_pushstring(L, res[len - 1]);
	lua_rawseti(L, -2, len);
	--len;
    }
    g_strfreev(res);
    return 1;
}

/***
Obtain registration information for application which registered URI.
@function boomark_file:app_info
@tparam string uri The URI
@tparam string app Application name
@treturn string|nil The command to invoke *app* on *uri* (or nil on error)
@treturn number|string The number of times *app* registered *uri* (or
 error message on error)
@treturn number|nil The last time *app* registered *uri* (or not present on
 error)
*/
static int bookmark_file_app_info(lua_State *L)
{
    GError *err = NULL;
    time_t t;
    guint count;
    gchar *exec;
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_get_app_info(st->bmf, luaL_checkstring(L, 2),
				 luaL_checkstring(L, 3), &exec, &count,
				 &t, &err);
    if(err) {
	lua_pushnil(L);
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    lua_pushstring(L, exec);
    g_free(exec);
    lua_pushnumber(L, count);
    lua_pushnumber(L, t);
    return 1;
}

/***
Set title for URI
@function bookmark_file:set_title
@tparam string uri (Optional) The URI.  If nil, the title of the bookmark
 file is set.
@tparam string title The new title
*/
static int bookmark_file_set_title(lua_State *L)
{
    const char *uri = lua_isnoneornil(L, 2) ? NULL : luaL_checkstring(L, 2);
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_title(st->bmf, uri, luaL_checkstring(L, 3));
    return 0;
}

/***
Set description for URI
@function bookmark_file:set_description
@tparam string uri (Optional) The URI.  If nil, the title of the bookmark
 file is set.
@tparam string desc The new description
*/
static int bookmark_file_set_description(lua_State *L)
{
    const char *uri = lua_isnoneornil(L, 2) ? NULL : luaL_checkstring(L, 2);
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_description(st->bmf, uri, luaL_checkstring(L, 3));
    return 0;
}

/***
Set MIME type for URI
@function bookmark_file:set_mime_type
@tparam string uri The URI
@tparam string mime_type The new MIME type
*/
static int bookmark_file_set_mime_type(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_mime_type(st->bmf, luaL_checkstring(L, 2),
				  luaL_checkstring(L, 3));
    return 0;
}

/***
Set private flag for URI
@function bookmark_file:set_is_private
@tparam string uri The URI
@tparam boolean private The new flag
*/
static int bookmark_file_set_is_private(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_is_private(st->bmf, luaL_checkstring(L, 2),
				   lua_toboolean(L, 3));
    return 0;
}

/***
Set icon for URI.
@function bookmark_file:set_icon
@tparam string uri The URI
@tparam string icon The icon URL
@tparam string mime_type The icon's MIME type
*/
static int bookmark_file_set_icon(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_icon(st->bmf, luaL_checkstring(L, 2),
			     luaL_checkstring(L, 3), luaL_checkstring(L, 4));
    return 0;
}

/***
Set time URI was added.
@function bookmark_file:set_added
@tparam string uri The URI
@tparam number time The time stamp, as seconds from epoch
*/
static int bookmark_file_set_added(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_added(st->bmf, luaL_checkstring(L, 2),
			      luaL_checknumber(L, 3));
    return 0;
}

/***
Set time URI was modified.
@function bookmark_file:set_modified
@tparam string uri The URI
@tparam number time The time stamp, as seconds from epoch
*/
static int bookmark_file_set_modified(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_modified(st->bmf, luaL_checkstring(L, 2),
			      luaL_checknumber(L, 3));
    return 0;
}

/***
Set time URI was visited.
@function bookmark_file:set_visited
@tparam string uri The URI
@tparam number time The time stamp, as seconds from epoch
*/
static int bookmark_file_set_visited(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_visited(st->bmf, luaL_checkstring(L, 2),
			      luaL_checknumber(L, 3));
    return 0;
}

/***
Set list of groups to which URI belongs.
@function bookmark_file:set_groups
@tparam string uri The URI
@treturn {string,...} The list of groups
*/
static int bookmark_file_set_groups(lua_State *L)
{
    gsize len;
    const gchar **gr;
    const char *uri = luaL_checkstring(L, 2);
    get_udata(L, 1, st, bookmark_file_state);
    luaL_checktype(L, 3, LUA_TTABLE);
    len = lua_len(L, 3);
    gr = g_malloc(len * sizeof(*gr));
    while(len > 0) {
	lua_pushinteger(L, len);
	lua_gettable(L, 3);
	gr[--len] = lua_tostring(L, -1);
	lua_pop(L, 1);
    }
    g_bookmark_file_set_groups(st->bmf, uri, gr, len);
    g_free(gr);
    return 0;
}

/***
Set registration information for application which registered URI.
@function boomark_file:set_app_info
@tparam string uri The URI
@tparam string app Application name
@tparam string exec The command to invoke *app* on *uri* (%f == file, %u == uri)
@tparam number rcount (Optional) The number of times *app* registered *uri*
 (absent, nil, or less than 0 to simply increment, or 0 to remove)
@tparam number stamp (Optional) The last time *app* registered *uri*
 (or -1, nil, or absent for current time)
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_set_app_info(lua_State *L)
{
    GError *err = NULL;
    gint rcount = lua_isnoneornil(L, 5) ? -1 : luaL_checknumber(L, 5);
    time_t stamp = lua_isnoneornil(L, 6) ? -1 : luaL_checknumber(L, 6);
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_set_app_info(st->bmf, luaL_checkstring(L, 2),
				 luaL_checkstring(L, 3),
				 luaL_checkstring(L, 4), rcount, stamp, &err);
    lua_pushboolean(L, !err);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Add a group to the list of groups URI belongs to.
@function bookmark_file:add_group
@tparam string uri The URI
@tparam string group The group
*/
static int bookmark_file_add_group(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_add_group(st->bmf, luaL_checkstring(L, 2),
			      luaL_checkstring(L, 3));
    return 0;
}

/***
Add an application to the list of applications that registered this URI.
@function bookmark_file:add_application
@tparam string uri The URI
@tparam string app The application name
@tparam string exec The command line to invoke application on URI (%f = file,
 %u = URI)
*/
static int bookmark_file_add_application(lua_State *L)
{
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_add_application(st->bmf, luaL_checkstring(L, 2),
				    luaL_checkstring(L, 3),
				    luaL_checkstring(L, 4));
    return 0;
}

/***
Remove a group from the list of groups URI belongs to.
@function bookmark_file:remove_group
@tparam string uri The URI
@tparam string group The group
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_remove_group(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_remove_group(st->bmf, luaL_checkstring(L, 2),
				 luaL_checkstring(L, 3), &err);
    lua_pushboolean(L, !err);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Remove an application from the list of applications that registered this URI.
@function bookmark_file:remove_application
@tparam string uri The URI
@tparam string app The application name
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_remove_application(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_remove_application(st->bmf, luaL_checkstring(L, 2),
				       luaL_checkstring(L, 3), &err);
    lua_pushboolean(L, !err);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Remove URI.
@function bookmark_file:remove
@tparam string uri The URI
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_remove(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_remove_item(st->bmf, luaL_checkstring(L, 2), &err);
    lua_pushboolean(L, !err);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

/***
Change URI, retaining group and application information.
@function bookmark_file:move
@tparam string uri The URI
@tparam string new The new URI
@treturn boolean True if successful
@treturn string Error message if unsuccessful
*/
static int bookmark_file_move(lua_State *L)
{
    GError *err = NULL;
    get_udata(L, 1, st, bookmark_file_state);
    g_bookmark_file_move_item(st->bmf, luaL_checkstring(L, 2),
			      luaL_checkstring(L, 3), &err);
    lua_pushboolean(L, !err);
    if(err) {
	lua_pushstring(L, err->message);
	g_error_free(err);
	return 2;
    }
    return 1;
}

static luaL_Reg bookmark_file_state_funcs[] = {
    {"load_from_file", bookmark_file_load_from_file},
    {"load_from_data", bookmark_file_load_from_data},
    {"to_data", bookmark_file_to_data},
    {"to_file", bookmark_file_to_file},
    {"has_item", bookmark_file_has_item},
    {"has_group", bookmark_file_has_group},
    {"has_application", bookmark_file_has_application},
    {"__len", bookmark_file_len},
    {"uris", bookmark_file_uris},
    {"title", bookmark_file_title},
    {"description", bookmark_file_description},
    {"mime_type", bookmark_file_mime_type},
    {"is_private", bookmark_file_is_private},
    {"icon", bookmark_file_icon},
    {"added", bookmark_file_added},
    {"modified", bookmark_file_modified},
    {"visited", bookmark_file_visited},
    {"groups", bookmark_file_groups},
    {"applications", bookmark_file_applications},
    {"app_info", bookmark_file_app_info},
    {"set_title", bookmark_file_set_title},
    {"set_description", bookmark_file_set_description},
    {"set_mime_type", bookmark_file_set_mime_type},
    {"set_is_private", bookmark_file_set_is_private},
    {"set_icon", bookmark_file_set_icon},
    {"set_added", bookmark_file_set_added},
    {"set_modified", bookmark_file_set_modified},
    {"set_visited", bookmark_file_set_visited},
    {"set_groups", bookmark_file_set_groups},
    {"set_app_info", bookmark_file_set_app_info},
    {"add_group", bookmark_file_add_group},
    {"add_application", bookmark_file_add_application},
    {"remove_group", bookmark_file_remove_group},
    {"remove_application", bookmark_file_remove_application},
    {"remove", bookmark_file_remove},
    {"move", bookmark_file_move},
    {"__gc", free_bookmark_file_state},
    {NULL, NULL}
};

/*********************************************************************/
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
    fent(file_get),
    fent(file_set),
    fent(is_dir),
    fent(is_file),
    fent(is_symlink),
    fent(is_exec),
    fent(exists),
    fent(umask),
    fent(mkstemp),
    fent(open_tmp),
    fent(read_link),
    fent(mkdir_with_parents),
    fent(mkdtemp),
    fent(dir_make_tmp),
    fent(dir),
    /* no support for mem-mapped files */
    /* no support for I/O descriptor matching utilities */
    /* there is no way to force glib's I/O descriptors to match lua's */
    fent(rename),
    fent(mkdir),
    fent(stat),
    fent(chmod),
    fent(remove),
    fent(can_read),
    fent(can_write),
    fent(chdir),
    fent(utime),
    /* URI Functions */
    fent(uri_parse_scheme),
    fent(uri_escape_string),
    fent(uri_unescape_string),
    fent(uri_list_extract_uris),
    fent(filename_from_uri),
    fent(filename_to_uri),
    /* Hostname Utilities */
    fent(hostname_to_ascii),
    fent(hostname_to_unicode),
    fent(hostname_is_non_ascii),
    fent(hostname_is_ascii_encoded),
    fent(hostname_is_ip_address),
    /* Shell-related Utilities */
    fent(shell_parse_argv),
    fent(shell_quote),
    fent(shell_unquote),
    /* No support for Commandline option parser */
    /* No support for Glob-style pattern matching */
    /* Perl-compatible regular expressions */
    fent(regex_new),
    fent(regex_escape_string),
    /* Simple XML Subset Parser */
    fent(markup_escape_text),
    fent(markup_parse_context_new),
    /* Key-value file parser */
    fent(key_file_new),
    /* Bookmark file parser */
    fent(bookmark_file_new),
    {NULL, NULL}
};

int luaopen_glib(lua_State *L)
{
    /* extra: version, os, dir_separator, searchpath_separator (4) */
    /*        uri_reserved_chars_* (5) */
    /*        key_file_desktop (1) */
    /* remove: NULL at end (1) */
    lua_createtable(L, 0, sizeof(lua_funcs)/sizeof(lua_funcs[0]) + 10 - 1);
    luaL_setfuncs(L, lua_funcs, 0);

    {
	char ver[80];
	snprintf(ver, 80, "%u.%u.%u", glib_major_version, glib_minor_version,
		 glib_micro_version);
	lua_pushstring(L, ver);
	lua_setfield(L, -2, "version");
    }
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
    lua_pushstring(L, G_SEARCHPATH_SEPARATOR_S);
    lua_setfield(L, -2, "searchpath_separator");
    /* no support for other standard macros */
    /* no support for Type Conversion Macros */
    /* no support for Byte Order Macros */
    /* no support for Numerical Definitions */
    /* no support for Miscellaneous Macros */
    /* see lua_funcs[] for rest */

    /* uri #defines */
    lua_pushliteral(L, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH);
    lua_setfield(L, -2, "uri_reserved_chars_allowed_in_path");
    lua_pushliteral(L, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH_ELEMENT);
    lua_setfield(L, -2, "uri_reserved_chars_allowed_in_path_element");
    lua_pushliteral(L, G_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO);
    lua_setfield(L, -2, "uri_reserved_chars_allowed_in_userinfo");
    lua_pushliteral(L, G_URI_RESERVED_CHARS_GENERIC_DELIMITERS);
    lua_setfield(L, -2, "uri_reserved_chars_generic_delimiters");
    lua_pushliteral(L, G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS);
    lua_setfield(L, -2, "uri_reserved_chars_subcomponent_delimiters");

    /* key_file #defines */
    lua_createtable(L, 0, 23);
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_GROUP);
    lua_setfield(L, -2, "group");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_TYPE);
    lua_setfield(L, -2, "key_type");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_VERSION);
    lua_setfield(L, -2, "key_version");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_NAME);
    lua_setfield(L, -2, "key_name");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME);
    lua_setfield(L, -2, "key_generic_name");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY);
    lua_setfield(L, -2, "key_no_display");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_COMMENT);
    lua_setfield(L, -2, "key_comment");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_ICON);
    lua_setfield(L, -2, "key_icon");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_HIDDEN);
    lua_setfield(L, -2, "key_hidden");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN);
    lua_setfield(L, -2, "key_only_show_in");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_NOT_SHOW_IN);
    lua_setfield(L, -2, "key_not_show_in");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_TRY_EXEC);
    lua_setfield(L, -2, "key_try_exec");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_EXEC);
    lua_setfield(L, -2, "key_exec");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_PATH);
    lua_setfield(L, -2, "key_path");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_TERMINAL);
    lua_setfield(L, -2, "key_terminal");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_MIME_TYPE);
    lua_setfield(L, -2, "key_mime_type");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_CATEGORIES);
    lua_setfield(L, -2, "key_categories");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY);
    lua_setfield(L, -2, "key_startup_notify");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_STARTUP_WM_CLASS);
    lua_setfield(L, -2, "key_startup_wm_class");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_KEY_URL);
    lua_setfield(L, -2, "key_url");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_TYPE_APPLICATION);
    lua_setfield(L, -2, "type_application");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_TYPE_LINK);
    lua_setfield(L, -2, "type_link");
    lua_pushliteral(L, G_KEY_FILE_DESKTOP_TYPE_DIRECTORY);
    lua_setfield(L, -2, "type_directory");
    lua_setfield(L, -2, "key_file_desktop");

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
    luaL_setfuncs(L, t##_funcs, 0); \
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
    newt_tab(regex_state);
    newt_free(regex_iter_state);
    newt_tab(markup_parse_state);
    newt_tab(key_file_state);
    newt_tab(bookmark_file_state);

    /* Internationalization */
    /* gettext macros are globals */
    lua_register(L, "_", glib_gettext);
    lua_register(L, "Q_", glib_dpgettext0);
    lua_register(L, "C_", glib_dpgettext4);
    lua_register(L, "N_", glib_ngettext);
    lua_register(L, "NC_", glib_ndpgettext4);

#if LUA_VERSION_NUM <= 501
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
#endif
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
