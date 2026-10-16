# Лабораторная работа №2 — Вариант 20

Дан массив координат  `(x, y, z)`. Необходимо найти три точки, которые образуют
треугольник максимальной площади

## Формат файла с точками

Первая строка — количество точек `n`, далее `n` строк с тремя вещественными числами `x y z` через пробел.

```
5
1.0 2.0 3.0
4.0 5.0 6.0
7.0 8.0 9.0
1.5 2.5 3.5
10.0 0.0 0.0
```

## Сборка

```sh
    cc -O2 -o sequential sequential.c -lm
    cc -O2 -pthread -o parallel parallel.c -lm
```

## Запуск

Последовательная версия:

```sh
    ./sequential points.txt
```

Параллельная версия (вторым аргументом задаётся количество потоков):

```sh
    ./parallel points.txt 1
    ./parallel points.txt 2
    ./parallel points.txt 4
    ./parallel points.txt 6
    ./parallel points.txt 8
    ./parallel points.txt 16
    ./parallel points.txt 1024
```
