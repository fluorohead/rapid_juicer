#include "engine.h"
#include "walker.h"
#include <QThread>
#include <QDebug>
#include <QDateTime>

QMap <QString, Signature> signatures { // значения будут автоматически упорядочены по ключам
    // ключ (он же сигнатура формата)
    // |
    // V
    { "special",    {0x000000003052455A, 4, Engine::recognize_special   } }, // "ZER0" zero-phase buffer filler
    { "bmp",        {0x0000000000004D42, 2, Engine::recognize_special       } }, // "BM"
    { "pcx4_8",     {0x000000000801040A, 4, Engine::recognize_special   } }, // временная заглушка
    { "pcx4_24",    {0x000000001801040A, 4, Engine::recognize_special   } }, // временная заглушка
    { "pcx5_8",     {0x000000000801050A, 4, Engine::recognize_special   } }, // временная заглушка
    { "pcx5_24",    {0x000000001801050A, 4, Engine::recognize_special   } }, // временная заглушка
    { "png",        {0x00000000474E5089, 4, Engine::recognize_special       } }, // "\x89PNG"
    { "riff",       {0x0000000046464952, 4, Engine::recognize_special      } }, // "RIFF"
    { "iff",        {0x000000004D524F46, 4, Engine::recognize_special       } }, // "FORM"
    { "gif",        {0x0000000038464947, 4, Engine::recognize_special       } }, // "GIF8"
    { "tiff_ii",    {0x00000000002A4949, 4, Engine::recognize_special   } }, // "II\0x2A\0x00"
    { "tiff_mm",    {0x000000002A004D4D, 4, Engine::recognize_special   } }, // "MM\0x00\0x2A"
    { "tga_tc32",   {0x0000000000020000, 4, Engine::recognize_special  } }, // "\0x00\0x00\0x02\0x00"
    { "jfif_soi",   {0x00000000E0FFD8FF, 4, Engine::recognize_special  } }, // "\0xFF\0xD8\0xFF\0xE0"
    { "jfif_eoi",   {0x000000000000D9FF, 2, Engine::recognize_special  } }, // "\0xFF\0xD9" маркер конца изображения
};

// функция-заглушка для обработки технической сигнатуры
RECOGNIZE_FUNC_RETURN Engine::recognize_special RECOGNIZE_FUNC_HEADER {
    return 0;
}

Engine::Engine(WalkerThread *walker_parent)
    : my_walker_parent(walker_parent)
{
    command = &my_walker_parent->command;
    control_mutex = my_walker_parent->walker_control_mutex;
    scrupulous = my_walker_parent->walker_config.scrupulous;
    read_buffer_size = Settings::getBufferSizeByIndex(my_walker_parent->walker_config.bfr_size_idx);
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
            control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
            // тут вдруг в главном потоке разблокировали mutex, поэтому пошли выполнять код ниже (пришла неявная команда Resume(Run))
            control_mutex->unlock();
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
