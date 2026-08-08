#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <random>
#include <utility>
#include <vector>

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

  bool checkWin(Cell player, size_t x, size_t y) {
    return checkColumn_(player, x, y) || checkRow_(player, x, y) ||
           checkLeftBottomToRight_(player, x, y);
  }

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

  bool checkLeftBottomToRight_(Cell player, size_t x, size_t y) {
    int count = 0;
    size_t x_ = x;
    size_t y_ = y;

    while (checkInBounds(x_, y_) && at(x_, y_) == player) {
      count++;
      x_++;
      y_++;
    }

    x_ = x;
    y_ = y;
    while (checkInBounds(x_, y_) && (at(x_, y_) == player)) {
      count++;
      x_--;
      y_--;
    }

    return (count - 1) >= 5;
  }

  bool checkRow_(Cell player, size_t x, size_t y) {
    int count = 0;

    size_t x_ = x;
    while (checkInBounds(x_, y) && (at(x_, y) == player)) {
      count++;
      x_++;
    }

    x_ = x;
    while (checkInBounds(x_, y) && (at(x_, y) == player)) {
      count++;
      x_--;
    }

    return (count - 1) >= 5;
  }

  bool checkColumn_(Cell player, size_t x, size_t y) {
    int count = 0;

    size_t y_ = y;
    while (checkInBounds(x, y_) && (at(x, y_) == player)) {
      count++;
      y_++;
    }

    y_ = y;
    while (checkInBounds(x, y_) && (at(x, y_) == player)) {
      count++;
      y_--;
    }

    return (count - 1) >= 5;
  }
};

int main() {
  std::cout << "RL-Gomoku: start\n";

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

      if (game.checkWin(currentPlayer, x, y)) {
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
