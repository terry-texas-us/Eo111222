"""
conftest.py
~~~~~~~~~~~
Shared pytest configuration for the AeSys test suite.

Auto-skip guards
----------------
Both test_pipe.py and test_dde.py require AeSys to be running.  Rather than
letting every test fail with an unreadable OS error, conftest adds two
session-scoped marks:

  - ``requires_pipe``  — skips the entire session when \\\\.\\pipe\\AeSys is not open
  - ``requires_dde``   — skips the entire session when DDE connect fails

Usage in test files
-------------------
Both test files are auto-marked via the ``pytest_collection_modifyitems`` hook
below — no per-test ``@pytest.mark`` decoration is needed.
"""

from __future__ import annotations

import sys
import pathlib

import pytest

# Make the aespy package importable from any working directory.
sys.path.insert(0, str(pathlib.Path(__file__).parent))


def _pipe_available() -> bool:
    try:
        from aespy._connection import PipeConnection
        with PipeConnection(timeout=2.0):
            pass
        return True
    except Exception:
        return False


def _dde_available() -> bool:
    try:
        from aespy._connection import DdeConnection
        with DdeConnection():
            pass
        return True
    except Exception:
        return False


# Evaluate once per session.
_PIPE_UP: bool | None = None
_DDE_UP:  bool | None = None


def _pipe_up() -> bool:
    global _PIPE_UP
    if _PIPE_UP is None:
        _PIPE_UP = _pipe_available()
    return _PIPE_UP


def _dde_up() -> bool:
    global _DDE_UP
    if _DDE_UP is None:
        _DDE_UP = _dde_available()
    return _DDE_UP


def pytest_collection_modifyitems(items: list) -> None:
    """Auto-apply skip marks based on transport availability."""
    skip_pipe = pytest.mark.skip(
        reason="Named pipe not available (pipe\\AeSys) -- is AeSys running?"
    )
    skip_dde = pytest.mark.skip(
        reason="DDE service 'AeSys' not available — is AeSys running?"
    )

    for item in items:
        path = str(item.fspath)
        if "test_pipe" in path and not _pipe_up():
            item.add_marker(skip_pipe)
        if "test_dde" in path and not _dde_up():
            item.add_marker(skip_dde)
