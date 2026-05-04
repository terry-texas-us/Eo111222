"""
aespy._connection
~~~~~~~~~~~~~~~~~
Low-level transport helpers shared by the pipe and DDE test modules.

Pipe transport
--------------
Uses raw Win32 CreateFile / WriteFile / ReadFile / CloseHandle via ctypes.
Python's built-in open() and the msvcrt/os.fdopen wrapper chain both
introduce CRT-level buffering and GC-delayed CloseHandle calls that prevent
the server from seeing the pipe disconnect promptly, causing PIPE_BUSY on
every reconnect attempt.

By keeping the raw HANDLE and calling CloseHandle directly we guarantee
that Windows sees the disconnect the moment close() is called.

DDE transport
-------------
Uses ctypes + windll to call the Win32 DdeInitialize / DdeClientTransaction
API directly.  No pywin32 required.
The DDE service name is "AeSys", topic "Commands".
"""

from __future__ import annotations

import ctypes
import ctypes.wintypes as wt
import struct
import time

# ---------------------------------------------------------------------------
# Pipe constants (must match EoPipeProtocol.h)
# ---------------------------------------------------------------------------

PIPE_PATH = r"\\.\pipe\AeSys"
PIPE_CONNECT_TIMEOUT = 5.0   # seconds to retry before giving up
PIPE_RETRY_INTERVAL  = 0.25  # seconds between retries

# Win32 constants
_GENERIC_READ_WRITE    = 0xC0000000
_OPEN_EXISTING         = 3
_FILE_ATTRIBUTE_NORMAL = 0x80
_INVALID_HANDLE_VALUE  = ctypes.c_void_p(-1).value

_k32 = ctypes.windll.kernel32

_k32.CreateFileW.restype  = ctypes.c_void_p
_k32.CreateFileW.argtypes = [
    wt.LPCWSTR, wt.DWORD, wt.DWORD,
    ctypes.c_void_p, wt.DWORD, wt.DWORD, ctypes.c_void_p,
]
_k32.WriteFile.restype  = wt.BOOL
_k32.WriteFile.argtypes = [
    ctypes.c_void_p, ctypes.c_char_p, wt.DWORD,
    ctypes.POINTER(wt.DWORD), ctypes.c_void_p,
]
_k32.ReadFile.restype  = wt.BOOL
_k32.ReadFile.argtypes = [
    ctypes.c_void_p, ctypes.c_char_p, wt.DWORD,
    ctypes.POINTER(wt.DWORD), ctypes.c_void_p,
]
_k32.CloseHandle.restype  = wt.BOOL
_k32.CloseHandle.argtypes = [ctypes.c_void_p]
_k32.GetLastError.restype = wt.DWORD


def _connect(path: str, timeout: float) -> int:
    """Return a raw Win32 HANDLE to the named pipe, polling until *timeout*."""
    deadline = time.monotonic() + timeout
    last_err: int = 0
    while time.monotonic() < deadline:
        h = _k32.CreateFileW(
            path, _GENERIC_READ_WRITE, 0, None,
            _OPEN_EXISTING, _FILE_ATTRIBUTE_NORMAL, None,
        )
        if h is not None and h != _INVALID_HANDLE_VALUE:
            return h
        last_err = _k32.GetLastError()
        time.sleep(PIPE_RETRY_INTERVAL)
    raise ConnectionError(
        f"Could not open {path!r} within {timeout}s (last Win32 error {last_err})"
    )


# ---------------------------------------------------------------------------
# Pipe connection
# ---------------------------------------------------------------------------

class PipeConnection:
    """Synchronous, line-oriented client for the AeSys named-pipe server.

    Keeps a raw Win32 HANDLE so CloseHandle fires immediately on close(),
    letting the server call ConnectNamedPipe for the next client without delay.

    Usage::

        with PipeConnection() as pipe:
            response = pipe.send("LINE 0,0 100,100")
            assert response.startswith("OK")
    """

    def __init__(self, path: str = PIPE_PATH, timeout: float = PIPE_CONNECT_TIMEOUT) -> None:
        self._path    = path
        self._timeout = timeout
        self._handle  = None   # raw Win32 HANDLE (int) or None

    def __enter__(self) -> "PipeConnection":
        self.open()
        return self

    def __exit__(self, *_) -> None:
        self.close()

    def open(self) -> None:
        """Connect to the pipe server, retrying until *timeout* seconds elapse."""
        self._handle = _connect(self._path, self._timeout)

    def close(self) -> None:
        """Disconnect immediately — CloseHandle fires synchronously."""
        if self._handle is not None:
            _k32.CloseHandle(self._handle)
            self._handle = None

    def send(self, command: str) -> str:
        """Send *command* and return the single response line (newline stripped)."""
        if self._handle is None:
            raise RuntimeError("PipeConnection is not open")

        payload = (command.rstrip("\r\n") + "\n").encode("utf-8")
        written = wt.DWORD(0)
        _k32.WriteFile(self._handle, payload, len(payload), ctypes.byref(written), None)

        # Read one byte at a time until newline (pipe is byte-stream, not message).
        buf = bytearray()
        ch_buf = ctypes.create_string_buffer(1)
        read_count = wt.DWORD(0)
        while True:
            ok = _k32.ReadFile(self._handle, ch_buf, 1, ctypes.byref(read_count), None)
            if not ok or read_count.value == 0:
                break
            b = ch_buf.raw[0]
            if b == ord(b"\n"):
                break
            if b != ord(b"\r"):
                buf.append(b)
        return buf.decode("utf-8")


