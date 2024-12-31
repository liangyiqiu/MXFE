%%
clear
close all

addpath('python\')
addpath('waveform\')
addpath("EVMMeasurementOfNRDownlinkWaveformsExample\")

plot_waveform=true;
plot_evm=true;

%% Simulation parameters
% Each NR-TM or FRC waveform is defined by a combination of:
%
% * NR-TM/FRC name
% * Channel bandwidth
% * Subcarrier spacing
% * Duplexing mode

% Select one of the Release 15 NR-TMs for FR1 and FR2 among:
% "NR-FR1-TM1.1","NR-FR1-TM1.2","NR-FR1-TM2",
% "NR-FR1-TM2a","NR-FR1-TM3.1","NR-FR1-TM3.1a",
% "NR-FR1-TM3.2","NR-FR1-TM3.3","NR-FR2-TM1.1",
% "NR-FR2-TM2","NR-FR2-TM2a","NR-FR2-TM3.1","NR-FR2-TM3.1a"

% or
% Select one of the Release 15 FRCs for FR1 and FR2 among:
% "DL-FRC-FR1-QPSK","DL-FRC-FR1-64QAM",
% "DL-FRC-FR1-256QAM","DL-FRC-FR2-QPSK",
% "DL-FRC-FR2-16QAM","DL-FRC-FR2-64QAM"

rc = "NR-FR2-TM3.1"; % Reference channel (NR-TM or FRC)

% Select the NR waveform parameters
bw = "100MHz"; % Channel bandwidth
scs = "120kHz"; % Subcarrier spacing
dm = "FDD"; % Duplexing mode
fprintf('Reference Channel = %s\n', rc);

%%
% For TMs, the generated waveform may contain more than one physical data
% shared channel (PDSCH). The chosen PDSCH to analyse is based on the radio
% network temporary identifier (RNTI). By default, these RNTIs are
% considered for EVM calculation:
%
% * NR-FR1-TM2: RNTI = 2 (64QAM EVM)
% * NR-FR1-TM2a: RNTI = 2 (256QAM EVM)
% * NR-FR1-TM3.1: RNTI = 0 and 2 (64QAM EVM)
% * NR-FR1-TM3.1a: RNTI = 0 and 2 (256QAM EVM)
% * NR-FR1-TM3.2: RNTI = 1 (16QAM EVM)
% * NR-FR1-TM3.3: RNTI = 1 (QPSK EVM)
% * NR-FR2-TM2: RNTI = 2 (64QAM EVM)
% * NR-FR2-TM2a: RNTI = 2 (256QAM EVM)
% * NR-FR2-TM3.1: RNTI = 0 and 2 (64QAM EVM)
% * NR-FR2-TM3.1a: RNTI = 0 and 2 (256QAM EVM)
%
% As per the specifications (TS 38.141-1, TS 38.141-2), these TMs are not
% designed to perform EVM measurements: NR-FR1-TM1.1, NR-FR1-TM1.2,
% NR-FR2-TM1.1. However, if you generate these TMs, the example measures
% the EVM for the following RNTIs.
%
% * NR-FR1-TM1.1: RNTI = 0 (QPSK EVM)
% * NR-FR1-TM1.2: RNTI = 2 (QPSK EVM)
% * NR-FR2-TM1.1: RNTI = 0 (QPSK EVM)
%
% For PDSCH FRCs and physical downlink control channel (PDCCH), by default,
% RNTI 0 is considered for EVM calculation.

%%
% To model wideband filter effects, specify a higher waveform
% sample rate. You can increase the sample rate by multiplying the
% waveform bandwidth with the oversampling factor, |OSR|.
% To use the nominal sample rate, set |OSR| to 1.
OSR =2.5;% oversampling factor

% Create waveform generator object 
tmwavegen = hNRReferenceWaveformGenerator(rc,bw,scs,dm);

% Waveform bandwidth
bandwidth = tmwavegen.Config.ChannelBandwidth*1e6;

if OSR > 1
    % The |Config| property in |tmwavegen| specifies the configuration of
    % the standard-defined reference waveform. It is a read-only property.
    % To customize the waveform, make the |Config| property writable.
    tmwavegen = makeConfigWritable(tmwavegen);

    % Increase the sample rate by multiplying the waveform bandwidth by
    % |OSR|
    tmwavegen.Config.SampleRate = bandwidth*OSR;
end

tmwavegen.Config.NumSubframes=1;

% Generate the waveform and get the waveform sample rate
[txWaveform_origin,tmwaveinfo,resourcesinfo] = generateWaveform(tmwavegen,tmwavegen.Config.NumSubframes);
sr = tmwaveinfo.Info.SamplingRate; % waveform sample rate


%%
% Normalize the waveform to fit the dynamic range of the dac.
% txWaveform=var.waveform;
txWaveform_origin = txWaveform_origin./max(abs(txWaveform_origin)).*2^14;
% txWaveform = txWaveform/max(max(real(txWaveform),max(imag(txWaveform))));

% txWaveform = imag(txWaveform)+real(txWaveform)*1i;

tx_size=length(txWaveform_origin);
tx_size=tx_size/4;

tx_waveform_aggregate=zeros(tx_size*2,1);
tx_waveform_aggregate(1:2:end)=txWaveform_origin(1:tx_size);
% tx_waveform_aggregate(2:2:end)=txWaveform_origin(tx_size+1:tx_size*2)./2;

lo_waveform=zeros(tx_size*2,1);
lo_waveform(1:2:end)=ones(tx_size,1);
% lo_waveform(2:2:end)=-ones(tx_size,1);


txWaveform = [repelem(tx_waveform_aggregate,1) repelem(lo_waveform,1)*2^14];
% txWaveform = [txWaveform_origin(1:tx_size) ones(tx_size,1)*2^14];

tmwavegen.Config.SampleRate = bandwidth*OSR;
%% CAF

% thr_dB = 8;
% oversample=5;
% T=7;
% c_caf = CAF(txWaveform,thr_dB,oversample,T);
% txWaveform=txWaveform-c_caf;

save ./waveform/tx_waveform.mat txWaveform


%% run python script

pyenv(Version="C:\Users\aspha\AppData\Local\Programs\Python\Python310\python.exe",ExecutionMode="OutOfProcess");
setenv('TCL_LIBRARY', 'C:\Users\aspha\AppData\Local\Programs\Python\Python310\tcl\tcl8.6');
setenv('TK_LIBRARY', 'C:\Users\aspha\AppData\Local\Programs\Python\Python310\tcl\tk8.6');
pyrunfile("./python/ad9081_digital_assit_MIMO_set_2T2R.py")
pyrunfile("./python/ad9081_digital_assit_MIMO_run_2T2R.py")
terminate(pyenv);

load ./waveform/rx_waveform_0.mat rx_waveform
% rx_waveform = 1i * (real(rx_waveform) - imag(rx_waveform)) + real(rx_waveform) + imag(rx_waveform) * 1i;

% tx_waveform = imag(txWaveform)+real(txWaveform)*1i;
 tx_waveform=tx_waveform_aggregate;
% tx_waveform=txWaveform_origin(1:tx_size);
rx_waveform=rx_waveform(1:2:end);

[CRX,lagRX] = xcorr(tx_waveform,rx_waveform);
CRX=abs(CRX);
[MRX,IRX]=max(CRX);
delay=-1-lagRX(IRX);
% delay=600;

figure
plot(lagRX,CRX);

rxWaveform=rx_waveform(delay:delay*2+tx_size)';
rxWaveform = imag(rxWaveform)+real(rxWaveform)*1i;

if plot_waveform
    figure
    hold on
    plot(real(rx_waveform));
    plot(imag(rx_waveform));
    hold off
    
    fs=250000000;
    nSamp = length(rxWaveform);
    rx_out_fft = fftshift(20*log10(abs(fft(rxWaveform/2^14)/nSamp)));
    df = fs/nSamp; 
    freq_range_rx = (-fs/2:df:fs/2-df).'/1000000;
    
    figure
    plot(freq_range_rx, rx_out_fft);
    xlabel('Frequency (MHz)');ylabel('Amplitude (dB)');grid on;
end

%% Measurements
% The function, hNRDownlinkEVM, performs these steps to decode and analyze the
% waveform:
%
% * Coarse frequency offset estimation and correction
% * Integer frequency offset estimation and correction
% * I/Q imbalance estimation and correction
% * Synchronization using the demodulation reference signal (DM-RS) over
% one frame for FDD (two frames for TDD)
% * OFDM demodulation of the received waveform
% * Fine frequency offset estimation and correction
% * Channel estimation
% * Equalization
% * Common Phase error (CPE) estimation and compensation
% * PDSCH EVM computation (enable the switch |evm3GPP|, to process
% according to the EVM measurement requirements specified in TS 38.104,
% Annex B (FR1) / Annex C (FR2)).
% * PDCCH EVM computation
%
% The example measures and outputs various EVM related statistics per
% symbol, per slot, and per frame peak EVM and RMS EVM. The example
% displays EVM for each slot and frame. It also
% displays the overall EVM averaged over the entire input waveform. The
% example produces a number of plots: EVM vs per OFDM symbol, slot,
% subcarrier, and overall EVM. Each plot displays the peak vs RMS EVM.

cfg = struct();
cfg.Evm3GPP = false;
cfg.TargetRNTIs = [];
cfg.PlotEVM =plot_evm;
cfg.DisplayEVM = true;
cfg.Label = tmwavegen.ConfiguredModel{1};
cfg.IQImbalance = true;

% Compute and display EVM measurements
[evmInfo,eqSym,refSym] = hNRDownlinkEVM(tmwavegen.Config,rxWaveform,cfg);

