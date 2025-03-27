function verify_func()
% Set sampling rate fs, frequency resolution df, and cyclic frequency resolution dalpha
fs = 1000; df = 5; dalpha = 0.5; %df 5 10 2.5 dalpha 0.5 1 0.25
bit.input = 26;
bit.windowing = 26;
bit.firstFFT = 26;
bit.ConjMulti = 26;
bit.secondFFT = 26;
result=autofamv4_M1(fs,df,dalpha,bit);
save_final_result_to_files(result);
end

function result=autofamv4_M1(fs,df,dalpha,bitInput)
%   AUTOFAM(X,FS,DF,DALPHA) computes the spectral auto-
%   correlation density function estimate of the signals X
%   by using the FFT Accumulation Method(FAM). Make sure that
%   DF is much bigger than DALPHA in order to have a reliable estimate.
%
%   INPUTS:
%   X       - input column vector;
%   FS      - sampling rate;
%   DF      - desired frequency resolution;
%   DALPHA  - desired cyclic frequency resolution
%
%   OUTPUTS:
%   SX     - spectral correlation density function estimate;
%   ALPHAO  - cyclic frequency;
%   FO      - spectrum frequency;
%
% e.g. x = (1:2240)'; fs = 1000; df = 5; dalpha = 0.5;
% or x = repmat(0:0.1:0.3, [1 560]); fs = 1000; df = 5; dalpha = 0.5;

% Definition of Parameters
Np = pow2(nextpow2(fs/df));  % Calculate the number of input channels, set as power of 2 to avoid truncation or zero-padding during FFT
L = Np / 4;  % Set the offset between adjacent rows in the channelization matrix, not greater than Np/4
P = pow2(nextpow2(fs/dalpha/L));  % Set the number of rows in the channelization matrix, defined as a power of 2
N = P * L;  % Total number of input data points

bit = bitInput;  % Save bit width settings

% Input Channelization
X = complex(zeros(256, 32));  % Target matrix: 256 rows, 32 columns (complex type)

% Iterate through 8 input files
for ch = 0 : 7
    fname = fullfile('data_in', sprintf('FAMDataIn_%d.txt', ch));
    data  = dlmread(fname);  % Read file assuming format: real imag
    cdata = complex(data(:,1), data(:,2));  % Convert to complex vector of length 1024

    % Divide 1024 cfloat into 4 blocks (256 each), each block fills a column
    for blk = 0 : 3
        col_index = ch * 4 + blk + 1;  % Column index (1 to 32)
        start_idx = blk * 256 + 1;    % Start index
        end_idx   = (blk + 1) * 256;  % End index
        X(:, col_index) = cdata(start_idx : end_idx);  % Fill data into matrix
    end
end

result.Input = X;


%%%%%%%%%%%%%%%
%% Windowing %%
%%%%%%%%%%%%%%%
% a=hamming(Np);
a = [0.079999924,0.08013916,0.080558777,0.081256866,0.082231522,0.083486557,0.085018158,0.086826324,0.088907242,0.091264725,0.093893051,0.096792221,0.099962234,0.10339737,0.10709953,0.111063,0.11528778,0.11976814,0.124506,0.12949562,0.13473511,0.14021873,0.1459465,0.15191269,0.15811539,0.16454887,0.17121124,0.17809677,0.18520355,0.19252396,0.20005608,0.20779419,0.21573448,0.22387123,0.23220062,0.24071503,0.24941254,0.25828743,0.26733208,0.27654266,0.28591156,0.29543686,0.30510902,0.31492615,0.32487679,0.33496094,0.34516716,0.35549355,0.36593056,0.37647438,0.38711739,0.39785194,0.40867424,0.41957474,0.43054962,0.44159126,0.45269203,0.4638443,0.47504425,0.48628426,0.49755669,0.50885391,0.52017021,0.53149986,0.54283333,0.55416489,0.56548882,0.57679749,0.58808327,0.59934044,0.61055946,0.62173843,0.63286591,0.64393806,0.65494537,0.66588593,0.67674828,0.6875267,0.69821739,0.70881081,0.71930122,0.72968483,0.73995209,0.75009727,0.76011467,0.77000046,0.7797451,0.78934479,0.79879189,0.80808449,0.81721115,0.82617188,0.83495903,0.84356499,0.85198784,0.86022186,0.86826134,0.87610054,0.88373566,0.89116287,0.89837646,0.90537262,0.91214752,0.91869545,0.9250145,0.93109894,0.93694687,0.94255447,0.94791603,0.95302963,0.95789337,0.96250343,0.96685791,0.97095108,0.97478485,0.9783535,0.98165512,0.98468971,0.98745537,0.98994827,0.99216843,0.99411201,0.99578285,0.99717522,0.99829102,0.99912834,0.99968529,0.99996567,0.99996567,0.99968529,0.99912834,0.99829102,0.99717522,0.99578285,0.99411201,0.99216843,0.98994827,0.98745537,0.98468971,0.98165512,0.9783535,0.97478485,0.97095108,0.96685791,0.96250343,0.95789337,0.95302963,0.94791603,0.94255447,0.93694687,0.93109894,0.9250145,0.91869545,0.91214752,0.90537262,0.89837646,0.89116287,0.88373566,0.87610054,0.86826134,0.86022186,0.85198784,0.84356499,0.83495903,0.82617188,0.81721115,0.80808449,0.79879189,0.78934479,0.7797451,0.77000046,0.76011467,0.75009727,0.73995209,0.72968483,0.71930122,0.70881081,0.69821739,0.6875267,0.67674828,0.66588593,0.65494537,0.64393806,0.63286591,0.62173843,0.61055946,0.59934044,0.58808327,0.57679749,0.56548882,0.55416489,0.54283333,0.53149986,0.52017021,0.50885391,0.49755669,0.48628426,0.47504425,0.4638443,0.45269203,0.44159126,0.43054962,0.41957474,0.40867424,0.39785194,0.38711739,0.37647438,0.36593056,0.35549355,0.34516716,0.33496094,0.32487679,0.31492615,0.30510902,0.29543686,0.28591156,0.27654266,0.26733208,0.25828743,0.24941254,0.24071503,0.23220062,0.22387123,0.21573448,0.20779419,0.20005608,0.19252396,0.18520355,0.17809677,0.17121124,0.16454887,0.15811539,0.15191269,0.1459465,0.14021873,0.13473511,0.12949562,0.124506,0.11976814,0.11528778,0.111063,0.10709953,0.10339737,0.099962234,0.096792221,0.093893051,0.091264725,0.088907242,0.086826324,0.085018158,0.083486557,0.082231522,0.081256866,0.080558777,0.08013916,0.079999924];
a = fi(a,1,bit.windowing+2,bit.windowing+1);  % Convert window coefficients to fixed-point
XW=single(diag(a)*X);  % Apply window function to each column
result.Windowing = XW;

