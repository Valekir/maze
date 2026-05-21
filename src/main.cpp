/** @file main.cpp
 * Главный файл приложения игры "Лабиринт"
 * @details В зависимости от флага запускается в режиме сервера (-s) или клиента (-c)
 */

#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include "game_server.hpp"
#include "game_client.hpp"

/** Выводит справку по аргументам запуска.
@param name Имя исполняемого файла */
void printUsage(const char *name, std::ostream &os = std::cerr)
{
    os << "Использование:\n"
       << "  Сервер:  " << name << " -s [-p порт] [-m лимит_ходов] [-z размер] [-w стен]\n"
       << "    -p порт          Порт прослушивания (по умолчанию 4321)\n"
       << "    -m лимит_ходов   Число ходов в обычном режиме (по умолчанию 10)\n"
       << "    -z размер        Размер стороны лабиринта (по умолчанию 3)\n"
       << "    -w стен          Число генерируемых стен (по умолчанию size*(size-1))\n"
       << "\n"
       << "  Клиент:  " << name << " -c [-n имя] [-a адрес] [-p порт] [-t]\n"
       << "    -n имя     Имя игрока (обязательно)\n"
       << "    -a адрес   IP-адрес сервера (по умолчанию 127.0.0.1)\n"
       << "    -p порт    Порт сервера (по умолчанию 4321)\n"
       << "    -t         Режим тестирования (без лимита ходов)\n"
       << "\n"
       << "  Для вывода справки:  " << name << " -h"
       << "\n";
}

/**
 * @brief Запуск в режиме сервера
 *
 * @param argc Число аргументов
 * @param argv Массив строк аргументов
 * @return Код возврата
 */
static int runServer(int argc, char **argv)
{
    unsigned short port = 4321;
    int max_moves = 10;
    unsigned maze_size = 3u;
    unsigned wall_count = 0;
    bool wall_count_set = false;

    int opt;
    while ((opt = getopt(argc, argv, "p:m:z:w:c:s")) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            const int val = std::atoi(optarg);
            if (val <= 0 || val > 65535)
            {
                std::cerr << "Ошибка: порт должен быть в диапазоне 1-65535\n";
                return 1;
            }
            port = static_cast<unsigned short>(val);
            break;
        }
        case 'm':
        {
            const int val = std::atoi(optarg);
            if (val <= 0)
            {
                std::cerr << "Ошибка: лимит ходов должен быть положительным\n";
                return 1;
            }
            max_moves = val;
            break;
        }
        case 'z':
        {
            const int val = std::atoi(optarg);
            if (val < 2 || val > 10)
            {
                std::cerr << "Ошибка: размер лабиринта должен быть от 2 до 10\n";
                return 1;
            }
            maze_size = static_cast<unsigned>(val);
            break;
        }
        case 'w':
        {
            const int val = std::atoi(optarg);
            if (val < 0)
            {
                std::cerr << "Ошибка: число стен не может быть отрицательным\n";
                return 1;
            }
            if (val > static_cast<int>(2 * maze_size * (maze_size - 1)))
            {
                std::cerr << "Количество стен превышает возможное\n";
                return 1;
            }
            wall_count = static_cast<unsigned>(val);
            wall_count_set = true;
            break;
        }
        case 'c':
        case 's':
            break;
        default:
            return 1;
        }
    }

    if (!wall_count_set)
    {
        wall_count = maze_size * (maze_size - 1);
    }

    try
    {
        GameServer server(port, max_moves, maze_size, wall_count);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Критическая ошибка сервера: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

/** Запуск в режиме клиента
 * @param argc Число аргументов
 * @param argv Массив строк аргументов
 * @return Код возврата
 */
static int runClient(int argc, char **argv)
{
    std::string server_ip = "127.0.0.1";
    unsigned short port = 4321;
    std::string player_name;
    bool perf_mode = false;

    int opt;
    while ((opt = getopt(argc, argv, "a:p:n:s:ct")) != -1)
    {
        switch (opt)
        {
        case 'a':
            server_ip = optarg;
            break;
        case 'p':
        {
            const int val = std::atoi(optarg);
            if (val <= 0 || val > 65535)
            {
                std::cerr << "Ошибка: порт должен быть в диапазоне 1-65535\n";
                return 1;
            }
            port = static_cast<unsigned short>(val);
            break;
        }
        case 'n':
            player_name = optarg;
            break;
        case 't':
            perf_mode = true;
            break;
        case 's':
        case 'c':
            break;
        default:
            return 1;
        }
    }

    if (player_name.empty())
    {
        std::cerr << "Ошибка: имя игрока обязательно (ключ -n)\n";
        return 1;
    }

    try
    {
        GameClient client(server_ip, port, player_name, perf_mode);
        client.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

/** Главная функция приложения
 * @details Ищет флаг режима (-s или -c), затем запускает соответствующую подсистему
 */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    // Ищем флаг режима
    bool is_server = false;
    bool mode_found = false;
    bool is_help = false;

    for (int i = 1; i < argc; i++)
    {
        const std::string arg(argv[i]);
        if (arg == "-s")
        {
            is_server = true;
            mode_found = true;
            break;
        }
        if (arg == "-c")
        {
            mode_found = true;
            break;
        }
        if (arg == "-h")
        {
            is_help = true;
            break;
        }
    }

    if (is_help)
    {
        printUsage(argv[0], std::cout);
        return 1;
    }

    if (!mode_found)
    {
        printUsage(argv[0]);
        return 1;
    }

    if (is_server)
    {
        return runServer(argc, argv);
    }
    else
    {
        return runClient(argc, argv);
    }
}
