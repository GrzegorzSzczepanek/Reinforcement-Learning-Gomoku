#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <random>
#include <utility>
#include <vector>

#include "test.h"

enum class Cell : uint8_t { Empty = 0, Player1 = 1, Player2 = 2 };

constexpr Cell operator!(Cell player) {
  switch (player) {
  case Cell::Player1:
    return Cell::Player2;
  case Cell::Player2:
    return Cell::Player1;
  default:
    return player;
  }
}

class Game {
public:
  explicit Game(std::size_t s = 20) : size_(s), board_(s * s, Cell::Empty) {}

  std::size_t size() const { return size_; }
  Cell at(std::size_t x, std::size_t y) const { return board_[y * size_ + x]; }
  void set(std::size_t x, std::size_t y, Cell c) { board_[y * size_ + x] = c; }

  void printCurrentBoard() const {
    for (std::size_t i = 0; i < size(); ++i) {
      for (size_t j = 0; j < size(); ++j) {
        int c = int(at(j, i));
        std::cout << c << " ";
      }
      std::cout << "\n";
    }
  }

  void undoSet(size_t x, size_t y) { board_[x + size_ * y] = Cell::Empty; }

  void printWinningBoard(
      const std::vector<std::pair<int, int>> &winningCords) const {
    const std::string GREEN_BG = "\033[1;42;30m";
    const std::string RESET = "\033[0m";

    for (std::size_t i = 0; i < size(); ++i) {
      for (std::size_t j = 0; j < size(); ++j) {
        bool isWinning = false;
        for (const auto &[wx, wy] : winningCords) {
          if (wx == static_cast<int>(j) && wy == static_cast<int>(i)) {
            isWinning = true;
            break;
          }
        }

        int c = static_cast<int>(at(j, i));
        if (isWinning) {
          std::cout << GREEN_BG << c << RESET << " ";
        } else {
          std::cout << c << " ";
        }
      }
      std::cout << "\n";
    }
  }

  // Aktualny detektor wygranej. Kompaktowy skan w 4 kierunkach, bez alokacji.
  // ~4x szybszy od starego checkWin (ktory wypelnial wektor winningCords na
  // stercie przy kazdym wywolaniu). Uzywany w petli rozgrywki.
  //
  // WYMAGANIE: player != Cell::Empty. hasWon startuje count=1 i szuka
  // sasiadow rownych `player`; wywolane z Empty zaliczyloby ciag pustych pol
  // jako wygrana. W praktyce wolamy je zaraz po set() na biezacym graczu.
  bool hasWon(Cell player, std::size_t x, std::size_t y) const {
    static constexpr int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (auto [dx, dy] : dirs) {
      int count = 1;

      // Krok w strone dodatnia.
      for (int i = 1; i < 5; ++i) {
        std::size_t nx = x + dx * i, ny = y + dy * i;
        if (nx >= size_ || ny >= size_ || at(nx, ny) != player)
          break;
        count++;
      }

      // Krok w strone ujemna.
      for (int i = 1; i < 5; ++i) {
        std::size_t nx = x - dx * i, ny = y - dy * i;
        if (nx >= size_ || ny >= size_ || at(nx, ny) != player)
          break;
        count++;
      }

      if (count >= 5)
        return true;
    }
    return false;
  }

  // STARA wersja — zostawiona zakomentowana jako referencja. Zwracala tez
  // wspolrzedne wygranej linii (winningCords) do podswietlania, ale alokacja
  // wektora na kazde wywolanie robila ja ~4x wolniejsza. Zastapiona przez
  // hasWon(). Helpery checkColumn_/checkRow_/... ponizej tez sa nieuzywane.
  //
  // bool checkWin(Cell player, size_t x, size_t y,
  //               std::vector<std::pair<int, int>> &winningCords) {
  //   return checkColumn_(player, x, y, winningCords) ||
  //          checkRow_(player, x, y, winningCords) ||
  //          checkLeftBottomToRight_(player, x, y, winningCords) ||
  //          checkRightBottomToLeft_(player, x, y, winningCords);
  // }

