# Read-only access to the running game (PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION).
import ctypes, ctypes.wintypes as wt, struct, subprocess

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.OpenProcess.restype = wt.HANDLE
k32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
BASE = 0x140000000


def pid():
    import os
    if os.environ.get('CD_PID'):  # native two-copy harness: the copy to read
        return int(os.environ['CD_PID'])
    out = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq CrimsonDesert.exe', '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout
    return int(out.split(',')[1].strip('"'))


H = k32.OpenProcess(0x0010 | 0x1000, False, pid())


def rd(addr, n):
    buf = ctypes.create_string_buffer(n)
    got = ctypes.c_size_t()
    if not k32.ReadProcessMemory(H, ctypes.c_void_p(addr), buf, n, ctypes.byref(got)):
        return None
    return buf.raw[:got.value]


def q(addr):
    b = rd(addr, 8)
    return struct.unpack('<Q', b)[0] if b else None


def f32s(addr, n):
    b = rd(addr, 4 * n)
    return struct.unpack('<%df' % n, b) if b else None
