"""Sanity self-play losowym agentem — nie wymaga PyTorch.

Uruchom:  python -m gomoku.random_agent
Sprawdza, ze Core + env dzialaja end-to-end (legal_moves, step, terminacja).
"""

from __future__ import annotations

import numpy as np

from .env import Cell, GomokuEnv


def play_one(size: int = 15, seed: int | None = None) -> dict:
    rng = np.random.default_rng(seed)
    env = GomokuEnv(size)
    env.reset()

    moves = 0
    result = None
    while True:
        legal = env.legal_moves()
        action = int(rng.choice(legal))
        step = env.step(action)
        moves += 1
        if step.done:
            result = step.info
            break

    winner = result.get("winner", Cell.Empty)
    return {"moves": moves, "winner": winner}


def main() -> None:
    n = 5
    for i in range(n):
        r = play_one(size=15, seed=i)
        print(f"gra {i}: ruchow={r['moves']:3d}  zwyciezca={r['winner']}")
    print("OK — Core + env dzialaja.")


if __name__ == "__main__":
    main()
