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

// Kładzie `n` kamieni po anty-przekątnej (↗) zaczynając od (x, y) i idąc
// w prawo-górę: (x, y), (x+1, y-1), ...  (y musi być >= n-1).
static void placeAntiDiag(Game &g, Cell player, std::size_t x, std::size_t y,
                          std::size_t n) {
  for (std::size_t i = 0; i < n; ++i)
    g.set(x + i, y - i, player);
}

// Owijka na aktualny detektor wygranej. Wszystkie testy wygranej idą przez
// hasWon() — stary checkWin(...) z winningCords jest wycofany (patrz main.cpp).
static bool won(Game &g, Cell player, std::size_t x, std::size_t y) {
  return g.hasWon(player, x, y);
}

// Helper dla getEmptyBoxes(), które zwraca pary size_t.
static bool contains_pair(const std::vector<std::pair<std::size_t, std::size_t>> &v,
                          std::size_t x, std::size_t y) {
  for (const auto &[cx, cy] : v)
    if (cx == x && cy == y)
      return true;
  return false;
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
  CHECK(won(g, Cell::Player1, 10, 10) == false);
}

TEST(win_horizontal_five_in_a_row) {
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 5); // (5,8)..(9,8)
  CHECK(won(g, Cell::Player1, 7, 8) == true);
}

TEST(no_win_horizontal_four_in_a_row) {
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 4); // tylko 4 kamienie
  CHECK(won(g, Cell::Player1, 6, 8) == false);
}

TEST(win_vertical_five_in_a_column) {
  Game g;
  placeCol(g, Cell::Player2, 4, 3, 5); // (4,3)..(4,7)
  CHECK(won(g, Cell::Player2, 4, 5) == true);
}

TEST(no_win_vertical_four_in_a_column) {
  Game g;
  placeCol(g, Cell::Player2, 4, 3, 4);
  CHECK(won(g, Cell::Player2, 4, 4) == false);
}

TEST(win_diagonal_five) {
  Game g;
  placeDiag(g, Cell::Player1, 2, 2, 5); // (2,2)..(6,6)
  CHECK(won(g, Cell::Player1, 4, 4) == true);
}

TEST(no_win_diagonal_four) {
  Game g;
  placeDiag(g, Cell::Player1, 2, 2, 4);
  CHECK(won(g, Cell::Player1, 3, 3) == false);
}

TEST(win_only_counts_matching_player) {
  // Pięć kamieni w rzędzie, ale należą do Player1 — Player2 nie wygrywa.
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 5);
  CHECK(won(g, Cell::Player2, 7, 8) == false);
}

// ===========================================================================
// Anty-przekątna (↗ / ↙) — czwarty kierunek wykrywany przez hasWon.
// ===========================================================================

TEST(win_antidiagonal_five) {
  Game g;
  // Linia (10,10),(11,9),(12,8),(13,7),(14,6) — pięć w kierunku ↗.
  placeAntiDiag(g, Cell::Player1, 10, 10, 5);
  CHECK(won(g, Cell::Player1, 12, 8) == true);
}

TEST(no_win_antidiagonal_four) {
  Game g;
  placeAntiDiag(g, Cell::Player2, 10, 10, 4);
  CHECK(won(g, Cell::Player2, 11, 9) == false);
}

TEST(win_antidiagonal_detected_from_endpoint) {
  // Ruch domykający na skraju linii, nie w środku.
  Game g;
  placeAntiDiag(g, Cell::Player1, 3, 8, 5); // (3,8),(4,7),(5,6),(6,5),(7,4)
  CHECK(won(g, Cell::Player1, 3, 8) == true);
  CHECK(won(g, Cell::Player1, 7, 4) == true);
}

// ===========================================================================
// Semantyka hasWon: skan jest obcięty do 5 kroków w każdą stronę (pętla
// `i < 5`). Te testy pilnują, że obcięcie nigdy nie gubi ani nie zmyśla
// wygranej — regresja na wypadek zmian w tej pętli.
// ===========================================================================

TEST(haswon_true_from_any_stone_in_exact_five) {
  // Piątka pozioma: ruch domykający z KAŻDEGO z pięciu pól ma dać wygraną.
  Game g;
  placeRow(g, Cell::Player1, 6, 9, 5); // (6,9)..(10,9)
  for (std::size_t x = 6; x <= 10; ++x)
    CHECK(won(g, Cell::Player1, x, 9) == true);
}

TEST(haswon_true_deep_inside_long_line) {
  // Linia dłuższa niż zasięg skanu (9 kamieni). Sprawdzenie ze środka też
  // musi widzieć piątkę, mimo że w jedną stronę jest tylko część kamieni.
  Game g;
  placeRow(g, Cell::Player2, 2, 5, 9); // (2,5)..(10,5)
  CHECK(won(g, Cell::Player2, 6, 5) == true); // środek
  CHECK(won(g, Cell::Player2, 2, 5) == true); // lewy skraj
  CHECK(won(g, Cell::Player2, 10, 5) == true); // prawy skraj
}

TEST(haswon_neighbouring_stone_does_not_win) {
  // KONTRAKT: hasWon jest wołane po ruchu, zawsze z player != Empty (nigdy
  // z Cell::Empty — inaczej zliczyłoby ciąg pustych pól jako "wygraną").
  // Tu sprawdzamy realny przypadek: kamień przeciwnika tuż obok cudzej
  // piątki nie może fałszywie zaliczyć wygranej.
  Game g;
  placeRow(g, Cell::Player1, 5, 5, 5); // piątka Player1: (5,5)..(9,5)
  g.set(4, 5, Cell::Player2);          // Player2 dostawiony z lewej
  CHECK(won(g, Cell::Player2, 4, 5) == false);
}

