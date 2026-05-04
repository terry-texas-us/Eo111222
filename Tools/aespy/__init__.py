"""
aespy — thin Python client for the AeSys CAD application.

Transports
----------
- Named pipe  (``\\\\.\\pipe\\AeSys``)  via aespy._connection.PipeConnection
- DDE          (service "AeSys", topic "Commands")  via aespy._connection.DdeConnection

No pywin32 dependency.  Requires Python 3.10+ on Windows.
"""

from ._connection import PipeConnection, is_ok, is_error, error_code, ok_value
from ._queries import (
    query_trap,
    query_block,
    query_blocks,
    query_room,
    query_rooms,
    query_layer,
    query_layers,
)

__all__ = [
    "PipeConnection",
    "is_ok",
    "is_error",
    "error_code",
    "ok_value",
    "query_trap",
    "query_block",
    "query_blocks",
    "query_room",
    "query_rooms",
    "query_layer",
    "query_layers",
]
