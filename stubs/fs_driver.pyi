# Original author: mhepp(https://forum.lvgl.io/u/mhepp/summary)
# Copyright (c) 2024 - 2025 Kevin G. Schlosser


import lvgl as lv


def _fs_open_cb(_, path: str, mode: int) -> dict[str, str]:
    ...


def _fs_close_cb(_, fs_file: lv.Blob) -> int:
    ...


def _fs_read_cb(_, fs_file: lv.Blob, buf: lv.Blob, btr: int, br: lv.Blob) -> int:
    ...


def _fs_seek_cb(_, fs_file: lv.Blob, pos: int, whence: int) -> int:
    ...


def _fs_tell_cb(_, fs_file: lv.Blob, pos: lv.Blob) -> int:
    ...


def _fs_write_cb(_, fs_file: lv.Blob, buf: lv.Blob, btw: int, bw: lv.Blob) -> int:
    ...


def _fs_dir_open_cb(_, path: str) -> dict[str, list[str]] | None:
    ...


def _fs_dir_read_cb(_, lv_fs_dir_t: lv.Blob, buf: lv.Blob, btr: int) -> int:
    ...


def _fs_dir_close_cb(_, __) -> int:
    ...


def fs_register(fs_drv: lv.fs_drv_t, letter: str, cache_size: int = 500) -> None:
    ...

