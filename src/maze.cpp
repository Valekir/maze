/** @file maze.cpp
 * Файл класса модели лабиринта
 */

#include "maze.hpp"
#include "utils.hpp"
#include <algorithm>
#include <string>

Wall Maze::makeWall(Cell a, Cell b)
{
    if (b < a)
    {
        std::swap(a, b);
    }
    return {a, b};
}

std::vector<Wall> Maze::generateWalls()
{
    std::vector<Wall> result;
    int s = static_cast<int>(_size);

    // Горизонтальные стены
    for (int y = 0; y < s; y++)
    {
        for (int x = 0; x < s - 1; x++)
        {
            result.push_back(makeWall({x, y}, {x + 1, y}));
        }
    }

    // Вертикальные стены
    for (int x = 0; x < s; x++)
    {
        for (int y = 0; y < s - 1; y++)
        {
            result.push_back(makeWall({x, y}, {x, y + 1}));
        }
    }

    return result;
}

bool Maze::inBounds(int x, int y)
{
    int s = static_cast<int>(_size);
    return x >= 0 && x < s && y >= 0 && y < s;
}

bool Maze::hasWallTop(int x, int y)
{
    const Cell from(x, y);
    const Cell to(x, y + 1);
    return _walls.find(makeWall(from, to)) != _walls.end();
}

bool Maze::hasWallRight(int x, int y)
{
    const Cell from(x, y);
    const Cell to(x + 1, y);
    return _walls.find(makeWall(from, to)) != _walls.end();
}

void Maze::addWall(int x1, int y1, int x2, int y2)
{
    _walls.insert(makeWall({x1, y1}, {x2, y2}));
}

bool Maze::canMove(int dx, int dy)
{
    const int nx = _player_x + dx;
    const int ny = _player_y + dy;

    if (!inBounds(nx, ny))
    {
        return false;
    }

    const Cell from(_player_x, _player_y);
    const Cell to(nx, ny);
    const Wall w = makeWall(from, to);

    return _walls.find(w) == _walls.end();
}

void Maze::generate(unsigned wall_count)
{
    _walls.clear();
    auto newWalls = generateWalls();
    if (newWalls.empty())
    {
        return;
    }

    // Случайное перемешивание стен
    std::shuffle(newWalls.begin(), newWalls.end(), _gen);

    // Пытаемся добавить стены по одной, проверяя существование пути
    unsigned added = 0;
    for (const auto &w : newWalls)
    {
        if (added >= wall_count)
        {
            break;
        }

        _walls.insert(w);
        if (!pathExists())
        {
            // откатываем, если путь исчез
            _walls.erase(w);
        }
        else
        {
            added++;
        }
    }
}

bool Maze::move(int dx, int dy)
{
    if (!canMove(dx, dy))
    {
        return false;
    }

    _player_x += dx;
    _player_y += dy;

    return true;
}

bool Maze::isWin()
{
    return _player_x == _goal_x && _player_y == _goal_y;
}

void Maze::print(std::ostream &os)
{
    const int s = static_cast<int>(_size);

    // Верхняя граница
    os << "+";
    for (int x = 0; x < s; x++)
    {
        os << "---+";
    }
    os << "\n";

    for (int y = s - 1; y >= 0; y--)
    {
        // Строка клеток
        os << "|";
        for (int x = 0; x < s; x++)
        {
            std::string content = "   ";
            if (x == _player_x && y == _player_y)
            {
                content = " P ";
            }
            else if (x == _goal_x && y == _goal_y)
            {
                content = " G ";
            }

            os << content;

            if (x < s - 1 && hasWallRight(x, y))
            {
                os << "|";
            }
            else if (x < s - 1)
            {
                os << " ";
            }
            else
            {
                os << "|";
            }
        }
        os << "\n";

        // Строка горизонтальных стен
        if (y > 0)
        {
            os << "+";
            for (int x = 0; x < s; x++)
            {
                if (hasWallTop(x, y - 1))
                {
                    os << "---";
                }
                else
                {
                    os << "   ";
                }
                os << "+";
            }
            os << "\n";
        }
    }

    // Нижняя граница
    os << "+";
    for (int x = 0; x < s; x++)
    {
        os << "---+";
    }
    os << "\n";
}

bool Maze::pathExists()
{
    return dfsPathExists(_walls, static_cast<int>(_size), Cell(0, 0), Cell(_goal_x, _goal_y));
}
