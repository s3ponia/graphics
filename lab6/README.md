# Лабораторная работа 5: Изучение алгоритма настройки автояркости изображения

## Цель работы: реализовать программу, которая позволяет проводить масштабирование изображений.

## Описание: Аргументы передаются через командную строку
lab6.exe input output width height dx dy gamma type,  
где  
* input, output - имена входного и выходного файлов в формате PNM P5 или P6.
* width, height - ширина и высота результирующего изображения, натуральные числа.
* dx,dy - смещение центра результата относительно центра исходного изображения, вещественные числа в единицах результирующего изображения.
* gamma - гамма-коррекция (0.0 = sRGB).
* type - способ масштабирования:
  * 0 – ближайшая точка (метод ближайшего соседа)
  * 1 – билинейное
  * 2 – Lanczos3
  * 3 – BC-сплайны. Для этого способа могут быть указаны ещё два параметра: B и C, по умолчанию 0 и 0.5 (Catmull-Rom).

Входные/выходные данные: PNM P5 или P6 (RGB). 
