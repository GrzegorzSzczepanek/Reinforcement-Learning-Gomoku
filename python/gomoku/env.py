"""Szkielet środowiska RL wokół Core (gomoku._core.Game).

To jest TEMPLATE — rusztowanie do wypełnienia. Metody są stubami z TODO;
logikę epizodu (reset / step / nagroda / terminacja / perspektywa) piszesz sam.

Co daje Ci Core (gotowe, patrz gomoku._core.Game):
    game.size()                      -> int
    game.at(x, y)                    -> Cell
    game.set(x, y, cell)             -> None
    game.undo_set(x, y)              -> None
    game.is_legal(x, y)             -> bool
    game.legal_moves()              -> list[int]   (indeksy y*size + x)
    game.is_full()                  -> bool
    game.has_won(player, x, y)      -> bool         (player != Cell.Empty)
    game.clear()                    -> None
    game.state_tensor(to_move)      -> np.ndarray [3, size, size] float32
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ._core import Cell, Game, opponent  # noqa: F401  (opponent bywa przydatny)


@dataclass
class StepResult:
    """Wynik jednego kroku. Pola dobierz pod swój algorytm."""

    obs: np.ndarray
    reward: float
    done: bool
    info: dict


class GomokuEnv:
    """Dwuosobowe, turowe środowisko Gomoku. Wypełnij metody poniżej."""

    def __init__(self, size: int = 20):
        self.size = size
        self._game = Game(size)
        self.to_move: Cell = Cell.Player1
        # TODO: dodaj własny stan epizodu (done, winner, licznik ruchów, ...).

    def reset(self) -> np.ndarray:
        """Wyczyść planszę, ustaw gracza startowego, zwróć pierwszą obserwację."""
        # self._game.clear()
        # self.to_move = Cell.Player1
        # return self.observation()
        raise NotImplementedError("TODO: reset()")

    def observation(self) -> np.ndarray:
        """Obserwacja [3, size, size] float32 z perspektywy gracza na ruchu."""
        # return self._game.state_tensor(self.to_move)
        raise NotImplementedError("TODO: observation()")

    def step(self, action: int) -> StepResult:
        """Wykonaj ruch (action = y*size + x), policz nagrodę i terminację.

        Szkielet do wypełnienia — rozkodowanie akcji i wygrana są za darmo:
            x, y = action % self.size, action // self.size
            self._game.set(x, y, self.to_move)
            won = self._game.has_won(self.to_move, x, y)
            ...
            self.to_move = opponent(self.to_move)
        """
        raise NotImplementedError("TODO: step()")

    # -- pomocnicze (opcjonalne, ale zwykle się przydają) ------------------

    def legal_moves(self) -> np.ndarray:
        """Indeksy płaskie legalnych ruchów (do maskowania polityki)."""
        return np.asarray(self._game.legal_moves(), dtype=np.int64)

    def legal_mask(self) -> np.ndarray:
        """Maska bool długości size*size: True = ruch dozwolony."""
        mask = np.zeros(self.size * self.size, dtype=bool)
        mask[self.legal_moves()] = True
        return mask

    def render(self) -> str:
        """ASCII plansza: . puste, X Player1, O Player2."""
        glyph = {Cell.Empty: ".", Cell.Player1: "X", Cell.Player2: "O"}
        rows = []
        for y in range(self.size):
            rows.append(" ".join(glyph[self._game.at(x, y)] for x in range(self.size)))
        return "\n".join(rows)
