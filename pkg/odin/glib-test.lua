glib = require 'glib'

print("Version Information")
print(glib.version)
print("Standard Macros")
print(glib.os, glib.dir_separator, glib.searchpath_separator)
print("Message Logging")
glib.log("message")
glib.log("glib-test", "msg", "test message")
glib.log("crit", "Critical message")
glib.log("critical", "Critical message")
glib.log("debug", "Debug message")
-- glib.log("err", "Error message")
-- glib.log("error", "Error message")
glib.log("info", "Info message")
glib.log("message", "Message")
glib.log("msg", "Message")
glib.log("warn", "Warning message")
glib.log("warning", "Warning message")
print(pcall(glib.log, "z", "w"))
print("Character Set Conversion")
print(glib.convert('x', 'y', 'z'))
print(pcall(glib.convert, nil, nil, nil, 'blah'))
ss = "AsdáßðasD/./þüú¹²"
print(glib.convert(ss, 'ascii', 'utf-8', '<unk>'))
s = glib.convert(ss, "latin1", "utf-8")
print("convert", s ~= ss, ss == glib.convert(s, "utf-8", "latin1"))
f = glib.convert(nil, "latin1", "utf-8")
s2 = ''
for c in ss:gmatch('.') do
    s2 = s2 .. f(c)
end
s2 = s2 .. f()
f = glib.convert(nil, "utf-8", "latin1")
ss2 = ''
for c in s2:gmatch('.') do
    ss2 = ss2 .. f(c)
