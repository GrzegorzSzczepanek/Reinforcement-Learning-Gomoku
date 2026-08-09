// test.cpp — definicje (implementacja) metod zadeklarowanych w test.h.
//
// Ten plik #include'uje własny header, żeby kompilator sprawdził, czy
// deklaracje z .h zgadzają się z definicjami tutaj.

#include "test.h"

// Lista inicjalizacyjna ": value_(value)" ustawia pole przy tworzeniu obiektu.
Test::Test(int value) : value_(value) {}

// Metoda prywatna: dostępna tylko z wnętrza klasy. Tu po prostu podwaja pole.
int Test::helper() const { return value_ * 2; }

// Metoda publiczna: korzysta z prywatnej i dodaje 1.
int Test::compute() const { return helper() + 1; }
