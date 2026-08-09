// test.h — deklaracja klasy Test.
//
// Header (.h) zawiera tylko DEKLARACJE: mówi "co istnieje" (jakie pola, jakie
// metody), a nie "jak to działa". Definicje (ciało metod) są w test.cpp.
// Dzięki temu inne pliki mogą #include "test.h" i wiedzieć jak używać klasy,
// nie widząc jej implementacji.

// Include guard: chroni przed dwukrotnym wklejeniem tego samego headera do
// jednej jednostki kompilacji (co dałoby błąd "redefinition"). #pragma once
// robi to samo krócej, ale klasyczny guard jest najbardziej przenośny.
#ifndef RL_GOMOKU_TEST_H
#define RL_GOMOKU_TEST_H

class Test {
public:
  // Konstruktor: przyjmuje jeden parametr, który zapamiętujemy w polu value_.
  explicit Test(int value);

  // Metoda publiczna — widoczna z zewnątrz (np. z main). Zwraca wynik
  // obliczenia opartego na metodzie prywatnej.
  int compute() const;

private:
  // Jedno pole (parametr klasy).
  int value_;

  // Metoda prywatna — pomocnicza, widoczna tylko wewnątrz klasy.
  int helper() const;
};

#endif // RL_GOMOKU_TEST_H
