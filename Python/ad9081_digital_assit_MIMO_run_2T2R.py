import adi
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

dev = adi.ad9081("ip:192.168.2.1")

dev.rx_enabled_channels = [0,1]
dev.tx_enabled_channels = [0,1]

dev.rx_buffer_size = 2**18
dev.tx_cyclic_buffer = False

dev.disable_dds()

# load waveform
from scipy import signal
import scipy.io as scio

dataFile = './waveform/tx_waveform.mat'
data = scio.loadmat(dataFile)
tx_waveform=(data['txWaveform'])

# Enable BRAM offload in FPGA (Necessary for high rates)
dev.tx_ddr_offload = 1

if dev.jesd204_device_status_check:
    print(dev.jesd204_device_status)

dev.rx_sync_start = "arm"
dev.tx_sync_start = "arm"

if not ("arm" == dev.tx_sync_start == dev.rx_sync_start):
    raise Exception(
        "Unexpected SYNC status: TX "
        + dev.tx_sync_start
        + " RX: "
        + dev.rx_sync_start
    )

dev.tx_destroy_buffer()
dev.rx_destroy_buffer()
dev._rx_init_channels()

dev.tx([tx_waveform[:,0],tx_waveform[:,1]])

dev.tx_sync_start = "trigger_manual"

if not ("disarm" == dev.tx_sync_start == dev.rx_sync_start):
    raise Exception(
        "Unexpected SYNC status: TX "
        + dev.tx_sync_start
        + " RX: "
        + dev.rx_sync_start
    )

try:
    x = dev.rx()
except Exception as e:
    print("Run#----------------------------- FAILED:", e)

# save rx waveform
dataNew = './waveform/rx_waveform_0.mat'
scio.savemat(dataNew, {'rx_waveform':x[0]})

dataNew = './waveform/rx_waveform_1.mat'
scio.savemat(dataNew, {'rx_waveform':x[1]})

print("Waveform saved. size:",len(x))