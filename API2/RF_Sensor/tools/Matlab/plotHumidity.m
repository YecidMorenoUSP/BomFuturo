
clc
close all
clearvars

fig = gcf;
fig.Position = [ 280 157 1119 614 ];

file_paths = {'../../data/H4.1_Pluma/'
              '../../data/H12.4/'
              '../../data/H6.2/'};

colors = {'#0072BD' '#D95319' '#EDB120'};

CUT_OFF = .5;
DATA   = [];
DATA_F = [];


hold on

samples = 0;
ln = [];
for idx_f = 1:numel(file_paths)
    data = [];
    countFiles = 0;
    FILES_PATH = file_paths{idx_f};
    config;
    DATA = [DATA;data];
    DATA_F = [DATA_F;data_f];

    plot((1:length(data))+samples,data,'Color',[1 1 1]*.8)
    ln(idx_f) = plot((1:length(data_f))+samples,data_f, ...
        Color=colors{idx_f},LineWidth=3);
    samples = samples+length(data);
end


legend(ln,{'H:4.1 Pluma' 'H:12.4' 'H:6.2'})

title("Microwave Sensor")

yyaxis left
% plot(DATA)
yyaxis right
% plot(DATA_F)

yyaxis left
xlabel("Samples [ a ]")
ylabel("Volts [ V ]")
ylim([2.4 3.3])

yyaxis right
ylabel("Humidity [ % ]")


xlim([0 samples])
grid on

print('out','-dpng','-r600')


