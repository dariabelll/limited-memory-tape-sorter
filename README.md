# Limited Memory Tape Sorter

![CI](https://github.com/dariabelll/limited-memory-tape-sorter/actions/workflows/ci.yml/badge.svg)

Консольное приложение на C++20 для сортировки чисел на ленточном устройстве при ограничении оперативной памяти.

## Что решает программа

На вход подаётся бинарная лента с `int32_t`-значениями. Нужно записать в выходную ленту те же значения, но отсортированные по возрастанию.

Главное ограничение: вход может быть больше доступной оперативной памяти, поэтому программа не загружает всю ленту целиком. Вместо этого используется внешняя сортировка:

1. входная лента читается последовательными блоками, которые помещаются в `memory_limit_bytes`;
2. каждый блок сортируется в памяти;
3. отсортированные блоки записываются во временные ленты;
4. временные ленты сливаются в итоговую отсортированную ленту;
5. результат копируется в выходной файл.

## Особенности

- `ITape` - интерфейс ленточного устройства.
- `FileTape` — файловая эмуляция ленты.
- Конфигурируемые задержки операций ленты без перекомпиляции.
- Ограничение памяти через `memory_limit_bytes`.
- External merge sort вместо загрузки всего input в память.
- Временные отсортированные ленты представлены как `SortedTapePart`.
- Merge-фаза умеет сливать несколько временных лент за раз, а не только попарно.
- `--verify` — последовательная проверка отсортированности output-ленты.
- `SortReport` — отчёт о ходе сортировки: количество временных лент, merge-раунды, сравнение с попарным merge.
- `TemporaryTapeCleaner` — RAII-cleanup временных лент.
- Unit/component-тесты на GoogleTest.
- CLI integration test через CTest + Python.
- GitHub Actions CI.

## Сборка

Требования:

- CMake 3.16+
- компилятор с поддержкой C++20
- Python 3 для интеграционного теста
- Git для загрузки GoogleTest через CMake

Сборка:

```bash
cmake -S . -B build
cmake --build build
```

Запуск тестов:

```bash
ctest --test-dir build --output-on-failure
```

Чистая локальная проверка:

```bash
rm -rf build
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Запуск

```bash
./build/limited-memory-tape-sorter <input_file> <output_file> [config_file] [--verify]
```

На Windows:

```powershell
.\build\limited-memory-tape-sorter.exe <input_file> <output_file> [config_file] [--verify]
```

### Аргументы командной строки

| Аргумент | Обязательный | Что означает |
|---|---:|---|
| `<input_file>` | да | входная бинарная лента с `int32_t`-значениями |
| `<output_file>` | да | выходная бинарная лента; файл создаётся заново или перезаписывается |
| `[config_file]` | нет | путь к конфигу; если не указан, используется `config.txt` |
| `[--verify]` | нет | после сортировки проверить, что output отсортирован |

Пример:

```bash
./build/limited-memory-tape-sorter input.bin output.bin config.example.txt --verify
```

## Формат входной и выходной ленты

Файлы лент бинарные.

Каждая ячейка хранит одно значение:

```cpp
std::int32_t
```

То есть длина корректного файла должна быть кратна 4 байтам. Текстовый файл с числами через пробел не является корректной входной лентой.

## Конфигурация

Пример конфигурации находится в `config.example.txt`.

Формат — `key=value`, один параметр на строку. Пустые строки и строки, начинающиеся с `#`, игнорируются.

```text
read_delay_ms=0
write_delay_ms=0
move_delay_ms=0
rewind_delay_ms=0
memory_limit_bytes=1048576
tmp_dir=tmp
```

### Параметры конфига

| Параметр | Что задаёт |
|---|---|
| `read_delay_ms` | задержка чтения одной ячейки |
| `write_delay_ms` | задержка записи одной ячейки |
| `move_delay_ms` | задержка сдвига ленты на одну позицию |
| `rewind_delay_ms` | задержка перемотки ленты |
| `memory_limit_bytes` | лимит памяти для значений, загружаемых алгоритмом |
| `tmp_dir` | директория временных лент |

`memory_limit_bytes` ограничивает именно объём значений, которые алгоритм загружает в память как рабочий блок:

```text
max_values_in_memory = memory_limit_bytes / sizeof(std::int32_t)
```

## Архитектура

| Компонент | Ответственность |
|---|---|
| `ITape` | минимальный интерфейс ленточного устройства |
| `FileTape` | эмуляция ленты через бинарный файл |
| `TapeConfig` | загрузка внешнего конфига |
| `TapeSorter` | внешний алгоритм сортировки |
| `SortedTapePart` | описание временной отсортированной ленты |
| `TapeVerifier` | последовательная проверка output-ленты |
| `SortReport` | сбор метрик сортировки |
| `TemporaryTapeCleaner` | RAII-удаление временных лент |

Сортировщик работает через ленточную абстракцию. Файловый доступ спрятан внутри `FileTape`; остальной код не должен работать с input/output как с массивом.

## Алгоритм

### 1. Создание временных отсортированных лент

`TapeSorter` открывает входную ленту и читает её последовательными блоками.

Каждый блок сортируется в памяти и записывается во временную ленту в `tmp_dir`.

### 2. Слияние временных лент

После первого этапа есть несколько временных отсортированных лент.

Обычный вариант внешней сортировки — сливать их попарно. В этом проекте merge-фаза выбирает, сколько временных лент можно сливать одновременно, исходя из лимита памяти. Во время слияния в памяти хранится текущее значение каждой активной ленты и служебная структура для выбора минимума.

В отчёте это видно по строкам:

```text
max tapes per merge
merge rounds
pairwise merge would need
```

Например:

```text
Merge phase:
  max tapes per merge: 5
  merge rounds: 1
  pairwise merge would need: 2 rounds
```

Это означает, что временные ленты были слиты не попарно, а одной merge-группой.

### 3. Копирование в output

Когда остаётся одна финальная временная лента, она последовательно копируется в выходную ленту. После этого временные файлы удаляются.

## Отчёт после сортировки

После запуска программа печатает summary, который показывает не только факт успешной сортировки, но и то, как именно сработал внешний алгоритм.

Пример:

```text
Sorting completed

Tape sorting summary
--------------------
Input tape:
  values: 100

Memory:
  limit: 128 bytes
  max loaded values: 32

Temporary tapes:
  initial sorted tapes: 4
  merged tapes: 1
  total created: 5

Merge phase:
  max tapes per merge: 5
  merge rounds: 1
  pairwise merge would need: 2 rounds

Output tape:
  values: 100
  verification: OK
```

Ключевые поля:

| Поле | Что показывает |
|---|---|
| `input values` | сколько значений было на входной ленте |
| `max loaded values` | сколько значений максимум загружалось в память за раз |
| `initial sorted tapes` | сколько временных отсортированных лент создано после первого этапа |
| `merged tapes` | сколько временных лент создано на merge-фазе |
| `max tapes per merge` | сколько временных лент можно было сливать одновременно |
| `merge rounds` | сколько раундов merge реально потребовалось |
| `pairwise merge would need` | сколько раундов потребовалось бы при попарном merge |
| `verification` | результат проверки `--verify` |

## Проверка результата

Флаг `--verify` запускает дополнительную проверку output-ленты.

Проверка тоже последовательная: программа читает output слева направо и проверяет, что каждое следующее значение не меньше предыдущего. Весь output при этом не загружается в память.

## Временные файлы

Временные ленты создаются в директории `tmp_dir` из конфига.

Используются два типа временных файлов:

```text
sorted_part_*.bin
merged_part_*.bin
```

После успешной сортировки временные файлы удаляются. Для cleanup используется `TemporaryTapeCleaner`: если во время сортировки возникает исключение, уже созданные временные ленты также удаляются при выходе из scope.

## Тесты

Тесты запускаются через CTest:

```bash
ctest --test-dir build --output-on-failure
```

В проекте есть:

- тесты файловой ленты;
- тесты чтения конфига;
- тесты сортировщика;
- тесты верификатора;
- тесты cleanup временных лент;
- интеграционный CLI-тест.

Интеграционный тест запускает собранный executable как пользователь: создаёт бинарный input, создаёт config, запускает программу и проверяет output.

## CI

В репозитории настроен GitHub Actions workflow.

На каждый push и pull request выполняется:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Структура проекта

```text
.
├── include/
│   ├── FileTape.hpp
│   ├── ITape.hpp
│   ├── SortReport.hpp
│   ├── SortedTapePart.hpp
│   ├── TapeConfig.hpp
│   ├── TapeSorter.hpp
│   ├── TapeVerifier.hpp
│   └── TemporaryTapeCleaner.hpp
├── src/
│   ├── FileTape.cpp
│   ├── TapeConfig.cpp
│   ├── TapeSorter.cpp
│   ├── TapeVerifier.cpp
│   ├── TemporaryTapeCleaner.cpp
│   └── main.cpp
├── tests/
│   ├── integration/
│   │   └── sort_cli.py
│   └── *.cpp
├── CMakeLists.txt
├── config.example.txt
└── README.md
```