% First FFT
XF1=FFTFloatv3_M1(XW);  % Perform first FFT
XF1_shift=fftshift(XF1, 1);  % Shift zero-frequency component to center
result.FirstFFT = single(XF1_shift);  % Save FFT result
XF1_shift = single(XF1_shift);

% Downconversion
E=zeros(Np,P);
a=zeros(Np,P);
for k = -Np/2:Np/2-1
    for input_index = 0:P-1
        a_value = -2 * pi * k * input_index * L / Np;
        a_value = mod(a_value + pi, 2 * pi) - pi;
        a_normalized = a_value / pi;
        if abs(a_normalized) < 0.00001
            a_normalized = 0;
        end
        a(k + Np/2 + 1, input_index + 1) = a_normalized;
        E(k + Np/2 + 1, input_index + 1) = exp(-1i * 2 * pi * k * input_index * L / Np);
    end
end
XD_beforeTranspose=XF1_shift.*E;

% Transpose
XD=transpose(XD_beforeTranspose);  % Transpose matrix so each row represents a time segment
result.XD = XD;
XD = single(XD);

% Multiplication
XM=zeros(P,Np^2);
for k=1:Np
    for l=1:Np
        XM(:,(k-1)*Np+l)=(XD(:,k).*conj(XD(:,l)));  % Auto-correlation
    end
end
result.Multi = single(XM);
XM = single(XM);

% Second FFT
XF2=FFTFloatv3_M1(XM);
XF2_1=fft(XM);
XF2=fftshift(XF2,1);
result.SecondFFT = single(XF2);
XF2 = single(XF2);
format long
XF2_2=XF2(P/4+1:3*P/4, :);  % Retain central portion after trimming
result.final_result = single(XF2_2);

end

function OutputSignal = FFTFloatv3_M1(InputSignal)
% FFTFloatv3_M1 performs FFT computation on single-precision floating-point data
% Input:
%   InputSignal - input signal matrix (rows: signal length, columns: time samples)
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
    s = bitrevorder(S(:,i));
    s = [s zeros(1, (2^p) - length(s))];
    WINDOWSIZE_LOG = log2(N);

    % Decimation-In-Time (DIT) FFT Algorithm
    Half = 1;
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

function save_final_result_to_files(result)
    % final_result: 16 x 65536
    finalMat = result.final_result;
    [numRows, numCols] = size(finalMat);
    % numRows=16, numCols=65536

    % Basic checks
    if numRows ~= 16 || numCols ~= 65536
        error('Matrix size must be 16 x 65536.');
    end

    % Create output directory if it doesn't exist
    outputDir = 'data_out';
    if ~exist(outputDir, 'dir')
        mkdir(outputDir);
    end

    % Each file has 8192 lines: 16 rows/col * 512 cols = 8192
    % So total 128 files are needed: 65536 columns / 512 = 128.
    numFiles = 128;          % 0..127
    colsPerFile = 512;       % columns in each file
    linesPerFile = 8192;     % rows in each file (since 16 * 512 = 8192)

    % Loop over each file
    for fIdx = 0:(numFiles-1)
        % Build filename FAMOut_0.txt ... FAMOut_127.txt in data_out folder
        outName = fullfile(outputDir, sprintf('FAMOut_%d.txt', fIdx));
        fid = fopen(outName, 'w');
        if fid < 0
            error('Cannot open file: %s', outName);
        end

        % Current file handles columns: fIdx*512+1 : (fIdx+1)*512
        colStart = fIdx * colsPerFile + 1;        
        colEnd   = (fIdx + 1) * colsPerFile;      

        % For each of the 512 columns in this file
        for c = colStart : colEnd
            % The row range is always 1..16 => total 16 lines per column
            for r = 1:numRows
                realPart = real(finalMat(r, c));
                imagPart = imag(finalMat(r, c));
                fprintf(fid, '%f %f\n', realPart, imagPart);
            end
        end

        fclose(fid);
    end

    disp('All files have been saved successfully in the data_out folder.');
end