  std::vector<std::pair<size_t, size_t>> getEmptyBoxes() {
    std::vector<std::pair<size_t, size_t>> res{};
    for (size_t x = 0; x < size(); x++) {
      for (size_t y = 0; y < size(); y++) {
        if (at(x, y) == Cell::Empty) {
          res.push_back({x, y});
        }
      }
    }
    return res;
  }

private:
  std::size_t size_;
  std::vector<Cell> board_;

  bool checkInBounds(size_t x, size_t y) {
    return (x < size()) && (y < size());
  }

  bool checkRightBottomToLeft_(Cell player, size_t x, size_t y,
                               std::vector<std::pair<int, int>> &winningCords) {
    winningCords.clear();
    int count = 0;
    size_t x_ = x;
    size_t y_ = y;

    while (checkInBounds(x_, y_) && at(x_, y_) == player) {
      count++;
      winningCords.push_back({x_, y_});
      x_--;
      y_++;
    }

    x_ = x;
    y_ = y;
    while (checkInBounds(x_, y_) && (at(x_, y_) == player)) {
      count++;
      winningCords.push_back({x_, y_});
      x_++;
      y_--;
    }

    return (count - 1) >= 5;
  }

  bool checkLeftBottomToRight_(Cell player, size_t x, size_t y,
                               std::vector<std::pair<int, int>> &winningCords) {
    winningCords.clear();
    int count = 0;
    size_t x_ = x;
    size_t y_ = y;

    while (checkInBounds(x_, y_) && at(x_, y_) == player) {
      count++;
      winningCords.push_back({x_, y_});
      x_++;
      y_++;
    }

    x_ = x;
    y_ = y;
    while (checkInBounds(x_, y_) && (at(x_, y_) == player)) {
      count++;
      winningCords.push_back({x_, y_});
      x_--;
      y_--;
    }

    return (count - 1) >= 5;
  }

  bool checkRow_(Cell player, size_t x, size_t y,
                 std::vector<std::pair<int, int>> &winningCords) {
    winningCords.clear();
    int count = 0;

    size_t x_ = x;
    while (checkInBounds(x_, y) && (at(x_, y) == player)) {
      count++;
      winningCords.push_back({x_, y});
      x_++;
    }

    x_ = x;
    while (checkInBounds(x_, y) && (at(x_, y) == player)) {
      count++;
      winningCords.push_back({x_, y});
      x_--;
    }

    return (count - 1) >= 5;
  }

  bool checkColumn_(Cell player, size_t x, size_t y,
                    std::vector<std::pair<int, int>> &winningCords) {
    winningCords.clear();
    int count = 0;

    size_t y_ = y;
    while (checkInBounds(x, y_) && (at(x, y_) == player)) {
      count++;
      winningCords.push_back({x, y_});
      y_++;
    }

    y_ = y;
    while (checkInBounds(x, y_) && (at(x, y_) == player)) {
      count++;
      winningCords.push_back({x, y_});
      y_--;
    }

    return (count - 1) >= 5;
  }
};

constexpr std::size_t BOARD_SIZE = 20;
constexpr std::size_t CHANNELS = 3;

using BoardState = std::array<float, CHANNELS * BOARD_SIZE * BOARD_SIZE>;

inline size_t getTensorIndex(size_t c, size_t x, size_t y) {
  return c * (BOARD_SIZE * BOARD_SIZE) + y * BOARD_SIZE + x;
}

int main() {
  std::cout << "RL-Gomoku: start\n";

  Test t(21);
  std::cout << "Test::compute() = " << t.compute() << "\n";
  std::array<std::array<int, 20>, 3> state{};

  for (int i = 0; i < 10; ++i) {
    Game game;
    Cell currentPlayer = Cell::Player1;
    auto boxes = game.getEmptyBoxes();
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(boxes.begin(), boxes.end(), g);

    while (!boxes.empty()) {
      auto [x, y] = boxes.back();
      boxes.pop_back();

      game.set(x, y, currentPlayer);

      if (game.hasWon(currentPlayer, x, y)) {
        std::cout << "Wygral gracz: " << static_cast<int>(currentPlayer)
                  << "\n";
        break;
      }

      currentPlayer = !currentPlayer;
    }

    game.printCurrentBoard();
    std::cout << "-----------------------\n";
  }

  return 0;
}
