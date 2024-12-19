
import lvgl as lv
import struct
import ctypes

# Calculate pointer size on current machine, and corresponding fmt

ptr_size = ctypes.sizeof({'p': (ctypes.PTR, ctypes.VOID)})  # NOQA
fmt_options = {2: 'H', 4: 'L', 8: 'Q'}
buf_fmt = fmt_options[ptr_size] if ptr_size in fmt_options else None


def aligned_buf(buf, alignment):

    """Return an aligned buffer

    Given a buffer, return a memory view within that buffer, which starts
    at an aligned address in RAM.
    The returned memory view is possibly smaller.

    !! You must keep a reference to the original buffer to prevent the
       garbage collector from collecting the aligned view!

    Arguments:

    buf         -- An object that implements buffer protocol
    alignment   -- Integer value

    """

    p = lv.C_Pointer()  # NOQA
    p.ptr_val = buf

    if not buf_fmt:
        return None

    addr = struct.unpack(buf_fmt, p.ptr_val)[0]
    mod = addr % alignment

    offset = alignment - mod if mod != 0 else 0
    if len(buf) <= offset:
        return None

    addr += offset
    p = lv.C_Pointer.__cast__(struct.pack(buf_fmt, addr))  # NOQA

    return p.ptr_val.__dereference__(len(buf) - offset)
