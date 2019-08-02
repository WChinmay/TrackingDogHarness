% Initialize Arduino
% arduino() and fgets()/fread() instead of serial() and fscan() can also be used
global s                % Serial Connection
s = serial('COM4');     % Replace with USB Port
s.Baudrate = 115200;    % Set Baud Rate
fopen(s);
pause(5);               % TIme for serial configuration

% Set intial values for lat, lon, vBatt and temp
lat = 44.567;
lon = -123.2755;
vBatt = 4.06;
temp = 19.5;

% Generate map 
plot_google_map('MapScale', 2)
axis([-123.288 -123.270 44.563 44.571])         % View for Tyler
% axis([-123.286 -123.270 44.561 44.570])         % Default view of OSU
% axis([-123.310 -123.286 44.561 44.570])         % 1 hr Test View
set(gcf, 'Position', get(0, 'Screensize'));     % Maximize view
plot_google_map('Refresh', 1)                   % Set Map to Refresh with Movement
plot_google_map('MapType', 'satellite');        % Satellite Map View
hold on;
pause(5);       % time to load google map data
% Plot a temp point in the center
h1 = plot(-123.2755, 44.567, '.r', 'MarkerSize', 10);  % Dearborn Hall
t1 = text(1,1,'Init','Color', 'k', 'BackgroundColor', 'w'); % Default text
    
% Loop Start
while 1
    % Get data string
    pause(1);
    dataString = fscanf(s);  % Format -  44.57235, -123.27560, 4.14, 20.57, 51, 36, 1137,                        
    % pause(1);
    % Check if received data, so that I can keep matlab script running even
    % if data connection is lost
    if(~isempty(dataString))
    % Verify that the data we received is ours
    if(strncmp(dataString,"8",1) ~= 1)      % Since we know that if the data is ours, it will start with an 8
        continue                            % Read serial again if the data wasn't from our beta transceiver
    end
    % Parse Data and get rid of garbage
    dataMatrix = strsplit(dataString,', ');     % Split string along delimiter
    lat = str2double(strip(char(dataMatrix(2))));   % Latitude
    lon = str2double(strip(char(dataMatrix(3))));   % Longitude
    vBatt = str2double(strip(char(dataMatrix(4)))); % Battery voltage
    temp = str2double(strip(char(dataMatrix(5))));  % Temperature
    analogRead1 = str2double(strip(char(dataMatrix(6))));   % AnalogRead1 value
    analogRead2 = str2double(strip(char(dataMatrix(7))));   % AnalogRead2 value
    packetnum = str2double(strip(char(dataMatrix(8))));     % Packet Number
    % Time stuff
    t = datetime('now');    % Get current date time
    % Scale analog readings
    analogRead1 = (analogRead1/1023)*6.6;
    analogRead2 = (analogRead2/1023)*6.6;
    % CSV File Writing Stuff
    M = [lat, lon, vBatt, temp, analogRead1, analogRead2, packetnum, hour(t), minute(t), second(t)];
    dlmwrite('test.csv',M,'delimiter',',','-append');       % Append in file for logging
    end
    % pause(300);     % Pause for 5 minutes for the 12 hour test
    % Plot the point on the map after removing the previous plot
    delete(h1); 
    h1 = plot(lon, lat, '.r', 'MarkerSize', 20);    % Plot point
    % Plot the text information on the map in a convenient spot
    txt = ['Latitude: ' num2str(lat) ' Longitude: ' num2str(lon) ' Temperature: ' num2str(temp) ' C, Battery Voltage: ' num2str(vBatt) ' v'];
    % txt = ['Temperature: ' num2str(temp) ' C, Battery Voltage: ' num2str(vBatt) ' v'];
    % txt = ['Temperature: ' num2str(temp) ' C, Battery Voltage: ' num2str(vBatt) ' v, Analog Value 1: ' num2str(analogRead1) ' v, Analog Value 2: ' num2str(analogRead2) 'v. '];
    delete(t1);     % Remove previous text
    % t1 = text(-123.285,44.5695,txt, 'Color', 'k', 'BackgroundColor', 'w');
    t1 = text(-123.2840,44.5635,txt, 'Color', 'k', 'BackgroundColor', 'w');      % Text for Tyler 
end		% Loop Stop
hold off
% Closing Serial
fclose(s)
delete(s)
clear all


