/** @file game_session.hpp
 * Заголовочный файл класса игровой сессии сервера игры "Лабиринт"
 * @details Класс обрабатывает одну игру с одним клиентом в рамках TCP-соединения
 * Выполняется в дочернем процессе после fork() сервера
 */

#ifndef GAME_SESSION_HPP
#define GAME_SESSION_HPP

#include <string>
#include <iostream>
#include <stdexcept>

#include <unistd.h>
#include <sys/socket.h>

#include "maze.hpp"
#include "maze_protocol.hpp"

/** Класс игровой сессии
 * @details Принимает сообщения от клиента через TCP-сокет, обрабатывает команды
 * перемещения, взаимодействует с моделью Maze, отправляет ответы согласно протоколу
 */
class GameSession
{
public:
    /** Создаёт игровую сессию
     * @param client_socket TCP-сокет соединения с клиентом
     * @param max_moves Максимальное число ходов для обычного режима
     * @param maze_size Размер стороны квадратного лабиринта
     * @param wall_count Число генерируемых стен
     */
    GameSession(int client_socket, int max_moves = 10, unsigned maze_size = 3u, unsigned wall_count = 3u)
        : _socket(client_socket), _max_moves(max_moves), _moves_left(max_moves), _perf_mode(false), _wall_count(wall_count), _maze(maze_size)
    {
    }

    /** Запускает основной игровой цикл
     * @throw std::runtime_error При ошибках ввода-вывода сокета
     */
    void run();

private:
    /** Принимает одно сообщение от клиента
     * @details Читает данные из сокета до символа '\n'
     * @return Строка сообщения без завершающего '\n'
     * @throw std::runtime_error При ошибке чтения или отключении клиента
     */
    std::string recvMessage();

    /** Отправляет сообщение клиенту
     * @details Добавляет '\n' к сообщению и отправляет через сокет
     * @param msg Сообщение для отправки
     */
    void sendMessage(const std::string &msg);

    /** Обрабатывает начальное сообщение HELLO или PERF
     * @param tokens Токены сообщения
     * @throw std::runtime_error При неверном формате
     */
    void handleInit(const std::vector<std::string> &tokens);

    /** Обрабатывает команду перемещения и формирует ответ
     * @param cmd Команда (FORWARD, RIGHT, LEFT, BACK, GIVEUP)
     * @param tokens Токены сообщения
     * @return Строка ответа для клиента
     */
    std::string processCommand(const std::string &cmd);

    /** Преобразует команду в смещение (dx, dy).
     * @param cmd Команда клиента
     * @param dx Выходной параметр — смещение по X
     * @param dy Выходной параметр — смещение по Y
     * @return true, если команда распознана
     */
    static bool cmdToDelta(const std::string &cmd, int &dx, int &dy);

    int _socket;              ///< TCP-сокет клиента
    int _max_moves;           ///< Лимит ходов
    int _moves_left;          ///< Оставшееся число ходов
    bool _perf_mode;          ///< Флаг режима тестирования (без лимита ходов)
    unsigned _wall_count;     ///< Число генерируемых стен
    Maze _maze;               ///< Модель лабиринта
    std::string _player_name; ///< Имя игрока
    char _buffer[1024];       ///< Буфер приёма данных
};

void GameSession::run()
{
    // Приём начального сообщения
    const std::string init_msg = recvMessage();
    const auto tokens = Protocol::tokenize(init_msg);
    if (tokens.empty())
    {
        throw std::runtime_error("GameSession: empty init message");
    }
    handleInit(tokens);

    // Генерация лабиринта
    _maze.generate(_wall_count);

    // Отправляем клиенту размер лабиринта
    sendMessage(Protocol::makeSizeRes(_maze.size()));

    std::cout << "[session] Игрок '" << _player_name << "'"
              << (_perf_mode ? " (тест.режим)" : "")
              << ", лабиринт сгенерирован" << std::endl;

    // Игровой цикл
    while (true)
    {
        const std::string msg = recvMessage();
        const auto cmd_tokens = Protocol::tokenize(msg);
        if (cmd_tokens.empty())
        {
            continue;
        }
        const std::string response = processCommand(cmd_tokens[0]);
        sendMessage(response);
        if (response == Protocol::RES_WIN || response == Protocol::RES_LOSE)
        {
            break;
        }
    }
    close(_socket);
}

std::string GameSession::recvMessage()
{
    std::string msg;
    while (true)
    {
        const ssize_t n = recv(_socket, _buffer, sizeof(_buffer) - 1, 0);
        if (n <= 0)
        {
            throw std::runtime_error("GameSession: client disconnected");
        }
        _buffer[n] = '\0';
        msg += _buffer;
        // Проверка наличия завершающего '\n'
        if (!msg.empty() && msg.back() == '\n')
        {
            msg.pop_back(); // убираем '\n'
            return msg;
        }
    }
}

void GameSession::sendMessage(const std::string &msg)
{
    const std::string framed = msg + "\n";
    send(_socket, framed.data(), framed.size(), 0);
}

void GameSession::handleInit(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        throw std::runtime_error("GameSession: invalid init format");
    }
    const std::string &cmd = tokens[0];
    _player_name = tokens[1];

    if (cmd == Protocol::REQ_PERF)
    {
        _perf_mode = true;
        _moves_left = -1; // безлимитный режим
    }
    else if (cmd == Protocol::REQ_HELLO)
    {
        _perf_mode = false;
        _moves_left = _max_moves;
    }
    else
    {
        throw std::runtime_error("GameSession: expected HELLO or PERF");
    }
}

std::string GameSession::processCommand(const std::string &cmd)
{
    if (cmd == Protocol::REQ_GIVEUP)
    {
        return Protocol::RES_LOSE;
    }

    int dx = 0, dy = 0;
    if (!cmdToDelta(cmd, dx, dy))
    {
        return Protocol::RES_LOSE; // неизвестная команда
    }

    if (_maze.move(dx, dy))
    {
        if (!_perf_mode)
            _moves_left--;
        if (_maze.isWin())
        {
            return Protocol::RES_WIN;
        }
        if (!_perf_mode && _moves_left <= 0)
        {
            return Protocol::RES_LOSE;
        }
        return Protocol::makeOkRes(_maze.playerX(), _maze.playerY(), _moves_left);
    }
    else
    {
        if (!_perf_mode)
            _moves_left--;
        if (!_perf_mode && _moves_left <= 0)
        {
            return Protocol::RES_LOSE;
        }
        return Protocol::makeWallRes(_moves_left);
    }
}

bool GameSession::cmdToDelta(const std::string &cmd, int &dx, int &dy)
{
    if (cmd == Protocol::REQ_FORWARD)
    {
        dx = 0;
        dy = 1;
        return true;
    }
    if (cmd == Protocol::REQ_RIGHT)
    {
        dx = 1;
        dy = 0;
        return true;
    }
    if (cmd == Protocol::REQ_LEFT)
    {
        dx = -1;
        dy = 0;
        return true;
    }
    if (cmd == Protocol::REQ_BACK)
    {
        dx = 0;
        dy = -1;
        return true;
    }
    return false;
}

#endif // GAME_SESSION_HPP
