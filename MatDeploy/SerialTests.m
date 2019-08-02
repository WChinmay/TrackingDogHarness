% Initialize Arduino
% arduino() and fgets()/fread() instead of serial() and fscan()
s = serial('COM4');
fopen(s);
pause(5);
% Generate map 
plot_google_map('MapScale', 2)
axis([-123.286 -123.270 44.561 44.570])
plot_google_map('Refresh', 1)
plot_google_map('MapType', 'satellite');
hold on;
pause(5);
% Plot a temp point in the center
h1 = plot(-123.2755, 44.567, '.r', 'MarkerSize', 10);  % Dearborn Hall
t1 = text(1,1,'Init','Color', 'r');
    
% Loop Start
while 1
    % Get data string
    pause(1)
    dataString = fscanf(s);  % Format - 4433.8633, N, 12316.8550, W, 3.20, 70.42, ?
    % Parse Data and get rid of garbage
    dataMatrix = strsplit(dataString,', ');
    lat = str2double(strip(char(dataMatrix(1))));
    lon = str2double(strip(char(dataMatrix(2))));
    vBatt = str2double(strip(char(dataMatrix(3))));
    temp = str2double(strip(char(dataMatrix(4))));
    
    % Plot the point on the map after removing the previous plot
    delete(h1); % might not need to do this
    h1 = plot(lon, lat, '.r', 'MarkerSize', 10);
    txt = ['Temperature: ' num2str(temp) ' C, Battery Voltage: ' num2str(vBatt)];
    delete(t1)
    t1 = text(-123.285,44.5695,txt,'Color', 'r');
end		% Loop Stop
% Closing Serial
fclose(s)
delete(s)
clear s