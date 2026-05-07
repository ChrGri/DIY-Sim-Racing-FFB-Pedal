clear all; close all; clc;

% This Matlab script was used to design the step loss recovery function. The iSV57 tells the ESP whats the count of the received pulses. The ESP compares it to the count of pulses it transmitted. Based on that, it calculates how many pulses have been lost and transmitts additional pulses to compensate the loss. 

%% 
% Unwrap the servos position values:

% the ESPs internal position is given as int32 variable
% it wraps at 2^31-1
% the servos step count per revolution is 6400. It would take revolutions =
% (2^31-1)steps / (6400steps/revolution) = 3.3554e+05 = 335540 revolutions
% to clip. The typical rail size is 100mm. The spindle pitch is 5mm/revolution. Thats
% 100mm/(5mm/revolution) = 20revolutions --> The ESPs internal position
% counter will not clip.

spindlePitch = 5; % mm/rev
railLength = 150; % mm
maxRevolutions = railLength / spindlePitch
stepsPerRev = 6400;
maxExpectedSteps = stepsPerRev * maxRevolutions;

% calculate the max step difference between ESP and servo caused by
% discrete sampling interval of the servo
transmissionLatency = 0.05; % seconds
maxRpm = 4000; % rev / min
maxStepLatency = stepsPerRev * ( maxRpm / 60 * transmissionLatency )



% simulate the ESPs internal position signal over broad range
stepSignal_ESP = -maxExpectedSteps:maxExpectedSteps;

% add servo offset
servoOffsetToEsp = -maxStepLatency;


% simulate the servos step clipping
stepSignal_servo = stepSignal_ESP + servoOffsetToEsp;
for i = 1:10
    % positive modulo
    tmp = stepSignal_servo > (2^15-1);
    stepSignal_servo(tmp) = stepSignal_servo(tmp) - 2^16;

    % negative modulo
    tmp = stepSignal_servo < -(2^15);
    stepSignal_servo(tmp) = stepSignal_servo(tmp) + 2^16;
end


% detect expected wrap
% t = stepSignal_ESP;
% wrapCounter = zeros(size(stepSignal_ESP));
% for i = 1:10
%     % positive modulo
%     tmp = t > (2^15-1);
%     t(tmp) = t(tmp) - 2^16;
%     wrapCounter(tmp) = wrapCounter(tmp) + 1;
% 
%     % negative modulo
%     tmp = t < -(2^15);
%     t(tmp) = t(tmp) + 2^16;
%     wrapCounter(tmp) = wrapCounter(tmp) - 1;
% end
% wraps = wrapCounter;
% stepSignal_servo_corrected = stepSignal_servo + wraps*2^16;


% best alignment
numberOfExpectedWraps = ceil( (max(abs(stepSignal_ESP)) + 2^15) / 2^16 );
stepSignal_servo_corrected_2 = stepSignal_servo;
wrapCounter = zeros(size(stepSignal_ESP));
for i = 1:numberOfExpectedWraps
    % positive modulo
    posDiff = stepSignal_ESP - stepSignal_servo_corrected_2;
    tmp = posDiff > (2^15-1);
    stepSignal_servo_corrected_2(tmp) = stepSignal_servo_corrected_2(tmp) + 2^16;
    wrapCounter(tmp) = wrapCounter(tmp) + 1;
    
    % negative modulo
    posDiff = stepSignal_ESP - stepSignal_servo_corrected_2;
    tmp = posDiff < -(2^15);
    stepSignal_servo_corrected_2(tmp) = stepSignal_servo_corrected_2(tmp) - 2^16;
     wrapCounter(tmp) = wrapCounter(tmp) - 1;
end 


nexttile()
plot(stepSignal_ESP, stepSignal_ESP, 'r', 'DisplayName', 'ESP')
hold on
plot(stepSignal_ESP, stepSignal_servo, 'g', 'DisplayName', 'Servo')
% plot(stepSignal_ESP, stepSignal_servo_corrected, '--b', 'DisplayName', 'ServoCorrected')
plot(stepSignal_ESP, stepSignal_servo_corrected_2, '--y', 'DisplayName', 'ServoCorrected')

grid on
% title()
xlabel('ESP step count \rightarrow')
ylabel('Devices step count \rightarrow')
legend('Location', 'northoutside')


nexttile()
plot(stepSignal_ESP, wrapCounter)
grid on
xlabel('ESP step count \rightarrow')
ylabel('Expected wrap count \rightarrow')

nexttile()
% plot(stepSignal_ESP, stepSignal_ESP - stepSignal_servo_corrected, 'b')
hold on
plot(stepSignal_ESP, (stepSignal_ESP-maxStepLatency) - (stepSignal_servo_corrected_2), 'y')
grid on
xlabel('ESP step count \rightarrow')
ylabel('Difference ESP - Servo (corrected) \rightarrow')


display("Done!")
%% 
% Step loss recovery:

% due to transmission errors, some steps the ESP sends to the servo are
% lost. This can cause a position drift between ESP and the servo. That
% means, the position from the ESP and the servo will drift apart, which in
% turn results in (a) diverging pedal position and (b) can cause the pedal
% to crash into the endstops of the sled without knowing. 
% To prevent this issue, the ESP obtains the servos internal position via
% the rs232 port and compares it to the ESPs expected position. If
% significant position difference is detected, the ESP sends corrections
% step/dir signals to the servo, thus ESP and servo position will be
% alligned again.
% Due to latency in the transmission chain and latency of the closed-loop
% position, the ESPs and servos position are delayed. Thus, computation of
% lost steps is only reasonable when the pedal is at steady state. 


% Assume that the ESPs internal position is correct and only the servos
% position must be corrected. This can be done via by sending correcting
% step/dir signals to the servo, like such 
% _stepper->setCurrentPosition(_stepper->getCurrentPosition() - stepOffset);

% Now stepOffset can be calculated as follows:
% stepOffset = stepper_cl->getCurrentPosition() - stepper_cl->getServosInternalPosition(); 
% With that, we see the ESPs position gets aligned with the servos position
% ESP pos   = ESPs position - stepOffset = ESPs position - (ESPs position - servos position) 
%           = ESPs position - ESPs position + servos position
%           = servos position


%% 
% Crash detection:

% If the servo crashes against bumpstops, it can cause severe overheating,
% as significant current is going through the coils.
% To prevent overheating, crashes into bumpstops must be detected and ovoided. 

% In factory setting, the servo has positive rotation angle (right hand
% rule, counter clockwise = positive rotation), when looking to the exposed axle.
% A positive rotation means, that the sled on the spindle is pulled towards
% the motor. 

% If the current is positive, the rotation direction is positive --> the
% sled will be pulled towards the motor. 

% compute the expected force on the spindle from pedal force. When they
% significantly differ, the bumpstop might have been hit. 

% calculate spindle torque depending on loadcell force
