import adi
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

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

dev.rx_main_nco_frequencies = [2600000000] * NM_RX
dev.tx_main_nco_frequencies = [1300000000] * NM_TX

dev.jesd204_fsm_ctrl = "1"  # This will resync the NCOs and all links

dev.rx_main_6dB_digital_gains = [1] * NM_RX
dev.tx_main_6dB_digital_gains = [1] * NM_TX

dev.rx_channel_6dB_digital_gains = [1] * N_RX
dev.tx_channel_6dB_digital_gains = [1] * N_TX

dev.rx_enabled_channels = [0,1]
dev.tx_enabled_channels = [0,1]

dev.rx_nyquist_zone = ["even"]