TEST(haswon_directions_are_independent) {
  // Krzyż: po 3 w poziomie i 3 w pionie przez wspólne pole — żaden kierunek
  // nie ma piątki, więc brak wygranej (kierunki nie sumują się).
  Game g;
  placeRow(g, Cell::Player1, 8, 8, 3); // (8,8),(9,8),(10,8)
  placeCol(g, Cell::Player1, 9, 6, 5); // przecina w (9,8), pion ma 5
  // Pion ma piątkę -> wygrana; poziom sam w sobie nie.
  CHECK(won(g, Cell::Player1, 9, 8) == true);
  // Ale gdyby pion był krótszy, samo (8,8) w poziomie (3) nie wygrywa:
  Game h;
  placeRow(h, Cell::Player1, 8, 8, 3);
  placeCol(h, Cell::Player1, 9, 6, 3);
  CHECK(won(h, Cell::Player1, 9, 8) == false);
}

// ===========================================================================
// Edge case'y planszy i długich linii — istotne przy milionach gier.
// ===========================================================================

TEST(win_at_top_left_corner) {
  Game g;
  placeRow(g, Cell::Player1, 0, 0, 5); // (0,0)..(4,0)
  CHECK(won(g, Cell::Player1, 0, 0) == true);
}

TEST(win_at_bottom_right_corner) {
  Game g; // 20x20 -> ostatni indeks 19
  placeCol(g, Cell::Player2, 19, 15, 5); // (19,15)..(19,19)
  CHECK(won(g, Cell::Player2, 19, 19) == true);
}

TEST(win_diagonal_into_corner) {
  Game g;
  placeDiag(g, Cell::Player1, 15, 15, 5); // (15,15)..(19,19)
  CHECK(won(g, Cell::Player1, 19, 19) == true);
}

TEST(six_in_a_row_still_wins) {
  // Więcej niż pięć w linii nadal jest wygraną (>= 5).
  Game g;
  placeRow(g, Cell::Player1, 3, 10, 6); // (3,10)..(8,10)
  CHECK(won(g, Cell::Player1, 5, 10) == true);
}

TEST(gap_breaks_the_line) {
  // Cztery, dziura, jeden — nie ma pięciu pod rząd.
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 4); // (5,8)..(8,8)
  g.set(10, 8, Cell::Player1);         // (9,8) puste
  CHECK(won(g, Cell::Player1, 7, 8) == false);
  CHECK(won(g, Cell::Player1, 10, 8) == false);
}

TEST(opponent_stone_blocks_the_line) {
  // Kamień przeciwnika w środku przerywa zliczanie.
  Game g;
  placeRow(g, Cell::Player1, 5, 8, 5);
  g.set(7, 8, Cell::Player2); // przerwa w rzędzie Player1
  CHECK(won(g, Cell::Player1, 6, 8) == false);
  CHECK(won(g, Cell::Player1, 8, 8) == false);
}

TEST(two_separate_short_lines_no_win) {
  // Dwie oddzielne czwórki tego samego gracza nie sumują się do wygranej.
  Game g;
  placeRow(g, Cell::Player1, 2, 3, 4);
  placeRow(g, Cell::Player1, 10, 3, 4);
  CHECK(won(g, Cell::Player1, 3, 3) == false);
  CHECK(won(g, Cell::Player1, 11, 3) == false);
}

TEST(win_on_small_board) {
  // Plansza dokładnie 5x5: pełna kolumna to wygrana.
  Game g(5);
  placeCol(g, Cell::Player1, 2, 0, 5);
  CHECK(won(g, Cell::Player1, 2, 2) == true);
}

TEST(no_win_on_too_small_board) {
  // Plansza 4x4 nie mieści pięciu w linii.
  Game g(4);
  placeRow(g, Cell::Player1, 0, 0, 4);
  CHECK(won(g, Cell::Player1, 0, 0) == false);
}

// ===========================================================================
// getEmptyBoxes — używane w pętli rozgrywki (miliony wywołań).
// ===========================================================================

TEST(empty_boxes_count_on_fresh_board) {
  Game g(5);
  CHECK(g.getEmptyBoxes().size() == 25);
}

TEST(empty_boxes_excludes_filled_cells) {
  Game g(5);
  g.set(0, 0, Cell::Player1);
  g.set(4, 4, Cell::Player2);
  auto boxes = g.getEmptyBoxes();
  CHECK(boxes.size() == 23);
  CHECK(!contains_pair(boxes, 0, 0));
  CHECK(!contains_pair(boxes, 4, 4));
  CHECK(contains_pair(boxes, 2, 2));
}

TEST(empty_boxes_empty_when_board_full) {
  Game g(3);
  for (std::size_t y = 0; y < g.size(); ++y)
    for (std::size_t x = 0; x < g.size(); ++x)
      g.set(x, y, Cell::Player1);
  CHECK(g.getEmptyBoxes().empty());
}

// ===========================================================================
// Operator ! na Cell (przełączanie gracza w pętli rozgrywki).
// ===========================================================================

TEST(negate_cell_swaps_players) {
  CHECK(!Cell::Player1 == Cell::Player2);
  CHECK(!Cell::Player2 == Cell::Player1);
}

TEST(negate_empty_is_identity) {
  // Empty nie ma "przeciwnika" — operator zwraca to samo.
  CHECK(!Cell::Empty == Cell::Empty);
}

TEST(double_negate_is_identity) {
  CHECK(!!Cell::Player1 == Cell::Player1);
  CHECK(!!Cell::Player2 == Cell::Player2);
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
