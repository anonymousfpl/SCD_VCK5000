function verify_func(bitW,bitF1,bitCM,bitF2)
%% Initialization
Type = 'FAM';
current_dir = pwd;  
Amp = 1; % Set the signal amplitude to 1

%% Input signal
index = 2;
fname = 'Deepsig';
path = [current_dir, '/', fname];

tic;
temp2 = load('DeepSig.mat').a(index,:,:);
% Perform normalization and quantize the signal data into a fixed-point format.
% Since 15 bits are used for the fractional part, the integer part has only 1 bit, 
% representing numbers in the range [-1, 1) (typical Q15 format).
temp = Amp * (single(temp2) ./ single(max(abs(temp2))));
% Construct the complex signal x
x = temp(:,:,1) + temp(:,:,2)*1i;

%% Part5
x = [x x];
% x = x * 10;
size(x)

% saveMatrixData(x, 'result_x.dat');
result_save = {};

% Set sampling rate fs, frequency resolution df, and cyclic frequency resolution dalpha
fs = 1000; df = 5; dalpha = 0.5; 
result_save.(['resultcell',num2str(index)]) = {'bits_w','bits_F1','bits_CM','bits_F2', 'normsimulation', 'normcalculation','q1','q2','q3'};

bit.input = 26;
bit.windowing = 26;
bit.firstFFT = 26;
bit.ConjMulti = 26;
bit.secondFFT = 26;

% Generate filename based on parameters
if 0 <= index && index < 10
    filename = [fname Type num2str(bit.windowing) num2str(bit.firstFFT) ...
                num2str(bit.ConjMulti) num2str(bit.secondFFT) ...
                'Amp' num2str(100*Amp) 'Norm0' num2str(index) 'v4'];
else
    filename = [fname Type num2str(bit.windowing) num2str(bit.firstFFT) ...
                num2str(bit.ConjMulti) num2str(bit.secondFFT) ...
                'Amp' num2str(100*Amp) 'Norm' num2str(index) 'v4'];
end

listing = dir(path);
exist = 0;

% If the file does not exist, call autofamv4_M1 function to process and save the result
if exist == 0
    switch Type
        case 'FAM'
            [Sx,result]=autofamv4_M1(x,fs,df,dalpha,bit);
    end
    % disp(['-----------' 'save' filename '-----------']);
end
toc;

end


function [Sx,result]=autofamv4_M1(x,fs,df,dalpha,bitInput)
%   AUTOFAM(X,FS,DF,DALPHA) computes the spectral auto-correlation 
%   density function estimate of the signals X by using the FFT 
%   Accumulation Method (FAM). Make sure DF is much bigger than 
%   DALPHA in order to have a reliable estimate.
%
%   INPUTS:
%   X       - input column vector
%   FS      - sampling rate
%   DF      - desired frequency resolution
%   DALPHA  - desired cyclic frequency resolution
%
%   OUTPUTS:
%   SX      - spectral correlation density function estimate
%
%   e.g. x = (1:2240)'; fs = 1000; df = 5; dalpha = 0.5;
%   or x = repmat(0:0.1:0.3, [1 560]); fs = 1000; df = 5; dalpha = 0.5;

if nargin ~= 5
    error('Wrong number of arguments.');
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Definition of Parameters %%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
Np = pow2(nextpow2(fs/df));    % Number of points per channel (2^power to avoid cut/zero-fill for FFT)
L = Np / 4;                    % Offset between adjacent rows in the channelization matrix
P = pow2(nextpow2(fs/dalpha/L)); 
N = P * L;                     % Total number of points

bit = bitInput;  % Save bit-width settings  

% If signal length < N, zero-pad; if length > N, truncate
if length(x) < N
    x(N) = 0;
elseif length(x) > N
    x = x(1:N);
end

NN = (P - 1) * L + Np;
xx = x;
xx(NN) = 0;
xx = xx(:);

% Split the signal data into P columns, each column has Np points
X = zeros(Np, P);
for k = 0:P-1
    X(:, k+1) = xx(k*L + 1 : k*L + Np);