end
ss2 = ss2 .. f()
print("convert-s", s == s2, ss == ss2)
print("Unicode Manipulation")
print(glib.validate(0xd800), glib.validate(0x10f000))
print(glib.isalpha('a'), glib.xdigit_value('a'))
print(glib.type('a'), glib.type(0xd800))
print(glib.break_type('a'), glib.break_type(0xd800))
print(glib.get_mirror_char('('), glib.get_mirror_char(40))
print(glib.get_script('ç'))
print(glib.utf8_sub(ss, 2, 5), glib.utf8_len(ss), glib.utf8_validate(ss))
print(glib.utf8_validate(s))
print(glib.utf8_strup(ss), glib.utf8_strdown(ss), glib.utf8_casefold(ss))
print(glib.utf8_normalize(ss), #glib.utf8_normalize(ss), glib.utf8_normalize(ss, true), #glib.utf8_normalize(ss, true))
print(glib.utf8_normalize(ss, false, true), #glib.utf8_normalize(ss, false, true), glib.utf8_normalize(ss, true, true), #glib.utf8_normalize(ss, true, true))
print(glib.utf8_collate(ss, glib.utf8_normalize(ss)))
print(glib.utf8_collate_key(ss), glib.utf8_collate_key_for_filename(ss))
print(#ss, #glib.utf8_to_utf16(ss), #glib.utf8_to_ucs4(ss))
print(glib.ucs4_to_utf8(glib.utf16_to_ucs4(glib.utf8_to_utf16(ss))) == ss)
print(glib.to_utf8(41) == ')')
print("Base64 Encoding")
print(glib.base64_encode(ss), glib.base64_decode(glib.base64_encode(ss)) == ss)
f = glib.base64_encode()
s = glib.base64_encode(ss)
f2 = glib.base64_decode()
s2 = ''
ss2 = ''
for c in ss:gmatch('.') do
    co = f(c)
    s2 = s2 .. co
    ss2 = ss2 .. f2(co)
end
co = f()
s2 = s2 .. co
ss2 = ss2 .. f2(co)
ss2 = ss2 .. f2()
-- for some reason, s ~= s2.  However, ss2 == ss, so it's OK
print(ss2 == ss, s, s2)
print("Data Checksums")
print(glib.md5sum(ss), glib.sha1sum(ss), glib.sha256sum(ss))
print(#glib.sha256sum(ss, true))
s = glib.sha1sum(ss, true)
f = glib.sha1sum()
for c in ss:gmatch('.') do
    f(c)
end
print(f(true) == s)
f = glib.sha1sum()
for c in ss:gmatch('.') do
    f(c)
end
print(f())
print(f())
print("Secure HMAC Digests")
key = 'hello'
print(glib.md5hmac(key, ss), glib.sha1hmac(key, ss), glib.sha256hmac(key, ss))
print(#glib.sha256hmac(key, ss, true))
s = glib.sha1hmac(key, ss, true)
f = glib.sha1hmac(key)
for c in ss:gmatch('.') do
    f(c)
end
print(f(true) == s)
print(f())
print("Internationalization")
print(_("hello"), Q_("blah|hello"), C_("blah", "hello"))
print(N_("hello"), NC_("blah", "hello"))
for i, v in ipairs(glib.get_locale_variants()) do
    print(i, v)
end
for i, v in ipairs(glib.get_locale_variants("en_US.utf8")) do
    print(i, v)
end
print("Date and Time Functions")
print("Timers")
t = glib.timer_new()
t2 = glib.timer_new()
t3 = glib.timer_new()
t2:stop()
print("sleeping...")
glib.usleep(5e5)
t:stop() t2:continue()
print("again...")
glib.sleep(0.3)
t:continue()
t3:start()
print("again...")
glib.sleep(0.5)
print("awake!")
print("timers", t:elapsed(), t2:elapsed(), t3:elapsed())
print("Random Numbers")
for i = 1, 10 do
    print(glib.random(6), glib.random(), glib.random(100, 1000))
end
f = glib.rand_new{42, 17}
print("seed 42, 17")
for i = 1, 10 do
    print(f(6), f(), f(100, 1000))
end
f = glib.rand_new(345)
r = {}
for i = 1, 100 do
    table.insert(r, {f(6), f(), f(100, 1000)})
end
f = glib.rand_new(345)
for i, v in ipairs(r) do
    nv = {f(6), f(), f(100, 1000)}
    for j, sv in ipairs(v) do
	if sv ~= nv[j] then
	    print("mismatch " .. tostring(i))
	end
    end
end
a = {}
for i = 1, 26 do
    a[i] = string.char(i + string.byte('a') - 1)
end
-- make random stable
glib.random = f
-- cmorris' shuffle()
function shuffle(a)
    local i
    for i = 1, #a do
	local r = glib.random(#a)
	local t = a[i]
	a[i] = a[r]
	a[r] = t
    end
end
shuffle(a)
for i, v in ipairs(a) do
    io.write(v)
end
io.write('\n')
-- cmorris' choice()
function choice(a)
    if #a == 0 then
	return nil
    end
    return a[glib.random(#a)];
end
print(choice(a))
-- cmorris' sample()
function sample(a, n)
    local b = {}
    local s = {}
    local i
    for i = 1, n do
	-- warning, this could take a while if n is near #a
	repeat
	    r = glib.random(#a)
	until not b[r]
	b[r] = true
	s[i] = a[r]
    end
    return s
end
print(unpack(sample(a, 7)))
print("Miscellaneous Utility Functions")
print(glib.application_name("Test for lua-glib"), glib.prgname("glib-test"))
print(glib.application_name(), glib.prgname())
print(glib.application_name(), glib.prgname())
print(glib.getenv("PATH"))
print(glib.setenv("PATH", "." .. glib.searchpath_separator .. glib.getenv("PATH"), true))
print(glib.getenv("PATH"))
print(unpack(glib.listenv()))
print(glib.get_user_name(), glib.get_real_name())
for i, v in ipairs(glib.get_dir_name()) do
    d = glib.get_dir_name(v)
    if type(d) == 'table' then
	print(v, table.concat(d, glib.searchpath_separator))
    else
	print(v, d)
    end
end
print(glib.get_dir_name('xxx'))
glib.setenv("HOME", "/var/tmp", true)
print(glib.get_dir_name('home'))
cwd = glib.get_current_dir()
print(glib.get_host_name(), cwd)
print(glib.path_is_absolute("hello"), glib.path_is_absolute(cwd))
print(glib.path_split_root(cwd))
print(glib.path_split_root('hello'))
print(glib.path_get_basename(cwd),
      glib.path_get_dirname(cwd))
print(glib.build_filename())
print(glib.build_filename{'var', 'tmp'})
print(glib.build_filename('var', 'tmp/x'))
a = {}
for i = 1, 1000 do
    table.insert(a, 'd')
end
fn = glib.build_filename(a)
fp = glib.build_path(glib.dir_separator:sub(1, 1), a)
print(#fn, #fp, fn == fp)
print(glib.build_path(glib.searchpath_separator, 'var', 'tmp'))
-- following shows that this is worhtless for pathlist generation, since
-- blank paths can actually be significant
print(glib.build_path(glib.searchpath_separator, '', 'tst', '', 'now', ''))
print(table.concat({'', 'tst', '', 'now', ''}, glib.searchpath_separator))
-- there is also no inverse operation
function split_file(f)
    local res = {}
    local r
    r, f = glib.path_split_root(f)
    while f ~= '' do
	local b = glib.path_get_basename(f)
	table.insert(res, 1, b)
	if b == f then break end
	f = glib.path_get_dirname(f)
    end
    return r, res
end
root, path = split_file("//var/tmp")
print(root, table.concat(path, '|'))
root, path = split_file("")
print(root, table.concat(path, '|'))
root, path = split_file("hello//there")
print(root, table.concat(path, '|'))
function pr_sizes(s)
    print(glib.format_size(s), glib.format_size(s, true),
          glib.format_size(s, false, true), glib.format_size(s, true, true))
end
pr_sizes(1e6)
pr_sizes(1024*1024)
pr_sizes(2^48)
pr_sizes(2^60)
print(glib.find_program_in_path("more"))
print(glib.find_program_in_path("qzmore"))
a = {}
for i = 1, 20 do
    a[i] = { glib.random(1, 15), i }
end
function ltelt(e1, e2)
    return e1[1] < e2[1]
end
function cmpelt(e1, e2)
    return e1[1] - e2[1]
end
b = {unpack(a)}
c = {unpack(a)}
table.sort(b, ltelt)
glib.qsort(c, cmpelt)
-- this table is to prove whether or not glib's sort is stable vs. lua's
print("i", "a[i]", "c[i]", "cn[i]", "c=b", "bn[i]", "bn=cn")
for i, v in ipairs(c) do
    print(i, a[i][1], v[1], v[2], b[i][1] == v[1], b[i][2], b[i][2] == v[2])
end
mt = {}
mt.__sub = function(a, b) return #a - #b end
a = {1}
b = {0, 1}
setmetatable(a, mt) setmetatable(b, mt)
print(glib.cmp("a", "a\0"), glib.cmp(1, 2), glib.cmp(a, b))
print(glib.cmp("b", "a"), glib.cmp(2, 1), glib.cmp(b, a))
print(glib.cmp("a", "a"), glib.cmp(1, 1.0), glib.cmp(a, {0}))

p, msg = glib.spawn('cat /etc/issue')
if not p then print(msg) end
print(p:io_wait(true, true, true))
print(p:status())
r, o = p:wait()
print(#o)
print(p:io_wait(true, true, true))
print(p:status())
p = nil
p, msg = glib.spawn{'cat', '/etc/timezone'}
if not p then print(msg) end
print(p:wait())
function get_penv(t)
    local i, v, r
    r = {}
    for i, v in ipairs(t) do
	local e = glib.getenv(v)
	if e then r[v] = e end
    end
    return r
end
p, msg = glib.spawn{'sh', '-c', 'export', env = get_penv{"PATH", "ADA_INCLUDE_PATH"}}
if not p then print(msg) end
print(p:wait())
p, msg = glib.spawn('sleep 20')
glib.sleep(0.5)
p:kill()
print(p:wait())
p, msg = glib.spawn{'cat', stdin = '/etc/issue'}
if not p then print(msg) end
r, s = p:wait()
print(r, #s)
p, msg = glib.spawn{cmd = 'cat /etc/issue', stdout = '/tmp/catout'}
if not p then print(msg) end
print(p:wait())
os.remove('/tmp/catout')
f = io.open('/etc/timezone')
p, msg = glib.spawn{cmd = 'cat', stdin = f}
if not p then print(msg) end
f:close()
print(p:wait())
p, msg = glib.spawn{'sh', '-c', 'echo error >&2'}
if not p then print(msg) end
print(p:wait())
p, msg = glib.spawn{'sh', '-c', 'echo error >&2', stderr = '!/tmp/err'}
if not p then print(msg) end
print(p:wait())
os.remove('/tmp/err')
p, msg = glib.spawn{cmd='/bin/bash', 'sh', '-c', 'echo \\"$0\\" $BASH_VERSION'}
if not p then print(msg) end
print(p:wait())
p, msg = glib.spawn{cmd='/bin/zsh', 'sh', '-c', 'echo \\"$0\\" $ZSH_VERSION'}
if not p then print(msg) end
print(p:wait())
p, msg = glib.spawn{'cat', stdin = true}
p:write('hello, world!\n')
p:write('hello again, world!\n')
p:write('54 1e7 +23&')
p:close()
print(p:read(5))
print("'" .. p:read() .. "'")
print(p:read('*l'))
print(p:read('*n', '*n', '*n'))
print(p:wait())
p, msg = glib.spawn('cat /etc/issue')
nw = 0
for l in p:lines() do
    for w in l:gmatch('%S+') do
	nw = nw + 1
    end
end
p:wait()
print(nw)

os.execute('ln -sf /etc/issue issue.xx')
os.execute('ln -sf issue.xy issue.xy')
os.execute('ln -sf issue.xyz issue.xz')

for i, s in ipairs{'/etc', '/etc/issue', '/bin/true', 'issue.xx', 'issue.xy', 'issue.xz', 'issue.xyz'} do
    print(s, glib.exists(s), glib.is_file(s), glib.is_dir(s), glib.is_symlink(s), glib.is_exec(s))
end
os.remove('issue.xx')
os.remove('issue.xy')
os.remove('issue.xz')
print(glib.mkstemp('/tmp/blah'))
print(glib.mkstemp('/blahXXXXXX'))
f, n = glib.mkstemp('/tmp/blahXXXXXX.txt')
print(f, n)
f:write('hello, world!\n')
f:close()
os.execute('cat ' .. n)
os.remove(n)
print(glib.open_tmp('/tmp/blah'))
print(glib.open_tmp('blah'))
f, n = glib.open_tmp('blahXXXXXX')
print(f, n)
f:write('hello, world!\n')
f:close()
os.execute('cat ' .. n)
os.remove(n)
for e in glib.dir('/tmp') do
    io.write(e .. ' ')
end
io.write('\n')
