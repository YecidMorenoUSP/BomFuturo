% 
% convert_all;
% 
% clc
% clearvars
% close all

config;


data = [];

for nFile = 1:numel(FL_TXT)
    file_name = FL_TXT{nFile};
    file_path = [FILES_PATH file_name];
    data = [data;load(file_path)];
end


figure(101)
plot((1:length(data))/1000,data)
ylim([0 4096])