#include "engine.h"
#include "walker.h"
#include <QThread>
#include <QDebug>
#include <QDateTime>

Engine::Engine(WalkerThread *walker_parent, WalkerCommand *walker_command, QMutex *walker_mutex)
    : my_walker_parent(walker_parent)
    , command(walker_command)
    , my_control_mutex(walker_mutex)
{
    previous_msecs = QDateTime::currentMSecsSinceEpoch();
}

void Engine::scan_file(const QString &file_name)
{
    /// здесь открываем новый файл

    ///

    int file_size = 1024;
    int total_readed_bytes = 0;

    for (int read_count = 0; read_count < 4; ++read_count) // эмуляция итерационных чтений файла в буфер
    {
        //// lambda
        auto scanner = [&]() /// основной движок (сейчас эмуляция)
        {
            qInfo() << "  :: iteration reading from file to buffer";
            QThread::msleep(500);
            total_readed_bytes += (1024 / 4); // искусственно, для отладки. в реальном коде переменная должна хранить действительное количество считанных байт.
        };
        //// lambda

        switch (*command)
        {
        case WalkerCommand::Run:
            scanner(); // один проход по буферу
            break;
        case WalkerCommand::Stop:
            qInfo() << "-> Engine: i'm stopped due to Stop command";
            read_count = 4;  // условие выхода из внешнего цикла (сейчас это for) чтения файла
            break;
        case WalkerCommand::Pause:
            qInfo() << "-> Engine: i'm paused due to Pause command";
            emit my_walker_parent->txImPaused();
            my_control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
            // тут вдруг в главном потоке разблокировали mutex, поэтому пошли выполнять код ниже (пришла неявная команда Resume(Run))
            my_control_mutex->unlock();
            if ( *command == WalkerCommand::Stop ) // вдруг, пока мы стояли на паузе, была нажата кнопка Stop?
            {
                read_count = 4; // условие выхода из внешнего цикла (сейчас это for) чтения файла
                break;
            }
            emit my_walker_parent->txImResumed();
            qInfo() << " >>>> Engine : received Resume(Run) command, when Engine was running!";
            break;
        case WalkerCommand::Skip:
            qInfo() << " >>>> Engine : current file skipped :" << file_name;
            *command = WalkerCommand::Run;
            return; // немедленно прекращаем сканирование и возвращаемся на место вызова в WalkerThread
            break;
        }

        update_file_progress(file_name, file_size, total_readed_bytes); // посылаем сигнал обновить progress bar для файла
    }

    /// здесь закрываем файл
    ///
    ///

    qInfo() << "-> Engine: returning from scan_file() to caller WalkerThread";
    return;
}

inline void Engine::update_file_progress(const QString &file_name, int file_size, int total_readed_bytes)
{
    s64i current_msecs = QDateTime::currentMSecsSinceEpoch();
    int file_progress = ( total_readed_bytes >= file_size ) ? 100 : (total_readed_bytes * 100) / file_size;

    if ( ( current_msecs < previous_msecs ) or ( current_msecs - previous_msecs >= MIN_SIGNAL_INTERVAL_MSECS ) )
    {
        previous_msecs = current_msecs;
        emit txFileProgress(file_name, file_progress);
        return;
    }
    //qInfo() << "    !!! Engine :: NO! I WILL NOT SEND SIGNALS CAUSE IT'S TOO OFTEN!";
}

Engine::~Engine()
{
    qInfo() << "Engine destructor called in thread id" << QThread::currentThreadId();
}
