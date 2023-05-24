
% convert_all;
% 
% clc
% clearvars
% close all

config;

file_name = FL_TXT{end};
file_path = [FILES_PATH file_name];

% data = load(file_path);

TS = 500;
t = length(data)-10000;
td = t:length(data);
t = (td)/TS;

if(t<=0)
    plot(data(1:1:end))
else
    plot(t,data(td))
end

ylim([0 4095])
