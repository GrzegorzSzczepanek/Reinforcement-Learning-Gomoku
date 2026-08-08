// Testy logiki gry z src/main.cpp.
//
// Trik: run_tests.sh kompiluje ten plik z flagą -Dmain=game_main, więc gdy
// dołączamy main.cpp, jego funkcja `main` zostaje przemianowana na `game_main`
// i nie koliduje z main() zdefiniowanym tutaj. Dzięki temu testujemy dokładnie
// ten sam kod (klasę Game), który wchodzi do binarki gry — bez duplikacji.
//
// Mini-framework: brak zewnętrznych zależności (żadnego GoogleTest itp.).
// Każdy CHECK zapisuje wynik, na końcu drukujemy podsumowanie i zwracamy
// kod wyjścia != 0 jeśli cokolwiek nie przeszło (przydatne w CI / pre-commit).

#include "main.cpp"
// main.cpp jest kompilowane z -Dmain=game_main; cofamy to podmienienie, żeby
// nasz własny main() poniżej pozostał prawdziwym punktem wejścia testów.
#undef main

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Mini test framework
// ---------------------------------------------------------------------------
namespace tf {

struct Stats {
  int passed = 0;
  int failed = 0;
};

Stats g_stats;
std::string g_current_test;
std::vector<std::string> g_failures;

void check(bool cond, const std::string &expr, const char *file, int line) {
  if (cond) {
    ++g_stats.passed;
  } else {
    ++g_stats.failed;
    std::string msg = std::string(file) + ":" + std::to_string(line) + "  [" +
                      g_current_test + "]  " + expr;
    g_failures.push_back(msg);
    std::cout << "  \033[31mFAIL\033[0m " << expr << "  (" << file << ":"
              << line << ")\n";
  }
}

} // namespace tf

#define CHECK(cond) tf::check((cond), #cond, __FILE__, __LINE__)

// Rejestr testów: prosta lista funkcji uruchamianych po kolei.
using TestFn = void (*)();
struct TestCase {
  const char *name;
  TestFn fn;
};
std::vector<TestCase> &registry() {
  static std::vector<TestCase> r;
  return r;
}

struct Registrar {
  Registrar(const char *name, TestFn fn) { registry().push_back({name, fn}); }
};

#define TEST(name)                                                             \
  static void name();                                                         \
  static Registrar reg_##name(#name, name);                                   \
  static void name()

// ---------------------------------------------------------------------------
// Helpery do budowania planszy
// ---------------------------------------------------------------------------

// Kładzie `n` kamieni gracza w poziomie zaczynając od (x, y).
static void placeRow(Game &g, Cell player, std::size_t x, std::size_t y,
                     std::size_t n) {
  for (std::size_t i = 0; i < n; ++i)
    g.set(x + i, y, player);
}

// Kładzie `n` kamieni gracza w pionie zaczynając od (x, y).
static void placeCol(Game &g, Cell player, std::size_t x, std::size_t y,
                     std::size_t n) {
  for (std::size_t i = 0; i < n; ++i)
    g.set(x, y + i, player);
}

// Kładzie `n` kamieni po przekątnej (↘) zaczynając od (x, y).
static void placeDiag(Game &g, Cell player, std::size_t x, std::size_t y,
                      std::size_t n) {
  for (std::size_t i = 0; i < n; ++i)
    g.set(x + i, y + i, player);
}

// ===========================================================================
// Testy podstaw planszy (publiczne API: size / at / set)
// ===========================================================================

TEST(default_board_is_20x20) {
  Game g;
  CHECK(g.size() == 20);
}

TEST(custom_board_size) {
  Game g(15);
  CHECK(g.size() == 15);
}

TEST(fresh_board_is_all_empty) {
  Game g(5);
  bool all_empty = true;
  for (std::size_t y = 0; y < g.size(); ++y)
    for (std::size_t x = 0; x < g.size(); ++x)
      if (g.at(x, y) != Cell::Empty)
        all_empty = false;
  CHECK(all_empty);
}

