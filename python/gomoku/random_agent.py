"""Smoke-test Core: losowa rozgrywka bezpośrednio na gomoku._core.Game.

NIE używa GomokuEnv (to Twój szkielet do wypełnienia) — sprawdza tylko, że
Core + binding działają end-to-end: legal_moves, set, has_won, is_full.

Uruchom:  python -m gomoku.random_agent
"""

from __future__ import annotations

import numpy as np

from ._core import Cell, Game, opponent


def play_one(size: int = 15, seed: int | None = None) -> dict:
    rng = np.random.default_rng(seed)
    game = Game(size)
    to_move = Cell.Player1

    moves = 0
    winner = Cell.Empty
    while True:
        legal = game.legal_moves()
        if not legal:
            break  # remis — pełna plansza
        action = int(rng.choice(legal))
        x, y = action % size, action // size
        game.set(x, y, to_move)
        moves += 1
        if game.has_won(to_move, x, y):
            winner = to_move
            break
        to_move = opponent(to_move)

    return {"moves": moves, "winner": winner}


def main() -> None:
    for i in range(5):
        r = play_one(size=15, seed=i)
        print(f"gra {i}: ruchow={r['moves']:3d}  zwyciezca={r['winner']}")
    print("OK — Core + binding dzialaja.")


if __name__ == "__main__":
    main()
