clc
clearvars

config;

files = FL_BIN;

for n = 1:numel(files)
    file = [FILES_PATH files{n}];
    eval(['!./../build/bin2txt ' file])
end