end

%%%%%%%%%%%%%%%
%% Windowing %%
%%%%%%%%%%%%%%%
% Window coefficients (Hamming) stored in array a
a = [0.079999924,0.08013916,0.080558777,0.081256866,0.082231522,0.083486557,...
     0.085018158,0.086826324,0.088907242,0.091264725,0.093893051,0.096792221,...
     0.099962234,0.10339737,0.10709953,0.111063,0.11528778,0.11976814,0.124506,...
     0.12949562,0.13473511,0.14021873,0.1459465,0.15191269,0.15811539,0.16454887,...
     0.17121124,0.17809677,0.18520355,0.19252396,0.20005608,0.20779419,0.21573448,...
     0.22387123,0.23220062,0.24071503,0.24941254,0.25828743,0.26733208,0.27654266,...
     0.28591156,0.29543686,0.30510902,0.31492615,0.32487679,0.33496094,0.34516716,...
     0.35549355,0.36593056,0.37647438,0.38711739,0.39785194,0.40867424,0.41957474,...
     0.43054962,0.44159126,0.45269203,0.4638443,0.47504425,0.48628426,0.49755669,...
     0.50885391,0.52017021,0.53149986,0.54283333,0.55416489,0.56548882,0.57679749,...
     0.58808327,0.59934044,0.61055946,0.62173843,0.63286591,0.64393806,0.65494537,...
     0.66588593,0.67674828,0.6875267,0.69821739,0.70881081,0.71930122,0.72968483,...
     0.73995209,0.75009727,0.76011467,0.77000046,0.7797451,0.78934479,0.79879189,...
     0.80808449,0.81721115,0.82617188,0.83495903,0.84356499,0.85198784,0.86022186,...
     0.86826134,0.87610054,0.88373566,0.89116287,0.89837646,0.90537262,0.91214752,...
     0.91869545,0.9250145,0.93109894,0.93694687,0.94255447,0.94791603,0.95302963,...
     0.95789337,0.96250343,0.96685791,0.97095108,0.97478485,0.9783535,0.98165512,...
     0.98468971,0.98745537,0.98994827,0.99216843,0.99411201,0.99578285,0.99717522,...
     0.99829102,0.99912834,0.99968529,0.99996567,0.99996567,0.99968529,0.99912834,...
     0.99829102,0.99717522,0.99578285,0.99411201,0.99216843,0.98994827,0.98745537,...
     0.98468971,0.98165512,0.9783535,0.97478485,0.97095108,0.96685791,0.96250343,...
     0.95789337,0.95302963,0.94791603,0.94255447,0.93694687,0.93109894,0.9250145,...
     0.91869545,0.91214752,0.90537262,0.89837646,0.89116287,0.88373566,0.87610054,...
     0.86826134,0.86022186,0.85198784,0.84356499,0.83495903,0.82617188,0.81721115,...
     0.80808449,0.79879189,0.78934479,0.7797451,0.77000046,0.76011467,0.75009727,...
     0.73995209,0.72968483,0.71930122,0.70881081,0.69821739,0.6875267,0.67674828,...
     0.66588593,0.65494537,0.64393806,0.63286591,0.62173843,0.61055946,0.59934044,...
     0.58808327,0.57679749,0.56548882,0.55416489,0.54283333,0.53149986,0.52017021,...
     0.50885391,0.49755669,0.48628426,0.47504425,0.4638443,0.45269203,0.44159126,...
     0.43054962,0.41957474,0.40867424,0.39785194,0.38711739,0.37647438,0.36593056,...
     0.35549355,0.34516716,0.33496094,0.32487679,0.31492615,0.30510902,0.29543686,...
     0.28591156,0.27654266,0.26733208,0.25828743,0.24941254,0.24071503,0.23220062,...
     0.22387123,0.21573448,0.20779419,0.20005608,0.19252396,0.18520355,0.17809677,...
     0.17121124,0.16454887,0.15811539,0.15191269,0.1459465,0.14021873,0.13473511,...
     0.12949562,0.124506,0.11976814,0.11528778,0.111063,0.10709953,0.10339737,...
     0.099962234,0.096792221,0.093893051,0.091264725,0.088907242,0.086826324,...
     0.085018158,0.083486557,0.082231522,0.081256866,0.080558777,0.08013916,0.079999924];
