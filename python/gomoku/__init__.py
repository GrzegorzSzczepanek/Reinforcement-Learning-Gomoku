"""Gomoku RL package.

Core (logika gry) jest w C++ i wystawiony jako `gomoku._core`. Reszta
(srodowisko, siec, agenci) zyje tutaj w Pythonie — cienka warstwa nad Core.
"""

from ._core import Cell, Game, WIN_LENGTH, opponent  # noqa: F401
from .env import GomokuEnv, StepResult  # noqa: F401

__all__ = ["Cell", "Game", "WIN_LENGTH", "opponent", "GomokuEnv", "StepResult"]
