# Copyright (C) 2021 Analog Devices, Inc.
#
# SPDX short identifier: ADIBSD

import adi
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal


def gen_tone(fc, fs, N):
    fc = int(fc / (fs / N)) * (fs / N)
    ts = 1 / float(fs)
    t = np.arange(0, N * ts, ts)
    i = np.cos(2 * np.pi * t * fc) * 2 ** 14
    q = np.sin(2 * np.pi * t * fc) * 2 ** 14
    return i + 1j * q


dev = adi.ad9081("ip:192.168.2.1")

# Configure properties
print("--Setting up chip")

# Total number of channels
N_RX = len(dev.rx_channel_nco_frequencies)
N_TX = len(dev.tx_channel_nco_frequencies)

# Total number of CDDCs/CDUCs
NM_RX = len(dev.rx_main_nco_frequencies)
NM_TX = len(dev.tx_main_nco_frequencies)

dev._ctx.set_timeout(3000)
dev._rxadc.set_kernel_buffers_count(1)
dev._txdac.set_kernel_buffers_count(2)

# Set NCOs
dev.rx_channel_nco_frequencies = [0] * N_RX
dev.tx_channel_nco_frequencies = [0] * N_TX

dev.rx_main_nco_frequencies = [1300000000] * NM_RX
dev.tx_main_nco_frequencies = [6700000000] * NM_TX

dev.jesd204_fsm_ctrl = "1"  # This will resync the NCOs and all links

dev.rx_main_6dB_digital_gains = [1] * NM_RX
dev.tx_main_6dB_digital_gains = [1] * NM_TX

dev.rx_channel_6dB_digital_gains = [1] * N_RX
dev.tx_channel_6dB_digital_gains = [1] * N_TX

dev.rx_enabled_channels = [0,1,2,3]
dev.tx_enabled_channels = [0,1,2,3]

dev.rx_nyquist_zone = ["even"]

dev.rx_buffer_size = 2**15
dev.tx_cyclic_buffer = True

dev.disable_dds()

# Create a sinewave waveform
fs = int(dev.rx_sample_rate)
N = 1024
iq1 = gen_tone(fs/256, fs, N)
iq2 = gen_tone(fs/32, fs, N)

# load waveform
from scipy import signal
import scipy.io as scio

dataFile = '../waveform/tx_waveform.mat'
data = scio.loadmat(dataFile)
tx_waveform=(data['txWaveform'])

# Enable BRAM offload in FPGA (Necessary for high rates)
dev.tx_ddr_offload = 1

dev.tx([tx_waveform[:,0],tx_waveform[:,1],tx_waveform[:,2],tx_waveform[:,3]])

x = dev.rx()
plt.plot(x[1].real)
plt.plot(x[1].imag)
plt.show()