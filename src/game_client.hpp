/** @file game_client.hpp
 * Заголовочный файл класса TCP-клиента игры "Лабиринт"
 * @details Клиент подключается к серверу, отправляет команды перемещения,
 * получает ответы и отображает консольный интерфейс с визуализацией хода игры
 */

#ifndef GAME_CLIENT_HPP
#define GAME_CLIENT_HPP

#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring>

#include <unistd.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "maze.hpp"
#include "maze_protocol.hpp"

/** Запись истории — команда и ответ сервера */
struct HistoryEntry
{
    std::string command;  ///< Отправленная команда
    std::string response; ///< Полученный ответ
};
/** Класс TCP-клиента игры "Лабиринт"
 * @details Реализует консольный интерфейс: отображает состояние лабиринта
 * историю ходов, оставшееся число ходов и принимает команды игрока
 * Для неблокирующего ввода с клавиатуры используется неканонический режим терминала
 */
class GameClient
{
public:
    /** Создаёт клиента
     * @param server_ip IP-адрес или имя хоста сервера
     * @param port Порт сервера
     * @param player_name Имя игрока
     * @param perf_mode Флаг режима тестирования (без лимита ходов)
     */
    GameClient(const std::string &server_ip, unsigned short port, const std::string &player_name, bool perf_mode = false)
        : _server_ip(server_ip), _port(port), _player_name(player_name), _perf_mode(perf_mode), _socket(-1), _moves_left(0), _game_over(false)
    {
    }

    /** Закрывает сокет при удалении */
    ~GameClient()
    {
        if (_socket >= 0)
        {
            close(_socket);
        }
    }

    /** Запускает клиент */
    void run();

private:
    /** Устанавливает TCP-соединение с сервером
     * @throw std::runtime_error При ошибках соединения
     */
    void connect();

    /** Отправляет начальное сообщение (HELLO или PERF) */
    void sendInit();

    /** Отправляет строку через сокет
     * @param msg Сообщение без завершающего \\n
     */
    void sendMsg(const std::string &msg);

    /** Принимает сообщение от сервера
     * @return Строка сообщения без \\n
     */
    std::string recvMsg();

    /** Отображает состояние игры в консоли */
    void renderUI();

    /** Считывает команду с клавиатуры (неблокирующе).
    * @return Символ клавиши: w/a/s/d/q или '\\0' 
    */
    char readKey();

    /** Настраивает терминал для неканонического ввода без echo */
    void setupTerminal();

    /** Восстанавливает исходные настройки терминала */
    void restoreTerminal();

    /** Добавляет стену в локальную модель при столкновении
     * @param cmd Команда, вызвавшая столкновение
     */
    void addWallFromCollision(const std::string &cmd);

    /** Обрабатывает ответ сервера и обновляет состояние клиента
     * @param response Строка ответа сервера
     * @param cmd Команда, на которую получен ответ
     */
    void processResponse(const std::string &response, const std::string &cmd);

    std::string _server_ip;             ///< Адрес сервера
    unsigned short _port;               ///< Порт сервера
    std::string _player_name;           ///< Имя игрока
    bool _perf_mode;                    ///< Флаг режима тестирования
    int _socket;                        ///< TCP-сокет
    int _moves_left;                    ///< Оставшиеся ходы (из ответов сервера)
    bool _game_over;                    ///< Флаг завершения игры
    Maze _maze;                         ///< Локальная модель с известными стенами
    std::vector<HistoryEntry> _history; ///< История ходов
    struct termios _saved_tty;          ///< Сохранённые настройки терминала
    char _buffer[1024];                 ///< Буфер приёма данных
};

void GameClient::run()
{
    setupTerminal();
    connect();
    sendInit();

    std::cout << "\033[2J\033[H"; // очистка экрана

    // Принимаем SIZE от сервера и настраиваем локальный лабиринт
    const std::string size_msg = recvMsg();
    const auto size_tokens = Protocol::tokenize(size_msg);
    if (size_tokens.size() >= 2 && size_tokens[0] == Protocol::RES_SIZE)
    {
        _maze.setSize(static_cast<unsigned>(std::stoi(size_tokens[1])));
    }

    renderUI();

    while (!_game_over)
    {
        char key = '\0';
        while (key == '\0' && !_game_over)
        {
            key = readKey();
            if (key == '\0')
            {
                usleep(30000); // 30 мс
            }
        }
        if (_game_over)
            break;

        std::string cmd;
        switch (key)
        {
        case 'w':
            cmd = Protocol::REQ_FORWARD;
            break;
        case 'a':
            cmd = Protocol::REQ_LEFT;
            break;
        case 's':
            cmd = Protocol::REQ_BACK;
            break;
        case 'd':
            cmd = Protocol::REQ_RIGHT;
            break;
        case 'q':
            cmd = Protocol::REQ_GIVEUP;
            break;
        default:
            continue;
        }

        sendMsg(cmd);
        const std::string response = recvMsg();
        _history.push_back({cmd, response});
        processResponse(response, cmd);
        renderUI();
    }
    restoreTerminal();
}