TEST(set_then_at_roundtrip) {
  Game g;
  g.set(3, 7, Cell::Player1);
  g.set(0, 0, Cell::Player2);
  CHECK(g.at(3, 7) == Cell::Player1);
  CHECK(g.at(0, 0) == Cell::Player2);
  CHECK(g.at(1, 1) == Cell::Empty);
}

TEST(set_is_addressed_by_x_y_not_y_x) {
  // Regresja: (x, y) i (y, x) to różne pola dla asymetrycznych współrzędnych.
  Game g;
  g.set(2, 5, Cell::Player1);
  CHECK(g.at(2, 5) == Cell::Player1);
  CHECK(g.at(5, 2) == Cell::Empty);
}

TEST(overwrite_cell) {
  Game g;
  g.set(4, 4, Cell::Player1);
  g.set(4, 4, Cell::Player2);
  CHECK(g.at(4, 4) == Cell::Player2);
}

// ===========================================================================
// Testy wykrywania wygranej (publiczne API: checkWin)
//
// checkWin(player, x, y) ma zwrócić true jeśli ruch w (x, y) domyka linię
// >= 5 kamieni gracza w którymkolwiek kierunku (pion / poziom / przekątna).
//
// UWAGA: część z tych testów może być na razie CZERWONA — logika kierunkowa
// w Game ma jeszcze błędy do naprawy (o to Ci chodziło). Testy opisują
// docelowe, poprawne zachowanie i pokażą dokładnie co i gdzie poprawić.
// ===========================================================================

TEST(no_win_on_empty_board) {
  Game g;
  CHECK(g.checkWin(Cell::Player1, 10, 10) == false);
}

TEST(win_horizontal_five_in_a_row) {
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 5); // (5,8)..(9,8)
  CHECK(g.checkWin(Cell::Player1, 7, 8) == true);
}

TEST(no_win_horizontal_four_in_a_row) {
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 4); // tylko 4 kamienie
  CHECK(g.checkWin(Cell::Player1, 6, 8) == false);
}

TEST(win_vertical_five_in_a_column) {
  Game g;
  placeCol(g, Cell::Player2, 4, 3, 5); // (4,3)..(4,7)
  CHECK(g.checkWin(Cell::Player2, 4, 5) == true);
}

TEST(no_win_vertical_four_in_a_column) {
  Game g;
  placeCol(g, Cell::Player2, 4, 3, 4);
  CHECK(g.checkWin(Cell::Player2, 4, 4) == false);
}

TEST(win_diagonal_five) {
  Game g;
  placeDiag(g, Cell::Player1, 2, 2, 5); // (2,2)..(6,6)
  CHECK(g.checkWin(Cell::Player1, 4, 4) == true);
}

TEST(no_win_diagonal_four) {
  Game g;
  placeDiag(g, Cell::Player1, 2, 2, 4);
  CHECK(g.checkWin(Cell::Player1, 3, 3) == false);
}

TEST(win_only_counts_matching_player) {
  // Pięć kamieni w rzędzie, ale należą do Player1 — Player2 nie wygrywa.
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 5);
  CHECK(g.checkWin(Cell::Player2, 7, 8) == false);
}

// ===========================================================================
// Runner
// ===========================================================================

int main() {
  std::cout << "==> " << registry().size() << " testow\n\n";

  for (auto &tc : registry()) {
    tf::g_current_test = tc.name;
    int before_failed = tf::g_stats.failed;
    tc.fn();
    bool ok = (tf::g_stats.failed == before_failed);
    std::cout << (ok ? "\033[32mPASS\033[0m " : "\033[31mFAIL\033[0m ")
              << tc.name << "\n";
  }

  std::cout << "\n----------------------------------------\n";
  std::cout << "Passed: " << tf::g_stats.passed
            << "   Failed: " << tf::g_stats.failed << "\n";

  if (tf::g_stats.failed > 0) {
    std::cout << "\nNieudane asercje:\n";
    for (auto &f : tf::g_failures)
      std::cout << "  - " << f << "\n";
    return 1;
  }

  std::cout << "Wszystkie testy przeszly.\n";
  return 0;
}
