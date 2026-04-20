# 4D Systems gen4-ESP32-70CT (LVGL MicroPython)

This configuration targets the 4D Systems gen4-ESP32-70CT (800x480 capacitive touch).

- Board info: `display_configs/Gen4-ESP32-70CT/board.json`
- TOML config: `display_configs/Gen4-ESP32-70CT/Gen4-ESP32-70CT.toml`

## Build

Run from the repository root:

```bash
python make.py esp32 clean BOARD=ESP32_GENERIC_S3 BOARD_VARIANT=SPIRAM_OCT --toml=display_configs/Gen4-ESP32-70CT/Gen4-ESP32-70CT.toml INDEV=ft5x46
```

## Build + Flash in one step

If the board is connected, you can build and flash directly:

```bash
python3 make.py esp32 --toml=display_configs/Gen4-ESP32-70CT/Gen4-ESP32-70CT.toml deploy
```

Optional explicit serial settings:

```bash
python3 make.py esp32 --toml=display_configs/Gen4-ESP32-70CT/Gen4-ESP32-70CT.toml PORT=/dev/ttyACM0 BAUD=921600 deploy
```

Notes:
- `PORT` can be omitted. The build script will try to auto-detect the ESP32 port.
- If multiple ESP32 devices are connected, the script will ask which one to flash.

## Flash later (manual)

If you build without `deploy`, the script prints an exact flash command at the end under:

- `To flash firmware:`

Run that printed command to flash later.

## Common issue

Do not use `--board-config` with GNU make. For this project, board presets are selected with `--toml=...`.
