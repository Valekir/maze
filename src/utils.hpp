/** @file utils.hpp Вспомогательные функции для игры "Лабиринт" */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <set>
#include "maze.hpp"

/** Проверяет существование пути от старта к цели (использует поиск в глубину)
 * @param walls Множество стен лабиринта
 * @param size Размер стороны поля
 * @param start Стартовая клетка
 * @param goal Целевая клетка
 * @return true, если путь существует
 */
bool dfsPathExists(const std::set<Wall> &walls, int size, const Cell &start, const Cell &goal);

#endif
