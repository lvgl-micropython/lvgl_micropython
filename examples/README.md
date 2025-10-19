# Examples

## Waveshare esp32 s3 1.9" IPS kit

Example [waveshare_esp32s3_1.9.py](./waveshare_esp32s3_1.9.py) for Waveshare kit as described [here](https://www.waveshare.com/product/arduino/boards-kits/esp32-s3/esp32-s3-lcd-1.9.htm).

### building and flashing

Assuming you have lvgl_micropython repo checked out and an esp-idf environment in your path:

```bash
python3 make.py esp32 clean BOARD=ESP32_GENERIC_S3 BOARD_VARIANT=SPIRAM_OCT DISPLAY=st7789 INDEV=cst816s
esptool.py erase_flash
cd lib/micropython/ports/esp32
esptool.py --chip esp32s3 -b 460800 --before default_reset write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 build-ESP32_GENERIC_S3-SPIRAM_OCT/bootloader/bootloader.bin 0x8000 build-ESP32_GENERIC_S3-SPIRAM_OCT/partition_table/partition-table.bin 0x10000 build-ESP32_GENERIC_S3-SPIRAM_OCT/micropython.bin
```

### copy example to device

Copy example file to device using mpremote (`pip install mpremote` to install)
```bash
mpremote cp waveshare_esp32s3_1.9.py :main.py
```


## Waveshare esp32 c6 1.9" IPS kit

Example [waveshare_esp32c6_1.9.py](./waveshare_esp32c6_1.9.py) for Waveshare kit as described [here](https://www.waveshare.com/product/arduino/boards-kits/esp32-c6/esp32-c6-lcd-1.9.htm).

### building and flashing

Assuming you have lvgl_micropython repo checked out and an esp-idf environment in your path:

```bash
python3 make.py esp32 clean BOARD=ESP32_GENERIC_C6 DISPLAY=st7789 INDEV=cst816s
cd lib/micropython/ports/esp32
esptool.py erase_flash
esptool.py --chip esp32c6 -b 460800 --before default_reset write_flash --flash_mode dio --flash_size 4MB --flash_freq 80m 0x0 build-ESP32_GENERIC_C6/bootloader/bootloader.bin 0x8000 build-ESP32_GENERIC_C6/partition_table/partition-table.bin 0x10000 build-ESP32_GENERIC_C6/micropython.bin
```

### copy example to device
```bash
mpremote cp waveshare_esp32c6_1.9.py :main.py
```
