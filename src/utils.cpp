/** @file utils.cpp Вспомогательные функции для игры "Лабиринт" */

#include "utils.hpp"
#include "maze.hpp"
#include <algorithm>
#include <stack>

bool dfsPathExists(const std::set<Wall> &walls, int size, const Cell &start, const Cell &goal)
{
    std::set<Cell> visited;
    std::stack<Cell> stack;
    stack.push(start);

    const int dx[] = {1, -1, 0, 0};
    const int dy[] = {0, 0, 1, -1};

    while (!stack.empty())
    {
        const Cell cur = stack.top();
        stack.pop();

        if (cur.x == goal.x && cur.y == goal.y)
        {
            return true;
        }
        if (visited.find(cur) != visited.end())
        {
            continue;
        }
        visited.insert(cur);

        for (int d = 0; d < 4; d++)
        {
            const int nx = cur.x + dx[d];
            const int ny = cur.y + dy[d];

            if (nx < 0 || nx >= size || ny < 0 || ny >= size)
            {
                continue;
            }
            const Cell next(nx, ny);
            const Wall w = Maze::makeWall(cur, next);
            if (walls.find(w) == walls.end())
            {
                stack.push(next);
            }
        }
    }
    return false;
}
