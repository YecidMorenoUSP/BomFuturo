
clc
close all
clearvars

CUT_OFF = .1;
SAVE_VID = false;
FILES_PATH = '../../data/Prensa/';

config;


hold on
grid on

l={};
yyaxis left
l{1} = plot(nan,nan,'LineWidth',.1,'Color',[1 1 1 1]*.85);
yyaxis right
l{2} = plot(nan,nan,'LineWidth',2);

t = 1:4:length(data);
l{1}.XData = data_t(t);l{1}.YData = data(t);
l{2}.XData = data_t(t);l{2}.YData = data_f(t);

title("Pressure on cotton")

yyaxis left
xlabel("Time [ s ]")
ylabel("Volts [ V ]")
yyaxis right
ylabel("Pressure [ Bar ]")

y_val  = [0 3.3];


fig = gcf;
fig.Position = [398,366,1002,476];

if(SAVE_VID)
    v = VideoWriter('out','Motion JPEG AVI');
    v.Quality = 100;
    v.FrameRate = 5;
    open(v);
end

span = 60;
for sim = 80:1:(data_t(end) - span)
    try

       l{1}.YData = data(t);

       xlim([0 span]+sim)
       ylim(y_val)
    
        if(SAVE_VID)
            frame = getframe(gcf);
            writeVideo(v,frame);
        end
        
        if(~SAVE_VID)
%            pause(0.1);
             drawnow
        end
    catch
        disp("Exit")
        break;    
    end

end

if(SAVE_VID)
    close(v);
end
