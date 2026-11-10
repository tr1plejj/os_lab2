# Запуск экспериментов

Команды выполняются из корня проекта после сборки Release.
Python 3 используется для повторения запусков и обработки результатов;
генерация матриц и умножение выполняются на C++.

## Основная серия

```bash
python3 tools/run_benchmarks.py
```

По умолчанию используются размеры 800, 1600 и 2200, числа потоков
1–16, 20, 24, 28, 32, 64, 128 и пять повторений.
CSV и журналы сохраняются в `artifacts/benchmarks`, исключенный из Git.
Повтор команды продолжает недостающие запуски при неизменном плане и бинарнике.

Параметры своей серии:

```bash
python3 tools/run_benchmarks.py --sizes 800 1600 2200 --threads 1 2 4 8 16 32 64 128 --repeats 5 --output artifacts/my-run
```

Для сравнения ускорения в списке потоков обязательно должно быть значение 1.
Перед замерами закройте тяжелые приложения и выполняйте только одну серию.

## Сравнение порядков циклов

```bash
cmake --build build --target loop_order_benchmark
./build/loop_order_benchmark 2200 ijt
./build/loop_order_benchmark 2200 itj
```

Пять повторов обоих порядков с записью CSV:

```bash
python3 tools/run_loop_benchmarks.py
```

По умолчанию выбран размер 2200, результаты записываются в
`artifacts/benchmarks/loop-order.csv`. Параметры `--size` и `--output`
меняют размер и каталог. Существующий CSV не перезаписывается.

## Обработка и графики

Установите matplotlib в локальный каталог проекта:

```bash
python3 -m pip install --target .cache/analysis-python matplotlib==3.10.7
```

Пересчитать опубликованные данные из `results/grade3`:

```bash
PYTHONPATH=.cache/analysis-python python3 tools/analyze_benchmarks.py
```

Обработать новую серию:

```bash
PYTHONPATH=.cache/analysis-python python3 tools/analyze_benchmarks.py --input artifacts/benchmarks --output artifacts/analysis
```

В выходном каталоге появятся средние времена, ускорение, эффективность,
отметки использованных замеров и графики. Исходные CSV не изменяются.