void GameClient::connect()
{
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0)
    {
        throw std::runtime_error("GameClient: socket creation failed");
    }

    struct addrinfo hints;
    struct addrinfo *result = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    const int gai_err = getaddrinfo(_server_ip.c_str(), nullptr, &hints, &result);
    if (gai_err != 0 || result == nullptr)
    {
        throw std::runtime_error("GameClient: host not found");
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    std::memcpy(&addr.sin_addr, &reinterpret_cast<sockaddr_in *>(result->ai_addr)->sin_addr, sizeof(addr.sin_addr));
    addr.sin_port = htons(_port);
    freeaddrinfo(result);

    if (::connect(_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        throw std::runtime_error("GameClient: connect failed");
    }
}

void GameClient::sendInit()
{
    if (_perf_mode)
    {
        sendMsg(Protocol::makePerfReq(_player_name));
    }
    else
    {
        sendMsg(Protocol::makeHelloReq(_player_name));
    }
}

void GameClient::sendMsg(const std::string &msg)
{
    const std::string framed = msg + "\n";
    static_cast<void>(send(_socket, framed.data(), framed.size(), 0));
}

std::string GameClient::recvMsg()
{
    std::string msg;
    while (true)
    {
        const ssize_t n = recv(_socket, _buffer, sizeof(_buffer) - 1, 0);
        if (n <= 0)
        {
            throw std::runtime_error("GameClient: server disconnected");
        }
        _buffer[n] = '\0';
        msg += _buffer;
        if (!msg.empty() && msg.back() == '\n')
        {
            msg.pop_back();
            return msg;
        }
    }
}

void GameClient::renderUI()
{
    std::cout << "\033[H";  // курсор в начало экрана
    std::cout << "\033[0J"; // очистка от курсора до конца

    std::cout << "=========== ЛАБИРИНТ ===========\n";
    std::cout << "Игрок: " << _player_name;
    if (_perf_mode)
        std::cout << "  [тестовый режим]";
    std::cout << "\n";
    if (!_perf_mode)
    {
        std::cout << "Осталось ходов: " << _moves_left << "\n";
    }
    std::cout << "\n";

    _maze.print(std::cout);
    std::cout << "\n";

    std::cout << "P - игрок,  G - цель\n";
    std::cout << "|--- : стена,      : проход\n\n";
    std::cout << "  [w] вперёд  [a] влево  [s] назад  [d] вправо  [q] сдаться\n\n";

    std::cout << "--- Последние ходы ---\n";
    const int start = static_cast<int>(_history.size()) > 6 ? static_cast<int>(_history.size()) - 6 : 0;
    for (size_t i = static_cast<size_t>(start); i < _history.size(); i++)
    {
        const auto &e = _history[i];
        std::cout << "  [" << (i + 1) << "] " << e.command << " -> " << e.response << "\n";
    }
    std::cout.flush();
}

char GameClient::readKey()
{
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0)
    {
        return c;
    }
    return '\0';
}

void GameClient::setupTerminal()
{
    tcgetattr(STDIN_FILENO, &_saved_tty);
    struct termios tty = _saved_tty;
    tty.c_lflag &= ~(ICANON | ECHO);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    std::cout << "\033[?25l" << std::flush; // скрыть курсор
}

void GameClient::restoreTerminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &_saved_tty);
    std::cout << "\033[?25h\n"
              << std::flush; // показать курсор
}

void GameClient::addWallFromCollision(const std::string &cmd)
{
    const int px = _maze.playerX();
    const int py = _maze.playerY();
    int tx = px, ty = py;
    if (cmd == Protocol::REQ_FORWARD)
    {
        ty = py + 1;
    }
    else if (cmd == Protocol::REQ_RIGHT)
    {
        tx = px + 1;
    }
    else if (cmd == Protocol::REQ_LEFT)
    {
        tx = px - 1;
    }
    else if (cmd == Protocol::REQ_BACK)
    {
        ty = py - 1;
    }
    else
        return;
    _maze.addWall(px, py, tx, ty);
}

void GameClient::processResponse(const std::string &response, const std::string &cmd)
{
    const auto tokens = Protocol::tokenize(response);
    if (tokens.empty())
    {
        return;
    }
    if (tokens[0] == Protocol::RES_OK)
    {
        if (tokens.size() >= 3)
        {
            const int x = std::stoi(tokens[1]);
            const int y = std::stoi(tokens[2]);
            _maze.setPlayerPosition(x, y);
        }
        if (tokens.size() >= 4)
        {
            _moves_left = std::stoi(tokens[3]);
        }
    }
    else if (tokens[0] == Protocol::RES_WALL)
    {
        addWallFromCollision(cmd);
        if (tokens.size() >= 2)
        {
            _moves_left = std::stoi(tokens[1]);
        }
    }
    else if (tokens[0] == Protocol::RES_WIN)
    {
        _game_over = true;
    }
    else if (tokens[0] == Protocol::RES_LOSE)
    {
        _game_over = true;
    }
}

#endif
