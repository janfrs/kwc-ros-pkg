%% Runs the openrave hanoi example
addpath(fullfile(pwd,'..','octave')); % in case launched from test folder
openraveros_restart();
addexamplesdir('hanoi_puzzle');
hanoi
