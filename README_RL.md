# RL-Gomoku — Core (C++) + RL flow (Python / PyTorch)

Gotowy szkielet do RL na Gomoku. Logika gry jest w **C++** (szybka, testowalna
w izolacji), a warstwa uczenia w **Pythonie** (PyTorch). Most między nimi to
**pybind11**. Ty piszesz algorytm RL; Core, binding i środowisko już stoją.

---

## Spis treści

1. [Architektura i filozofia](#architektura-i-filozofia)
2. [Układ plików](#układ-plików)
3. [Instalacja i build](#instalacja-i-build)
4. [Szybki start](#szybki-start)
5. [API Core (C++)](#api-core-c)
6. [API Python](#api-python)
7. [Format obserwacji (state_tensor)](#format-obserwacji-state_tensor)
8. [Konwencja akcji i perspektywy](#konwencja-akcji-i-perspektywy)
9. [Testy](#testy)
10. [Troubleshooting](#troubleshooting)
11. [Jak to rozbudowywać](#jak-to-rozbudowywać)

---

## Architektura i filozofia

```
   ┌──────────────┐   pybind11   ┌──────────────┐   PyTorch   ┌──────────────┐
   │  Core (C++)  │ ───────────► │  _core (.so) │ ──────────► │  RL flow     │
   │  gomoku_core │              │  gomoku._core│             │  env + net   │
   └──────────────┘              └──────────────┘             └──────────────┘
   czysta logika gry             cienki binding               Twój kod RL
   (bez Pythona)                 (bez logiki RL)              (Python)
```

Dwie decyzje projektowe, o które prosiłeś:

- **Cienki binding.** Core wystawia tylko czystą grę: legalne ruchy, wykrycie
  wygranej, tensor obserwacji. Logika epizodu (`reset` / `step` / nagroda /
  terminacja) żyje w **Pythonie** (`env.py`). Efekt: zmiana kształtu nagrody
  albo reguł self-play = edycja Pythona, **bez rekompilacji C++**.
- **Rozmiar planszy w runtime.** `Game(size)` — możesz trenować na 9×9, grać na
  15×15, testować na 5×5. State to `std::vector<float>`, kształt liczony z
  `size()`.

---

## Układ plików

```
core/                       # C++ Core — czysta logika gry
  include/gomoku/game.hpp    #   deklaracje: Cell, opponent(), Game
  src/game.cpp               #   definicje

bindings/
  module.cpp                 # pybind11: Game -> Python, state_tensor -> numpy

python/gomoku/
  __init__.py                # eksportuje Cell, Game, GomokuEnv, ...
  env.py                     # GomokuEnv: reset/step/reward/self-play (w Pythonie)
  net.py                     # GomokuNet: CNN policy+value (PyTorch) — do podmiany
  random_agent.py            # sanity self-play losowym agentem (bez torcha)

src/main.cpp                 # Twój C++ playground (binarka `gomoku`) — nietknięty
src/test.h, src/test.cpp     # przykład podziału header/source (moduły)
tests/test_game.cpp          # testy Core (mini-framework)
run_tests.sh                 # kompiluje + uruchamia testy

CMakeLists.txt               # buduje: gomoku_core + _core (pybind) + gomoku (binarka)
pyproject.toml               # scikit-build-core: `pip install -e .`
```

---

## Instalacja i build

Są **dwie niezależne ścieżki** builda. Nie kolidują ze sobą.

### A) Ścieżka Pythona / RL (główna)

Buduje moduł C++ `_core` i instaluje pakiet `gomoku` jako *editable*:

```bash
pip install -e .
```

PyTorch **nie** jest twardą zależnością (żebyś zainstalował wariant pod swój
sprzęt: CUDA / CPU / MPS). Domyślny wheel dołożysz przez extras:

```bash
pip install -e ".[torch]"
```

Wymagania build-time (ściąga je automatycznie `pip` z `pyproject.toml`):
`scikit-build-core`, `pybind11`, `numpy`, CMake ≥ 3.16.

### B) Ścieżka czystego C++ (playground + testy)

Nie wymaga Pythona. Buduje bibliotekę Core i Twoją binarkę `gomoku`:

```bash
cmake -S . -B build
cmake --build build
./build/gomoku          # uruchom playground
```

CMake wykrywa, czy budujesz przez `pip` (scikit-build ustawia `SKBUILD`).
Bez tego binarka `gomoku` się buduje; z tym — tylko moduł `_core`. Jeśli
pybind11 nie jest zainstalowany, moduł `_core` jest po prostu pomijany
(zobaczysz o tym komunikat), a Core i binarka i tak się zbudują.

---

## Szybki start

```python
import torch
from gomoku import GomokuEnv
from gomoku.net import GomokuNet

env = GomokuEnv(size=15)
net = GomokuNet(board_size=15)

obs = env.reset()                      # [3, 15, 15] float32, perspektywa gracza na ruchu

x = torch.from_numpy(obs).unsqueeze(0) # [1, 3, 15, 15]
logits, value = net(x)                 # policy_logits [1, 225], value [1]

mask = torch.from_numpy(env.legal_mask()).unsqueeze(0)
probs = GomokuNet.masked_policy(logits, mask)   # rozkład tylko po legalnych

step = env.step(int(probs.argmax()))   # StepResult(obs, reward, done, info)
print(step.reward, step.done, step.info)
```

Sanity check bez PyTorch:

```bash
python -m gomoku.random_agent
```

---

## API Core (C++)

Namespace `gomoku`. Nagłówek: `core/include/gomoku/game.hpp`.

```cpp
enum class Cell : uint8_t { Empty = 0, Player1 = 1, Player2 = 2 };
constexpr Cell opponent(Cell player);   // Empty -> Empty
constexpr int  WIN_LENGTH = 5;

class Game {
  explicit Game(std::size_t size = 20);

  std::size_t size() const;
  Cell at(std::size_t x, std::size_t y) const;
  void set(std::size_t x, std::size_t y, Cell c);
  void undoSet(std::size_t x, std::size_t y);     // stawia Empty (przydatne w MCTS)

  bool isLegal(std::size_t x, std::size_t y) const;   // w granicach && puste
  std::vector<int> legalMoves() const;                // indeksy y*size + x
  bool isFull() const;

  // Detektor wygranej: skan 4 kierunków od (x, y), bez alokacji.
  // WYMAGANIE: player != Cell::Empty (patrz niżej).
  bool hasWon(Cell player, std::size_t x, std::size_t y) const;

  void clear();                                       // reset do pustej planszy

  // Obserwacja NCHW [3, size, size], row-major (patrz "Format obserwacji").
  std::vector<float> stateTensor(Cell toMove) const;
  std::array<std::size_t, 3> stateShape() const;      // {3, size, size}
};
```

> **Kontrakt `hasWon`:** wołaj z `player != Empty`. Metoda startuje `count = 1`
> i szuka sąsiadów równych `player`; wywołana z `Empty` zaliczyłaby ciąg pustych
> pól jako „wygraną”. W praktyce wołasz ją zaraz po `set()` na graczu, który
> właśnie zagrał — więc nigdy z `Empty`.

---

## API Python

### `gomoku` (pakiet)

```python
from gomoku import Cell, Game, WIN_LENGTH, opponent   # z Core (_core)
from gomoku import GomokuEnv, StepResult              # z env.py
```

### `GomokuEnv(size=20)` — `python/gomoku/env.py`

Dwuosobowe, turowe środowisko. Cała logika epizodu jest tutaj (nie w C++).

| Metoda | Zwraca | Opis |
|---|---|---|
| `reset()` | `np.ndarray [3,S,S]` | Czyści planszę, Player1 na ruchu, zwraca obserwację. |
| `observation()` | `np.ndarray [3,S,S]` | Obserwacja z perspektywy gracza na ruchu. |
| `legal_moves()` | `np.ndarray int64` | Indeksy płaskie legalnych ruchów. |
| `legal_mask()` | `np.ndarray bool [S*S]` | Maska: `True` = ruch dozwolony. |
| `step(action)` | `StepResult` | Wykonuje ruch `action = y*S + x`. |
| `render()` | `str` | ASCII planszy (`.` puste, `X` P1, `O` P2). |

`StepResult` (dataclass):

```python
obs:    np.ndarray   # [3,S,S] float32 — z perspektywy NASTĘPNEGO gracza
reward: float        # nagroda dla gracza, który wykonał ten ruch (+1 wygrana, 0 inaczej, -1 ruch nielegalny)
done:   bool         # czy epizod się skończył (wygrana / pełna plansza / ruch nielegalny)
info:   dict         # {"winner": Cell, "last_move": (x,y), "by": Cell}  lub  {"illegal": True, ...}
```

Uwagi o nagrodzie/terminacji (edytowalne w `env.py`, to tylko domyślne):
- wygrana ruchem → `reward = +1.0`, `done = True`
- pełna plansza bez wygranej (remis) → `reward = 0.0`, `done = True`
- ruch na zajęte/poza planszą → `reward = -1.0`, `done = True` (twardy bezpiecznik;
  zwykle maskujesz to wcześniej przez `legal_mask()`)

### `GomokuNet` — `python/gomoku/net.py`

Minimalna sieć policy+value w stylu AlphaZero. **Do podmiany** — traktuj jako
punkt startowy.

```python
GomokuNet(board_size=20, in_ch=3, channels=64, blocks=4)

forward(x) -> (policy_logits, value)
#   x:             [B, 3, S, S]
#   policy_logits: [B, S*S]      (logity nad wszystkimi polami)
#   value:         [B]           (ocena pozycji w [-1, 1], tanh)

GomokuNet.masked_policy(logits, legal_mask) -> probs
#   maskuje nielegalne pola (-inf) i zwraca softmax tylko po legalnych
```

`import torch` jest w `net.py` na górze — reszta pakietu (`Core`, `env`,
`random_agent`) działa **bez** zainstalowanego PyTorch.

---

## Format obserwacji (`state_tensor`)

NCHW `[3, size, size]` float32, row-major, z perspektywy gracza **na ruchu**:

| Kanał | Zawartość |
|---|---|
| 0 | moje kamienie (gracza na ruchu) — 1.0 / 0.0 |
| 1 | kamienie przeciwnika — 1.0 / 0.0 |
| 2 | stała płaszczyzna: 1.0 gdy na ruchu jest `Player1`, w przeciwnym razie 0.0 |

Kanał 2 to wskaźnik tury — dzięki niemu **jedna sieć obsługuje obu graczy**
(self-play), bo obserwacja jest zawsze „z mojego punktu widzenia”.

---

## Konwencja akcji i perspektywy

- **Akcja** to indeks płaski: `action = y * size + x`. Spójne z `legal_moves()`
  i wymiarem `policy_logits` (`S*S`). Rozkodowanie: `x = action % size`,
  `y = action // size`.
- **Perspektywa.** `observation()` jest zawsze budowana z punktu widzenia
  gracza na ruchu (kanał 0 = jego kamienie). Po `step()` zwrócony `obs` jest już
  z perspektywy następnego gracza. `reward` w tym samym `StepResult` dotyczy
  gracza, który właśnie **wykonał** ruch — przy uczeniu self-play pamiętaj o tym
  przeciwstawnym znaku między kolejnymi półruchami.

---

## Testy

Testy Core (C++), mini-framework bez zależności:

```bash
./run_tests.sh
```

Pokrywają: podstawy planszy, `hasWon` we wszystkich 4 kierunkach, długie linie,
rogi, małe plansze, `getEmptyBoxes`/legalne ruchy, operator przełączania gracza.

> Uwaga: `tests/test_game.cpp` testuje `Game` z `src/main.cpp` (Twój playground),
> nie Core z `core/`. Gdy zdecydujesz się scalić `main.cpp` na `#include
> "gomoku/game.hpp"`, testy warto przełączyć na Core (patrz niżej).

---

## Troubleshooting

**`ImportError` / niepasujące ABI po `pip install -e .`**
Moduł `.so` musi być zbudowany pod tę samą wersję Pythona, której używasz do
importu. CMake wymusza interpreter przez `find_package(Python ...)` +
`Python_EXECUTABLE`. Jeśli masz kilka Pythonów (pyenv, system), buduj i
importuj tym samym:
```bash
which python3            # upewnij się, że to ten aktywny
pip install -e . --force-reinstall --no-build-isolation
```

**`pybind11 nie znaleziony — pomijam modul _core`**
To komunikat z czystego builda CMake (ścieżka B) bez pybind11 — normalne. Do
zbudowania modułu użyj `pip install -e .` (ścieżka A), która dostarcza pybind11.

**`import torch` się wywala, a chcę tylko env**
`env` i `random_agent` nie potrzebują torcha. Torcha wymaga dopiero `net.py`.
Zainstaluj go osobno pod swój sprzęt lub `pip install -e ".[torch]"`.

**Zmieniłem `core/*.cpp` i nie widać efektu w Pythonie**
Przebuduj moduł: `pip install -e . --no-build-isolation` (albo usuń `build/`).

---

## Jak to rozbudowywać

- **Nowa metoda w Core** → dopisz w `core/include/gomoku/game.hpp` +
  `core/src/game.cpp`, wystaw w `bindings/module.cpp` (`.def(...)`), przebuduj
  `pip install -e .`.
- **Inny kształt nagrody / reguły** → tylko `env.py`, bez dotykania C++.
- **Inna sieć** → `net.py` (albo własny moduł); kontrakt wejścia/wyjścia:
  `[B,3,S,S] -> (policy_logits [B,S*S], value [B])`.
- **MCTS / self-play** → `undo_set` w Core jest pod to przygotowany (szybkie
  cofanie ruchu bez kopiowania planszy).
- **Jedno źródło prawdy dla `Game`** → obecnie `src/main.cpp` ma własną kopię
  klasy `Game` (Twój playground), a `core/` ma wersję biblioteczną. Gdy zechcesz,
  można przełączyć `main.cpp` na `#include "gomoku/game.hpp"` i usunąć duplikat
  — wtedy binarka, testy i binding korzystają z jednego Core.
```
