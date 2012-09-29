-- kpathsea does not use env. vars all the time
-- there is no way to detect kpsewhich without giving an error when not
-- present, unless I resort to apr
if not tex_progname then tex_progname = 'latex' end
function kpserun(arg)
    local proc = apr.proc_create('kpsewhich')
    proc:cmdtype_set('program/env/path')
    proc:io_set('none', 'parent-block', 'parent-block')
    proc:exec{arg, '-progname=' .. tex_progname}
    s = proc:out_get():read('*a') .. proc:err_get():read('*a')
    local done, why, code = proc:wait(true)
    if code == 0 then
	return s
    else
	return nil
    end
end
use_kpse = kpserun('-version') ~= nil
if use_kpse then
    for i, v in ipairs{'tex', 'bib', 'bst'} do
	V = v:upper()
        setenv(V .. 'INPUTS', trim(kpserun('-show-path=' .. v)))
    end
end
