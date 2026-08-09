// gomoku/game.hpp — czysta logika gry Gomoku (Core).
//
// Zero zaleznosci od PyTorch/pybind — to ma sie kompilowac samo jako biblioteka
// C++ i byc testowalne w izolacji. Warstwa RL (Python) i binding stoja OBOK,
// nie w srodku. Rozmiar planszy jest konfigurowalny w runtime.

#ifndef GOMOKU_GAME_HPP
#define GOMOKU_GAME_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gomoku {

enum class Cell : std::uint8_t { Empty = 0, Player1 = 1, Player2 = 2 };

// Przeciwnik danego gracza. Empty -> Empty.
constexpr Cell opponent(Cell player) {
  switch (player) {
  case Cell::Player1:
    return Cell::Player2;
  case Cell::Player2:
    return Cell::Player1;
  default:
    return player;
  }
}

// Liczba kamieni w linii potrzebna do wygranej (klasyczne gomoku: 5).
inline constexpr int WIN_LENGTH = 5;

class Game {
public:
  explicit Game(std::size_t size = 20);

  std::size_t size() const { return size_; }

  Cell at(std::size_t x, std::size_t y) const {
    return board_[y * size_ + x];
  }
  void set(std::size_t x, std::size_t y, Cell c) {
    board_[y * size_ + x] = c;
  }
  void undoSet(std::size_t x, std::size_t y) {
    board_[y * size_ + x] = Cell::Empty;
  }

  // Czy pole w granicach planszy i puste (legalny ruch).
  bool isLegal(std::size_t x, std::size_t y) const {
    return x < size_ && y < size_ && at(x, y) == Cell::Empty;
  }

  // Wszystkie puste pola jako indeksy (y * size + x) — wygodne jako maska akcji.
  std::vector<int> legalMoves() const;

  // Czy plansza jest pelna (remis, gdy nikt nie wygral).
  bool isFull() const;

  // Detektor wygranej: skan 4 kierunkow od (x, y), bez alokacji.
  // WYMAGANIE: player != Cell::Empty (wolane po ruchu na biezacym graczu).
  bool hasWon(Cell player, std::size_t x, std::size_t y) const;

  // Reset planszy do pustej (bez realokacji, gdy rozmiar sie nie zmienia).
  void clear();

  // Obserwacja NCHW dla sieci: 3 kanaly [C, size, size], row-major.
  //   kanal 0: kamienie gracza `toMove`
  //   kanal 1: kamienie przeciwnika
  //   kanal 2: stała plaszczyzna (1.0 gdy toMove == Player1, else 0.0)
  // Zwraca splaszczony wektor dlugosci 3 * size * size.
  std::vector<float> stateTensor(Cell toMove) const;

  // Ksztalt tensora z stateTensor(): {3, size, size}. Pomocne dla bindingu.
  std::array<std::size_t, 3> stateShape() const { return {3, size_, size_}; }

private:
  std::size_t size_;
  std::vector<Cell> board_;
};

} // namespace gomoku

#endif // GOMOKU_GAME_HPP
