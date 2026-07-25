"""Exceptions for the suprafit Python wrapper. Claude Generated."""

from __future__ import annotations


class SupraFitError(Exception):
    """Base class for all suprafit wrapper errors."""


class SupraFitNotFoundError(SupraFitError):
    """The `suprafit_cli` executable could not be located. Lists the searched paths."""


class ModelNameError(SupraFitError):
    """An unknown model name/id was passed to add_model()."""


class CLIExecutionError(SupraFitError):
    """`suprafit_cli` exited non-zero. Captures the command, exit code, and stderr tail."""

    def __init__(self, message: str, *, cmd=None, returncode=None, stderr: str = ""):
        super().__init__(message)
        self.cmd = cmd
        self.returncode = returncode
        self.stderr = stderr


class ResultParseError(SupraFitError):
    """The output project JSON did not contain the expected model/method blocks."""