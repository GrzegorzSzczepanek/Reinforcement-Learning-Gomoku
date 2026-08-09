"""Szkielet sieci (PyTorch) pod Gomoku — styl AlphaZero: policy + value.

To jest GOTOWIEC do podmiany: architektura jest minimalna (kilka blokow
konwolucyjnych + dwie glowy). Wejscie pasuje do env.observation(): [B, 3, S, S].
Wyjscie:
  - policy_logits: [B, S*S]  (logity nad wszystkimi polami; maskuj legal_mask)
  - value:         [B]       (ocena pozycji w [-1, 1], tanh)

Zaleznosc od torch jest importowana leniwie, zeby reszta pakietu (env, Core)
dzialala bez zainstalowanego PyTorch.
"""

from __future__ import annotations

import torch
import torch.nn as nn
import torch.nn.functional as F


class ConvBlock(nn.Module):
    def __init__(self, in_ch: int, out_ch: int):
        super().__init__()
        self.conv = nn.Conv2d(in_ch, out_ch, kernel_size=3, padding=1, bias=False)
        self.bn = nn.BatchNorm2d(out_ch)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return F.relu(self.bn(self.conv(x)))


class ResBlock(nn.Module):
    def __init__(self, ch: int):
        super().__init__()
        self.c1 = nn.Conv2d(ch, ch, 3, padding=1, bias=False)
        self.b1 = nn.BatchNorm2d(ch)
        self.c2 = nn.Conv2d(ch, ch, 3, padding=1, bias=False)
        self.b2 = nn.BatchNorm2d(ch)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        y = F.relu(self.b1(self.c1(x)))
        y = self.b2(self.c2(y))
        return F.relu(x + y)


class GomokuNet(nn.Module):
    """Minimalna siec policy+value. Podmien channels/blocks wedle uznania."""

    def __init__(self, board_size: int = 20, in_ch: int = 3,
                 channels: int = 64, blocks: int = 4):
        super().__init__()
        self.board_size = board_size
        self.stem = ConvBlock(in_ch, channels)
        self.body = nn.Sequential(*[ResBlock(channels) for _ in range(blocks)])

        # Glowa polityki: 1x1 conv -> logity nad polami.
        self.policy_conv = nn.Conv2d(channels, 2, kernel_size=1)
        self.policy_bn = nn.BatchNorm2d(2)
        self.policy_fc = nn.Linear(2 * board_size * board_size,
                                   board_size * board_size)

        # Glowa wartosci: 1x1 conv -> skalar w [-1, 1].
        self.value_conv = nn.Conv2d(channels, 1, kernel_size=1)
        self.value_bn = nn.BatchNorm2d(1)
        self.value_fc1 = nn.Linear(board_size * board_size, 64)
        self.value_fc2 = nn.Linear(64, 1)

    def forward(self, x: torch.Tensor):
        x = self.stem(x)
        x = self.body(x)

        p = F.relu(self.policy_bn(self.policy_conv(x)))
        p = p.flatten(1)
        policy_logits = self.policy_fc(p)

        v = F.relu(self.value_bn(self.value_conv(x)))
        v = v.flatten(1)
        v = F.relu(self.value_fc1(v))
        value = torch.tanh(self.value_fc2(v)).squeeze(-1)

        return policy_logits, value

    @staticmethod
    def masked_policy(logits: torch.Tensor, legal_mask: torch.Tensor) -> torch.Tensor:
        """Zwraca rozklad prawdopodobienstwa tylko nad legalnymi ruchami."""
        neg_inf = torch.finfo(logits.dtype).min
        masked = torch.where(legal_mask, logits, torch.full_like(logits, neg_inf))
        return F.softmax(masked, dim=-1)
