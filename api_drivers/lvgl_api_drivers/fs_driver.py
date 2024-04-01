'''
Original author: mhepp(https://forum.lvgl.io/u/mhepp/summary)
'''

import lvgl as lv
import struct

def _fs_open_cb(drv, path, mode):

    if mode == lv.FS_MODE_WR:
        p_mode = 'wb'
    elif mode == lv.FS_MODE_RD:
        p_mode = 'rb'
    elif mode == lv.FS_MODE_WR | lv.FS_MODE_RD:
        p_mode = 'rb+'
    else:
        raise RuntimeError("fs_open_callback() - open mode error, %s is invalid mode" % mode)

    try:
        f = open(path, p_mode)

    except OSError as e:
        raise RuntimeError("fs_open_callback(%s) exception: %s" % (path, e))

    return {'file' : f, 'path': path}


def _fs_close_cb(drv, fs_file):
    try:
        fs_file.__cast__()['file'].close()
    except OSError as e:
        raise RuntimeError("fs_close_callback(%s) exception: %s" % (fs_file.__cast__()['path'], e))

    return lv.FS_RES_OK


def _fs_read_cb(drv, fs_file, buf, btr, br):
    try:
        tmp_data = fs_file.__cast__()['file'].read(btr)
        buf.__dereference__(btr)[0:len(tmp_data)] = tmp_data
        br.__dereference__(4)[0:4] = struct.pack("<L", len(tmp_data))
    except OSError as e:
        raise RuntimeError("fs_read_callback(%s) exception %s" % (fs_file.__cast__()['path'], e))

    return lv.FS_RES_OK


def _fs_seek_cb(drv, fs_file, pos, whence):
    try:
        fs_file.__cast__()['file'].seek(pos, whence)
    except OSError as e:
        raise RuntimeError("fs_seek_callback(%s) exception %s" % (fs_file.__cast__()['path'], e))

    return lv.FS_RES_OK


def _fs_tell_cb(drv, fs_file, pos):
    try:
        tpos = fs_file.__cast__()['file'].tell()
        pos.__dereference__(4)[0:4] = struct.pack("<L", tpos)
    except OSError as e:
        raise RuntimeError("fs_tell_callback(%s) exception %s" % (fs_file.__cast__()['path'], e))

    return lv.FS_RES_OK


def _fs_write_cb(drv, fs_file, buf, btw, bw):
    try:
        wr = fs_file.__cast__()['file'].write(buf.__dereference__(btw)[0:btw])
        bw.__dereference__(4)[0:4] = struct.pack("<L", wr)
    except OSError as e:
        raise RuntimeError("fs_write_callback(%s) exception %s" % (fs_file.__cast__()['path'], e))

    return lv.FS_RES_OK


def fs_register(fs_drv, letter, cache_size=500):
    lv.fs_drv_init(fs_drv)
    fs_drv.letter = ord(letter)
    fs_drv.open_cb = _fs_open_cb
    fs_drv.read_cb = _fs_read_cb
    fs_drv.write_cb = _fs_write_cb
    fs_drv.seek_cb = _fs_seek_cb
    fs_drv.tell_cb = _fs_tell_cb
    fs_drv.close_cb = _fs_close_cb

    if cache_size >= 0:
        fs_drv.cache_size = cache_size

    lv.fs_drv_register(fs_drv)