# ---------------------------------------------------------------------------
# Response parsing helpers (mirrors EoPipeProtocol.h)
# ---------------------------------------------------------------------------

def is_ok(response: str) -> bool:
    """Return True when *response* is an ``OK`` or ``OK <value>`` line."""
    return response == "OK" or response.startswith("OK ")


def is_error(response: str) -> bool:
    """Return True when *response* starts with ``ERROR:``."""
    return response.startswith("ERROR:")


def error_code(response: str) -> str:
    """Extract the ``SCREAMING_SNAKE`` code from an ``ERROR:<CODE> …`` line.

    Returns an empty string when *response* is not an error line.
    """
    if not is_error(response):
        return ""
    rest = response[6:]  # strip "ERROR:"
    return rest.split(" ", 1)[0]


def ok_value(response: str) -> str:
    """Return the value payload from an ``OK <value>`` line, or ``""``."""
    if response.startswith("OK "):
        return response[3:]
    return ""


# ---------------------------------------------------------------------------
# DDE transport (ctypes, no pywin32)
# ---------------------------------------------------------------------------

# DDEML constants
DMLERR_NO_ERROR      = 0x0000
XTYP_EXECUTE         = 0x4050
XTYP_REQUEST         = 0x20B0
CF_TEXT              = 1
DDE_FACKREQ          = 0x0008
TIMEOUT_ASYNC        = 0xFFFFFFFF

_dde = ctypes.windll.user32

# Opaque handle types
HCONV    = ctypes.c_void_p
HSZ      = ctypes.c_void_p
HDDEDATA = ctypes.c_void_p

# The C++ server is compiled with CharacterSet=Unicode, so DdeInitialize
# expands to DdeInitializeW and DdeCreateStringHandle uses CP_WINUNICODE.
# The Python client uses the matching W entry points so that service/topic
# string handles are created in the same Unicode DDEML instance space.
_dde.DdeInitializeW.restype  = ctypes.c_uint
_dde.DdeInitializeW.argtypes = [
    ctypes.POINTER(ctypes.c_ulong), ctypes.c_void_p, ctypes.c_ulong, ctypes.c_ulong
]
_dde.DdeCreateStringHandleW.restype  = HSZ
_dde.DdeCreateStringHandleW.argtypes = [ctypes.c_ulong, ctypes.c_wchar_p, ctypes.c_int]
_dde.DdeFreeStringHandle.argtypes    = [ctypes.c_ulong, HSZ]
_dde.DdeConnect.restype  = HCONV
_dde.DdeConnect.argtypes = [ctypes.c_ulong, HSZ, HSZ, ctypes.c_void_p]
_dde.DdeDisconnect.argtypes = [HCONV]
_dde.DdeClientTransaction.restype  = HDDEDATA
_dde.DdeClientTransaction.argtypes = [
    ctypes.c_void_p, ctypes.c_ulong, HCONV, HSZ,
    ctypes.c_uint, ctypes.c_uint, ctypes.c_ulong,
    ctypes.POINTER(ctypes.c_ulong),
]
_dde.DdeFreeDataHandle.argtypes = [HDDEDATA]
_dde.DdeAccessData.restype  = ctypes.c_void_p
_dde.DdeAccessData.argtypes = [HDDEDATA, ctypes.POINTER(ctypes.c_ulong)]
_dde.DdeUnaccessData.argtypes = [HDDEDATA]
_dde.DdeUninitialize.argtypes = [ctypes.c_ulong]
_dde.DdeGetLastError.restype  = ctypes.c_uint
_dde.DdeGetLastError.argtypes = [ctypes.c_ulong]

# CP_WINUNICODE (1200) — must match the server's DdeCreateStringHandle codepage.
_CP_WINUNICODE = 1200


