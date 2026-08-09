"""Cienkie srodowisko RL wokol Core (gomoku._core.Game).

Celowo trzyma logike epizodu (reset/step/reward/terminacja) TUTAJ, w Pythonie,
a nie w C++ — zeby latwo eksperymentowac z ksztaltem nagrody, self-play itd.
Core dostarcza tylko: legalne ruchy, wykrycie wygranej i tensor obserwacji.

Akcje sa indeksami plaskimi: action = y * size + x  (spojne z legal_moves()).
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ._core import Cell, Game, opponent


@dataclass
class StepResult:
    """Wynik pojedynczego kroku srodowiska (styl Gym, ale jako dataclass)."""

    obs: np.ndarray          # [3, size, size] float32 — z perspektywy gracza NA RUCHU
    reward: float            # nagroda dla gracza, ktory WYKONAL ten ruch
    done: bool               # czy epizod sie skonczyl
    info: dict               # dodatki: winner, last_move, itp.


class GomokuEnv:
    """Dwuosobowe, turowe srodowisko Gomoku.

    Konwencja perspektywy: obserwacja jest zawsze budowana z punktu widzenia
    gracza, ktory ma teraz ruch (kanal 0 = jego kamienie). Dzieki temu jedna
    siec obsluguje obu graczy (self-play).
    """

    def __init__(self, size: int = 20):
        self.size = size
        self._game = Game(size)
        self.to_move: Cell = Cell.Player1
        self.done: bool = False
        self.winner: Cell = Cell.Empty

    # -- API srodowiska ----------------------------------------------------

    def reset(self) -> np.ndarray:
        self._game.clear()
        self.to_move = Cell.Player1
        self.done = False
        self.winner = Cell.Empty
        return self.observation()

    def observation(self) -> np.ndarray:
        """Obserwacja z perspektywy gracza na ruchu, [3, size, size] float32."""
        return self._game.state_tensor(self.to_move)

    def legal_moves(self) -> np.ndarray:
        """Indeksy plaskie legalnych ruchow (do maskowania polityki)."""
        return np.asarray(self._game.legal_moves(), dtype=np.int64)

    def legal_mask(self) -> np.ndarray:
        """Maska bool dlugosci size*size: True = ruch dozwolony."""
        mask = np.zeros(self.size * self.size, dtype=bool)
        mask[self.legal_moves()] = True
        return mask

    def step(self, action: int) -> StepResult:
        """Wykonuje ruch gracza na ruchu. `action` = y * size + x."""
        if self.done:
            raise RuntimeError("step() po zakonczeniu epizodu; wywolaj reset().")

        x, y = action % self.size, action // self.size
        if not self._game.is_legal(x, y):
            # Nielegalny ruch: karzemy i konczymy epizod. Alternatywnie mozna
            # maskowac polityke wczesniej, ale tu jest twardy bezpiecznik.
            self.done = True
            return StepResult(
                obs=self.observation(),
                reward=-1.0,
                done=True,
                info={"illegal": True, "action": action},
            )

        mover = self.to_move
        self._game.set(x, y, mover)

        if self._game.has_won(mover, x, y):
            self.done = True
            self.winner = mover
            reward = 1.0
        elif self._game.is_full():
            self.done = True
            reward = 0.0  # remis
        else:
            reward = 0.0

        # obserwacja PO ruchu jest juz z perspektywy nastepnego gracza
        self.to_move = opponent(self.to_move)
        obs = self.observation()

        return StepResult(
            obs=obs,
            reward=reward,
            done=self.done,
            info={"winner": self.winner, "last_move": (x, y), "by": mover},
        )

    # -- pomocnicze --------------------------------------------------------

    def render(self) -> str:
        """ASCII plansza: . puste, X Player1, O Player2."""
        glyph = {Cell.Empty: ".", Cell.Player1: "X", Cell.Player2: "O"}
        rows = []
        for y in range(self.size):
            rows.append(" ".join(glyph[self._game.at(x, y)] for x in range(self.size)))
        return "\n".join(rows)
