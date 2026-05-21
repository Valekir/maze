/** @file maze.hpp
 * Заголовочный файл класса модели лабиринта для игры "Лабиринт"
 * @details Лабиринт представляет собой квадратное поле. Игрок стартует в (0,0),
 * цель всегда в правом верхнем углу. Стенки случайным образом расставляются
 * между соседними клетками
 */

#ifndef MAZE_HPP
#define MAZE_HPP

#include <set>
#include <vector>
#include <random>
#include <ostream>

/** Структура для представления клетки игрового поля */
struct Cell
{
    int x, y;

    Cell() : x(0), y(0) {}
    Cell(int x_, int y_) : x(x_), y(y_) {}

    bool operator<(const Cell &other) const
    {
        if (x != other.x)
        {
            return x < other.x;
        }
        return y < other.y;
    }
};

/** Структура для представления стены между двумя клетками */
struct Wall
{
    Cell from, to;

    bool operator<(const Wall &other) const
    {
        if (from < other.from)
        {
            return true;
        }
        if (other.from < from)
        {
            return false;
        }
        return to < other.to;
    }
};

/** Класс модели лабиринта
 * @details Хранит состояние игрового поля: размер, множество стен, позицию игрока и целевую позицию. Обеспечивает методы
 * для генерации стен, проверки возможностиперемещения и выполнения хода
 */
class Maze
{
public:
    /** Создаёт лабиринт заданного размера
     * @param size Размер стороны квадратного поля (по умолчанию 3) */
    Maze(unsigned size = 3u) : _size(size), _player_x(0), _player_y(0),
                               _goal_x(static_cast<int>(size) - 1), _goal_y(static_cast<int>(size) - 1),
                               _gen(std::random_device{}()) {}

    /** Генерирует случайный набор стен
     * @details Создаёт несколько случайных стен между соседними клетками. Гарантирует, что путь от старта до цели существует
     * @param wall_count Число генерируемых стен (по умолчанию 4)
     */
    void generate(unsigned wall_count = 4u);

    /** Проверяет возможность перемещения в заданном направлении
     * @param dx Смещение по горизонтали [-1, 1]
     * @param dy Смещение по вертикали [-1, 1]
     * @return true, если перемещение возможно
     */
    bool canMove(int dx, int dy);

    /** Выполняет перемещение игрока
     * @param dx Смещение по горизонтали
     * @param dy Смещение по вертикали
     * @return true, если перемещение выполнено успешно
     */
    bool move(int dx, int dy);

    /** Проверяет, достиг ли игрок целевой клетки
     * @return true, если игрок достиг цели
     */
    bool isWin();

    /** Возвращает текущую координату X игрока */
    int playerX()
    {
        return _player_x;
    }

    /** Возвращает текущую координату Y игрока */
    int playerY()
    {
        return _player_y;
    }

    /** Возвращает размер поля */
    unsigned size()
    {
        return _size;
    }

    /** Выводит текстовое представление лабиринта в поток
     * @param os Выходной поток
     */
    void print(std::ostream &os);

    /** Создаёт стену
     * @param a Первая клетка
     * @param b Вторая клетка
     * @return Пара клеток, между которыми расположена стена
     */
    static Wall makeWall(Cell a, Cell b);

    /** Проверяет существование пути от старта к цели (использует поиск в глубину)
     * @return true, если путь существует
     */
    bool pathExists();

    /** Устанавливает позицию игрока
     * @details Используется клиентом для синхронизации позиции по ответу сервера
     * @param x Новая координата X
     * @param y Новая координата Y
     */
    void setPlayerPosition(int x, int y)
    {
        _player_x = x;
        _player_y = y;
    }

    /** Добавляет стену в модель
     * @details Используется клиентом при получении ответа WALL
     * @param x1 X первой клетки
     * @param y1 Y первой клетки
     * @param x2 X второй клетки
     * @param y2 Y второй клетки
     */
    void addWall(int x1, int y1, int x2, int y2);

    /** Устанавливает размер лабиринта
     * @details Обновляет размер поля и пересчитывает позицию цели (правый верхний угол). Очищает существующие стены
     * @param size Новый размер стороны квадратного поля
     */
    void setSize(unsigned size)
    {
        _size = size;
        _goal_x = static_cast<int>(size) - 1;
        _goal_y = static_cast<int>(size) - 1;
        _walls.clear();
    }

private:
    /** Проверяет наличие стены над клеткой
     * @param x Координата X
     * @param y Координата Y
     * @return true, если стена присутствует
     */
    bool hasWallTop(int x, int y);

    /** Проверяет наличие стены справа от клетки
     * @param x Координата X
     * @param y Координата Y
     * @return true, если стена присутствуе
     */
    bool hasWallRight(int x, int y);

    /** Проверяет, находится ли клетка в границах поля
     * @param x Координата X
     * @param y Координата Y
     * @return true, если клетка внутри поля
     */
    bool inBounds(int x, int y);

    /** Возвращает список стен, отсутствующих в текущем наборе
     * @return Вектор возможных новых стен
     */
    std::vector<Wall> generateWalls();

    unsigned _size;        ///< Размер стороны поля.
    int _player_x;         ///< Текущая X-координата игрока.
    int _player_y;         ///< Текущая Y-координата игрока.
    int _goal_x;           ///< X-координата цели.
    int _goal_y;           ///< Y-координата цели.
    std::set<Wall> _walls; ///< Множество стен лабиринта.
    std::mt19937 _gen;     ///< Генератор случайных чисел.
};

#endif
