function compileSlimCurve(Cpath)
% compileSlimCurve(path) a function to compile the mxSlimCurve to use with
%   your installation of Matlab.
%
%       Cpath    Directory containing the SlimCurve source code.
%               [Default = '../c/']
%
%   This code copies the mexopts.sh file from matlabroot and removes the
%   instances of "-ansi" from it for SlimCurve to compile properly. It
%   copies EcfSingle.c, EcfUtil.c, Ecf.h, and EcfInternal.h from Cpath into
%   its folder and deletes old compiled binaries. It finally compiles
%   SlimCurve to work with Matlab.
% GNU GPL license 3.0
% copyright 2013 Jakub Nedbal

% Assume that C-files are in '../c/' if no folder has been provided
if ~exist('Cpath', 'var')
    Cpath = ['..' filesep 'c' filesep];
end

% Check if source directory exists
if ~exist(Cpath, 'dir')
    error('Source directory for C-files does not exist');
end

% Check if SlimCurve source files exist
files = {'EcfSingle.c', 'EcfUtil.c', 'Ecf.h', 'EcfInternal.h'};
for file = files
    if ~exist([Cpath file{1}], 'file')
        error('Cannot find %s in %s.', file{1}, Cpath);
    end
end


% Check is mc.SlimCurve.c exists in the current folder.
if ~exist('mxSlimCurve.c', 'file')
    error('Cannot find mxSlimCurve.c.');
end

% Copy mexopts.sh file from matlabroot and delete all instances of "-ansi"
% from it. Otherwise SLimcurve would not work
copyfile([matlabroot filesep 'bin/mexopts.sh'], 'mexopts.sh', 'f');
perl('replaceANSI.pl', 'mexopts.sh');

% Copy all necessary slimcurve source files into the current directory
files = {'EcfSingle.c', 'EcfUtil.c', 'Ecf.h', 'EcfInternal.h'};
for file = files
    copyfile([Cpath file{1}], '.', 'f');
end

% Delete old SlimCurve compiles binaries
if exist(['mxSlimCurve.' mexext], 'file')
    delete(['mxSlimCurve.' mexext]);
end

% Compile SlimCurve
mex -f ./mexopts.sh mxSlimCurve.c EcfUtil.c EcfSingle.c