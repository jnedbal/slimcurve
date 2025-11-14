function compileSlimCurve(Cpath)
% compileSlimCurve([Cpath])
% Build mxSlimCurve.* without legacy mexopts.sh.
% - Works on modern MATLAB with MinGW/MSVC.
% - Auto-locates the C sources if Cpath is not supplied.

here = fileparts(mfilename('fullpath'));
old = cd(here); cleanup = onCleanup(@() cd(old));

% --- Resolve Cpath ---
if ~exist('Cpath','var') || isempty(Cpath)
    % Try common locations first
    candidates = { fullfile(here,'..','c'), fullfile(here,'c'), pwd };
    Cpath = '';
    for k = 1:numel(candidates)
        if exist(candidates{k},'dir') && exist(fullfile(candidates{k},'EcfSingle.c'),'file')
            Cpath = candidates{k}; break;
        end
    end
    % If still empty, search recursively for EcfSingle.c
    if isempty(Cpath)
        hit = dir(fullfile(here,'**','EcfSingle.c'));
        if isempty(hit)
            hit = dir(fullfile(pwd,'**','EcfSingle.c'));
        end
        if ~isempty(hit)
            Cpath = hit(1).folder;
        end
    end
    if isempty(Cpath)
        error('Could not locate the C sources (EcfSingle.c). Pass Cpath explicitly.');
    end
end
fprintf('Using Cpath = %s\n', Cpath);

% --- Check required files ---
need = {'EcfSingle.c','EcfUtil.c','Ecf.h','EcfInternal.h'};
for k = 1:numel(need)
    f = fullfile(Cpath,need{k});
    assert(exist(f,'file')==2, 'Missing %s in %s', need{k}, Cpath);
end
gate = fullfile(here,'mxSlimCurve.c');
assert(exist(gate,'file')==2, 'Missing mxSlimCurve.c in %s', here);

% --- Ensure a C compiler is selected ---
try, mex -setup C; catch ME, warning('%s', ME.message); end

% --- Compose build ---
src = { gate, fullfile(Cpath,'EcfUtil.c'), fullfile(Cpath,'EcfSingle.c') };
inc = { ['-I', Cpath] };
flags = {'-v','-R2018a'};
libs  = {}; if isunix && ~ismac, libs{end+1}='-lm'; end

% --- Compile (with fallback) ---
try
    fprintf('Compiling mxSlimCurve (R2018a API)…\n');
    mex(flags{:}, inc{:}, src{:}, libs{:});
catch ME
    warning('R2018a build failed (%s). Retrying with -R2017b…', ME.message);
    flags = {'-v','-R2017b'};
    mex(flags{:}, inc{:}, src{:}, libs{:});
end

out = ['mxSlimCurve.' mexext];
assert(exist(out,'file')==3, 'Build finished but %s not found.', out);
fprintf('Success: %s created in %s\n', out, here);
end
