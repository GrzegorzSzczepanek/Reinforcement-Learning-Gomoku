// module.cpp — cienki binding pybind11 dla gomoku::Game.
//
// Filozofia: wystawiamy tylko czysta logike gry. reset/step/reward/self-play
// pisze uzytkownik w Pythonie (python/gomoku/env.py). Tu zero logiki RL.

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstring>

#include "gomoku/game.hpp"

namespace py = pybind11;
using gomoku::Cell;
using gomoku::Game;

// stateTensor -> numpy float32 o ksztalcie [3, size, size].
static py::array_t<float> stateTensorNumpy(const Game &g, Cell toMove) {
  auto flat = g.stateTensor(toMove);
  auto shape = g.stateShape(); // {3, size, size}
  py::array_t<float> arr({shape[0], shape[1], shape[2]});
  std::memcpy(arr.mutable_data(), flat.data(), flat.size() * sizeof(float));
  return arr;
}

PYBIND11_MODULE(_core, m) {
  m.doc() = "Gomoku Core (C++) — czysta logika gry dla RL.";

  py::enum_<Cell>(m, "Cell")
      .value("Empty", Cell::Empty)
      .value("Player1", Cell::Player1)
      .value("Player2", Cell::Player2);

  m.def("opponent", &gomoku::opponent, py::arg("player"),
        "Zwraca przeciwnika danego gracza (Empty -> Empty).");
  m.attr("WIN_LENGTH") = gomoku::WIN_LENGTH;

  py::class_<Game>(m, "Game")
      .def(py::init<std::size_t>(), py::arg("size") = 20)
      .def("size", &Game::size)
      .def("at", &Game::at, py::arg("x"), py::arg("y"))
      .def("set", &Game::set, py::arg("x"), py::arg("y"), py::arg("cell"))
      .def("undo_set", &Game::undoSet, py::arg("x"), py::arg("y"))
      .def("is_legal", &Game::isLegal, py::arg("x"), py::arg("y"))
      .def("legal_moves", &Game::legalMoves,
           "Puste pola jako indeksy y*size + x (maska akcji).")
      .def("is_full", &Game::isFull)
      .def("has_won", &Game::hasWon, py::arg("player"), py::arg("x"),
           py::arg("y"),
           "True gdy ruch (x,y) domyka linie >=5 dla gracza. player != Empty.")
      .def("clear", &Game::clear)
      .def("state_tensor", &stateTensorNumpy, py::arg("to_move"),
           "Obserwacja NCHW float32 [3, size, size]: ja / przeciwnik / na-ruchu.");
}
