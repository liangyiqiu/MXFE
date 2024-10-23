Vivado HDL Steps
1. add axi quad spi IP
2. drag QSPI FLASH from BOARD to axi quad spi block
3. Assign address for axi quad spi (right click assign)
4. generate block design, delete old HDL wrapper and create a new one
5. edit system_top.v, add spi_flash IOs in system_top module and link them to system_wrapper module
6. run synthesis, then open synthesiszed design
7. tool->edit devie properties->configuration, set configuration rate (51 MHz) and bus width (8 bits), save the new constraints
8. generate bit stream
9. export hardware with bitstream

Vitis IDE Steps
1. create application project with exported hardware design, choose hello world template
2. delete src file and add eb-config.h,elf32.h and elf-bootloader.c
3. set the offset address (0x04000000) of the image in eb-config.h
4. open lscript.ld, change all sections to BRAM region
5. build the hardware platform and the application
6. Xilinx->program device, choose the exported bit and mmi file, choose the elf file built last step, generate the download.bit file

Flash Steps
1. make mcs file in Vivado with the download.bit file, choose the correct flash type (mt25qu01g-spi-x1_x2_x4_x8)
2. open hardware manager, add configuration memory device and program it
3. open xsdb, program the image to flash (program_flash -f simpleImage.vcu118_ad9081_204c_txmode_9_rxmode_10_500Msps.strip -flash_type mt25qu01g-spi-x1_x2_x4_x8 -offset 0x04000000 -target_id 2)
4. power off and on, the boot imformation will be printed to the uart console