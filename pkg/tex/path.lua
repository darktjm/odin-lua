-- kpathsea does not use env. vars all the time
-- there is no way to detect kpsewhich without giving an error when not
-- present, unless I resort to apr
if not tex_progname then tex_progname = 'latex' end
kpsewhich = glib.find_program_in_path('kpsewhich') or 'kpsewhich'
function kpserun(...)
   local p, err = glib.spawn{kpsewhich, '-progname=' .. tex_progname,
                             stderr = true, ...}
   if not p then return p, err end
   local code, out, err = p:wait()
   if code ~= 0 then return nil, err end
   return out .. err
end
use_kpse = kpserun('-version') ~= nil
if use_kpse then
    for i, v in ipairs{'tex', 'bib', 'bst'} do
	V = v:upper()
        setenv(V .. 'INPUTS', trim(kpserun('-show-path=' .. v)))
    end
end
