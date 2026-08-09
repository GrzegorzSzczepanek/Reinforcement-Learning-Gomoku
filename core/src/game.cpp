#include "gomoku/game.hpp"

#include <algorithm>

namespace gomoku {

Game::Game(std::size_t size)
    : size_(size), board_(size * size, Cell::Empty) {}

void Game::clear() {
  std::fill(board_.begin(), board_.end(), Cell::Empty);
}

bool Game::isFull() const {
  for (Cell c : board_)
    if (c == Cell::Empty)
      return false;
  return true;
}

std::vector<int> Game::legalMoves() const {
  std::vector<int> moves;
  moves.reserve(board_.size());
  for (std::size_t y = 0; y < size_; ++y)
    for (std::size_t x = 0; x < size_; ++x)
      if (at(x, y) == Cell::Empty)
        moves.push_back(static_cast<int>(y * size_ + x));
  return moves;
}

bool Game::hasWon(Cell player, std::size_t x, std::size_t y) const {
  static constexpr int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

  for (auto [dx, dy] : dirs) {
    int count = 1;

    for (int i = 1; i < WIN_LENGTH; ++i) {
      std::size_t nx = x + dx * i, ny = y + dy * i;
      if (nx >= size_ || ny >= size_ || at(nx, ny) != player)
        break;
      count++;
    }

    for (int i = 1; i < WIN_LENGTH; ++i) {
      std::size_t nx = x - dx * i, ny = y - dy * i;
      if (nx >= size_ || ny >= size_ || at(nx, ny) != player)
        break;
      count++;
    }

    if (count >= WIN_LENGTH)
      return true;
  }
  return false;
}

std::vector<float> Game::stateTensor(Cell toMove) const {
  const Cell other = opponent(toMove);
  const std::size_t plane = size_ * size_;
  std::vector<float> out(3 * plane, 0.0f);

  const float turnValue = (toMove == Cell::Player1) ? 1.0f : 0.0f;
  for (std::size_t i = 0; i < plane; ++i) {
    Cell c = board_[i];
    if (c == toMove)
      out[i] = 1.0f;              // kanal 0: moje kamienie
    else if (c == other)
      out[plane + i] = 1.0f;      // kanal 1: przeciwnik
    out[2 * plane + i] = turnValue; // kanal 2: kto na ruchu
  }
  return out;
}

} // namespace gomoku