XW = single(diag(a) * X); % Apply the window to each column of X

%%%%%%%%%%%%%%%
%% First FFT %%
%%%%%%%%%%%%%%%
% Perform the first FFT on the windowed signal XW
XF1 = fft(XW);
% Shift zero-frequency component to the center
XF1_shift = fftshift(XF1, 1);
XF1_shift = single(XF1_shift);

%%%%%%%%%%%%%%%%%%%%
%% Downconversion %%
%%%%%%%%%%%%%%%%%%%%
% Apply a time shift to compensate for the L offset in the data before.
E = zeros(Np,P);
for k = -Np/2 : Np/2 - 1
    for input_index = 0 : P-1
        E(k + Np/2 + 1, input_index + 1) = exp(-1i * 2 * pi * k * input_index * L / Np);
    end
end

% Multiply FFT result with downconversion matrix E
XD_beforeTranspose = XF1_shift .* E;

%%%%%%%%%%%%%%%%%%%%
%% Transpose %%
%%%%%%%%%%%%%%%%%%%%
XD = transpose(XD_beforeTranspose);
XD = single(XD);

%%%%%%%%%%%%%%%%%%%%
%% Multiplication %%
%%%%%%%%%%%%%%%%%%%%
% Self-multiplication (autocorrelation-like operation)
XM = zeros(P, Np^2);
for k=1:Np
    for l=1:Np
        XM(:, (k-1)*Np + l) = (XD(:, k) .* conj(XD(:, l)));
    end
end
XM = single(XM);

%%%%%%%%%%%%%%%%
%% Second FFT %%
%%%%%%%%%%%%%%%%
XF2 = fft(XM);
XF2_shift = fftshift(XF2,1);
result.SecondFFT = single(XF2_shift);
XF2_shift = single(XF2_shift);

% Clip high-frequency and low-frequency components, keeping only the middle portion (optional)
% XF2_2 = XF2_shift(P/4+1 : 3*P/4, :); 
XF2_3 = XF2_shift([1:8, 25:32], :);
XF2_3_abs = abs(XF2_3).^2;

% Get the magnitude
Sx = zeros(Np+1, 2*N+1);

end


function OutputSignal = FFTFloatv3_M1(InputSignal)
% FFTFloatv3_M1 performs an FFT calculation in single-precision float.
%
% Input:
%   InputSignal - the input signal matrix, with rows as the signal length
%                 and columns as different time samples
%
% Output:
%   OutputSignal - FFT-transformed signal matrix

S = double(InputSignal); 
FFTn = length(S(:,1));  
k = length(S(1,:));    
N = FFTn;

W = exp(-1 * 2j * pi * (0:N-1) / N);
OutputSignal = zeros(FFTn, k);

p = nextpow2(FFTn);

for i = 1:k
    s = bitrevorder(S(:,i));  % bit-reversal ordering
    s = [s zeros(1, (2^p) - length(s))]; % zero-pad if needed

    WINDOWSIZE_LOG = log2(N);
    Half = 1;
    % DIT (Decimation-In-Time) FFT
    for step = 1:WINDOWSIZE_LOG
        for index = 0:(N / (2^(WINDOWSIZE_LOG - step))):(N - 1)
            for n = 0:Half - 1
                pos = n + index + 1;
                pow = (2^(WINDOWSIZE_LOG - step)) * n;
                w = W(pow + 1);

                a = (s(pos) + s(pos + Half) * w);
                b = (s(pos) - s(pos + Half) * w);
                s(pos) = a;
                s(pos + Half) = b;
            end
        end
        Half = Half * 2;
    end
    OutputSignal(:, i) = s;
end

end
