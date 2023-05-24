if exist('data','var') ~= 1
    data = [];
end

if exist('countFiles','var') ~= 1
    countFiles = 0;
end

if exist('FILES_PATH','var') ~= 1
    FILES_PATH = '../../data/';
end



DIR_TXT = dir([FILES_PATH '*.txt']);

S = DIR_TXT;
S = S(~[S.isdir]);
[~,idx] = sort([S.datenum]);
DIR_TXT = S(idx);

FL_TXT = {DIR_TXT.name};

if(numel(FL_TXT) > countFiles)
    d = numel(FL_TXT) - countFiles;
    fprintf("Loading %d files\n",numel(FL_TXT) - countFiles)
    leer = (1+numel(FL_TXT)-d):(numel(FL_TXT));
    disp(leer)
    for nFile = leer  
        file_name = FL_TXT{nFile};
        file_path = [FILES_PATH file_name];
        data_cur = load(file_path)*-3.3/4095 + 3.3;
        data = [data;data_cur];
    end
%     data  = 3.3-data * 3.3/4095;
    countFiles = numel(FL_TXT);
end

if(numel(FL_TXT) < countFiles)
    disp("Borrando datos")
    countFiles = 0;
    data = [];
end


if exist('CUT_OFF','var') ~= 1
    CUT_OFF = 10;
end

Ts = 500;
dt = 1/Ts;
X_TIME = [0 length(data)/Ts-1 ];

data_f = data;
rc = 1/(2*pi*CUT_OFF);
alpha = (dt)/(dt+rc);

for i = 2:length(data_f)
    data_f(i) = alpha*data(i)  +  (1-alpha)*data_f(i-1);
end

t_idx  = 1:length(data);
data_t = t_idx/Ts;