class DdeConnection:
    """Minimal DDE client for the AeSys ``Commands`` topic.

    Sends execute strings (bracketed AutoCAD-style commands) and optionally
    reads back the ``Result`` item.

    Usage::

        with DdeConnection() as dde:
            dde.execute("[LINE 0,0 100,100]")
    """

    SERVICE = "AeSys"
    TOPIC   = "Commands"
    RESULT_ITEM = "Result"

    def __init__(self) -> None:
        self._inst: ctypes.c_ulong = ctypes.c_ulong(0)
        self._conv: HCONV = None
        self._initialized = False

    def __enter__(self) -> "DdeConnection":
        self.open()
        return self

    def __exit__(self, *_) -> None:
        self.close()

    def open(self) -> None:
        """Initialise DDEML (Unicode instance) and connect to AeSys."""
        err = _dde.DdeInitializeW(
            ctypes.byref(self._inst),
            ctypes.c_void_p(0),   # PFNCALLBACK -- not needed for client-only
            0x00000010,            # APPCMD_CLIENTONLY
            0,
        )
        if err != DMLERR_NO_ERROR:
            raise ConnectionError(f"DdeInitialize failed: 0x{err:04X}")
        self._initialized = True

        # CP_WINUNICODE = 1200; matches the server's DdeCreateStringHandle codepage.
        hsz_svc   = _dde.DdeCreateStringHandleW(self._inst, self.SERVICE, _CP_WINUNICODE)
        hsz_topic = _dde.DdeCreateStringHandleW(self._inst, self.TOPIC,   _CP_WINUNICODE)
        self._conv = _dde.DdeConnect(self._inst, hsz_svc, hsz_topic, None)
        _dde.DdeFreeStringHandle(self._inst, hsz_svc)
        _dde.DdeFreeStringHandle(self._inst, hsz_topic)

        if not self._conv:
            err2 = _dde.DdeGetLastError(self._inst)
            _dde.DdeUninitialize(self._inst)
            self._initialized = False
            raise ConnectionError(
                f"DdeConnect failed (DDEML error 0x{err2:04X}) -- "
                "is AeSys running with the DDE service registered?"
            )

    def close(self) -> None:
        if self._conv:
            _dde.DdeDisconnect(self._conv)
            self._conv = None
        if self._initialized:
            _dde.DdeUninitialize(self._inst)
            self._initialized = False

    def execute(self, command: str, timeout_ms: int = 5000) -> None:
        """Send *command* as a DDE EXECUTE request.

        *command* must follow the bracketed format: ``"[LINE 0,0 100,100]"``.
        Raises ``RuntimeError`` on failure.
        """
        if not self._conv:
            raise RuntimeError("DdeConnection is not open")
        # ProcessExecRequest casts DdeAccessData to wchar_t* so the payload
        # must be UTF-16-LE regardless of whether the DDEML instance is A or W.
        # Commands must use the registered name: [CLI(command text)]
        payload = (command + "\0").encode("utf-16-le")
        buf = ctypes.create_string_buffer(payload, len(payload))
        hdata = _dde.DdeClientTransaction(
            buf,
            len(payload),
            self._conv,
            None,
            0,
            XTYP_EXECUTE,
            timeout_ms,
            None,
        )
        if not hdata:
            err = _dde.DdeGetLastError(self._inst)
            raise RuntimeError(f"DDE EXECUTE failed for: {command!r} (DDEML 0x{err:04X})")
        _dde.DdeFreeDataHandle(hdata)

    def request(self, item: str, timeout_ms: int = 5000) -> str:
        """Send a DDE REQUEST for *item* and return the CF_TEXT value as a string."""
        if not self._conv:
            raise RuntimeError("DdeConnection is not open")
        hsz_item = _dde.DdeCreateStringHandleA(self._inst, item.encode("cp1252"), 1004)
        result_len = ctypes.c_ulong(0)
        hdata = _dde.DdeClientTransaction(
            None, 0,
            self._conv,
            hsz_item,
            CF_TEXT,
            XTYP_REQUEST,
            timeout_ms,
            ctypes.byref(result_len),
        )
        _dde.DdeFreeStringHandle(self._inst, hsz_item)
        if not hdata:
            return ""
        ptr = _dde.DdeAccessData(hdata, None)
        if ptr:
            raw = ctypes.string_at(ptr, result_len.value)
        else:
            raw = b""
        _dde.DdeUnaccessData(hdata)
        _dde.DdeFreeDataHandle(hdata)
        return raw.rstrip(b"\x00").decode("cp1252", errors="replace")
