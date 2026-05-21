/** @file maze_
 * Файл с описанием протокола игры "Лабиринт".
 * @details Протокол текстовый, сообщения разделяются символом перевода строки.
 * Формат сообщений:
 * - Клиент: REQ_HELLO имя | REQ_PERF имя | REQ_FORWARD | REQ_RIGHT | REQ_LEFT | REQ_BACK | REQ_GIVEUP
 * - Сервер: RES_OK x y ходов | RES_WALL ходов | RES_WIN | RES_LOSE | RES_SIZE размер
 */

#ifndef MAZE_PROTOCOL_HPP
#define MAZE_PROTOCOL_HPP

#include <string>
#include <sstream>
#include <vector>

/**
 * Пространство имён с константами и функциями протокола игры "Лабиринт"
 */
namespace Protocol
{
    /** Команда начала игры. Формат: "REQ_HELLO имя" */
    const std::string REQ_HELLO = "HELLO";

    /** Команда начала игры в режиме тестирования. Формат: "REQ_PERF <имя>" */
    const std::string REQ_PERF = "PERF";

    /** Команда движения вперёд (+Y) */
    const std::string REQ_FORWARD = "FORWARD";

    /** Команда движения вправо (+X) */
    const std::string REQ_RIGHT = "RIGHT";

    /** Команда движения влево (-X) */
    const std::string REQ_LEFT = "LEFT";

    /** Команда движения назад (-Y) */
    const std::string REQ_BACK = "BACK";

    /** Команда сдачи */
    const std::string REQ_GIVEUP = "GIVEUP";

    /** Успешное перемещение. Формат: "OK x y осталось_ходов" */
    const std::string RES_OK = "OK";

    /** Столкновение со стеной. Формат: "WALL осталось_ходов" */
    const std::string RES_WALL = "WALL";

    /** Победа — игрок достиг цели */
    const std::string RES_WIN = "WIN";

    /** Поражение — кончились ходы или игрок сдался */
    const std::string RES_LOSE = "LOSE";

    /** Размер лабиринта. Формат: "SIZE <размер>" */
    const std::string RES_SIZE = "SIZE";

    /** Формирует сообщение HELLO
     * @param name Имя игрока
     * @return Строка сообщения
     */
    std::string makeHelloReq(const std::string &name)
    {
        return REQ_HELLO + " " + name;
    }

    /** Формирует сообщение PERF (режим тестирования)
     * @param name Имя игрока
     * @return Строка сообщения
     */
    std::string makePerfReq(const std::string &name)
    {
        return REQ_PERF + " " + name;
    }

    /** Формирует ответ OK
     * @param x Координата X игрока
     * @param y Координата Y игрока
     * @param remaining Оставшееся число ходов
     * @return Строка ответа
     */
    std::string makeOkRes(int x, int y, int remaining)
    {
        return RES_OK + " " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(remaining);
    }

    /** Формирует ответ WALL
     * @param remaining Оставшееся число ходов
     * @return Строка ответа
     */
    std::string makeWallRes(int remaining)
    {
        return RES_WALL + " " + std::to_string(remaining);
    }

    /** Формирует ответ SIZE с размером лабиринта
     * @param size Размер стороны лабиринта
     * @return Строка ответа
     */
    inline std::string makeSizeRes(unsigned size)
    {
        return RES_SIZE + " " + std::to_string(size);
    }

    /** Разбирает строку сообщения на токены, разделённые пробелами
     * @param msg Исходное сообщение
     * @return Вектор токенов
     */
    inline std::vector<std::string> tokenize(const std::string &msg)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(msg);
        std::string token;

        while (iss >> token)
        {
            tokens.push_back(token);
        }
        return tokens;
    }
}

#endif
