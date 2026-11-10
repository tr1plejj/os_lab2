# Многопоточное умножение матриц

Лабораторная работа № 2 на C++20. Умножение и распределение строк между
потоками реализованы вручную с помощью `std::thread`.

[Задание](lab2.md) · [Отчет](report.md) · [Результаты измерений](results/grade3/)

## Требования

- Компилятор с поддержкой C++20, например GCC 13.
- CMake 3.24 или новее, Make либо Ninja.
- Для первого скачивания GoogleTest нужен доступ в интернет.
  Системная установка GoogleTest не требуется.

## Сборка

Из корня проекта:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
```

Для GCC и Clang в Release используются `-O2 -DNDEBUG`.
Число после `--parallel` задает количество задач сборки.

Сборка без тестов и загрузки GoogleTest:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --parallel 4
```

## Запуск

```bash
./build/matrix_demo 2000 1500 1800 8
```

Порядок аргументов: `n m k T`. Программа умножает матрицу A размером
`n×m` на матрицу B размером `m×k`, используя `T` вычисляющих потоков.
Все аргументы — положительные целые числа.

Выводятся прогресс, время умножения в секундах и сумма всех элементов
результата. Матрицы до 10×10 дополнительно печатаются целиком.

Небольшой пример:

```bash
./build/matrix_demo 2 3 2 2
```

## Тесты

Если ранее использовалась сборка без тестов, сначала включите их:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Сто повторений многопоточных тестов:

```bash
./build/matrix_tests --gtest_filter='ParallelMultiplication.*:ProgressReporting.*' --gtest_repeat=100
```

Назначение каждого теста описано в [tests/README.md](tests/README.md).
Инструкции для серии замеров, сравнения циклов и построения графиков —
в [tools/README.md](tools/README.md).

Формат C++-кода задается файлом `.clang-format` с профилем Google.
