"""
aespy — thin Python client for the AeSys CAD application.

Transports
----------
- Named pipe  (\\.\pipe\AeSys)  via aespy._connection.PipeConnection
- DDE          (service "AeSys", topic "Commands")  via aespy._connection.DdeConnection

No pywin32 dependency.  Requires Python 3.10+ on Windows.
"""
