%%
clear
close all

%% Waveform generation

% BW: 400MHz  SCS: 120KHz  64QAM
M = 64;      % Modulation order
osf = 1.25;     % Oversampling factor
nfft = 3332;  % FFT length
cplen = 236;  % Cyclic prefix length
n_symbol = 14; % number of symbols

qamSig=zeros(nfft,n_symbol);
ofdm_symbol=zeros((nfft+cplen)*osf,n_symbol);
for i=1:n_symbol
    data = randi([0 M-1],nfft,1);
    qamSig(:,i) = qammod(data,M,UnitAveragePower=true);
    ofdm_symbol(:,i) = ofdmmod(qamSig(:,i),nfft,cplen,OversamplingFactor=osf); 
end

ofdm_waveform = reshape(ofdm_symbol,[n_symbol*(nfft+cplen)*osf,1]);
ofdm_waveform=ofdm_waveform./max(abs(ofdm_waveform))*2^14;
tx_size=length(ofdm_waveform);

txWaveform=[ofdm_waveform ofdm_waveform ofdm_waveform ofdm_waveform];
save ../waveform/tx_waveform.mat txWaveform

%% Setting ad9988

Python_path="C:\Users\aspha\AppData\Local\Programs\Python\Python310\"; % replace with your python path

pyenv(Version=Python_path+"python.exe",ExecutionMode="OutOfProcess");
setenv('TCL_LIBRARY', Python_path+'tcl\tcl8.6');
setenv('TK_LIBRARY', Python_path+'\tcl\tk8.6');

pyrunfile("../python/ad9081_sync_set.py")

%% Sync transmiting and receiving

pyrunfile("../python/ad9081_sync_run.py")

load ../waveform/rx_waveform_0.mat rx_waveform
y1=rx_waveform;
load ../waveform/rx_waveform_1.mat rx_waveform
y2=rx_waveform;
load ../waveform/rx_waveform_2.mat rx_waveform
y3=rx_waveform;
load ../waveform/rx_waveform_3.mat rx_waveform
y4=rx_waveform;

%% Plot wavefrom

figure
hold on
plot(abs(y1));
plot(abs(y2));
plot(abs(y3));
plot(abs(y4));
hold off

[freq_range,fft_result_y1]=fft_smooth(y1/2^14,500000000,tx_size,10);
[~,fft_result_y2]=fft_smooth(y2/2^14,500000000,tx_size,10);
[~,fft_result_y3]=fft_smooth(y3/2^14,500000000,tx_size,10);
[~,fft_result_y4]=fft_smooth(y4/2^14,500000000,tx_size,10);

figure
hold on
plot(freq_range, fft_result_y1);
plot(freq_range, fft_result_y2);
plot(freq_range, fft_result_y3);
plot(freq_range, fft_result_y4);
hold off
xlabel('Frequency (MHz)');ylabel('Amplitude (dB)');grid on;

%% Exit

terminate(pyenv);
