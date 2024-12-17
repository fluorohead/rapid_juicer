#include "engine.h"
#include "walker.h"
#include <QThread>
#include <QDebug>
#include <QDateTime>

QMap <QString, Signature> signatures { // в QMap значения будут автоматически упорядочены по ключам
    // ключ (он же сигнатура формата)
    // |
    // V                       00RREEZZ
    { "special",    {0x000000003052455A, 4, Engine::recognize_special } }, // "ZER0" zero-phase buffer filler
    { "bmp",        {0x0000000000004D42, 2, Engine::recognize_special } }, // "BM"
    { "pcx4_8",     {0x000000000801040A, 4, Engine::recognize_special } }, // временная заглушка
    { "pcx4_24",    {0x000000001801040A, 4, Engine::recognize_special } }, // временная заглушка
    { "pcx5_8",     {0x000000000801050A, 4, Engine::recognize_special } }, // временная заглушка
    { "pcx5_24",    {0x000000001801050A, 4, Engine::recognize_special } }, // временная заглушка
    { "png",        {0x00000000474E5089, 4, Engine::recognize_special } }, // "\x89PNG"
    { "riff",       {0x0000000046464952, 4, Engine::recognize_special } }, // "RIFF"
    { "iff",        {0x000000004D524F46, 4, Engine::recognize_special } }, // "FORM"
    { "gif",        {0x0000000038464947, 4, Engine::recognize_special } }, // "GIF8"
    { "tiff_ii",    {0x00000000002A4949, 4, Engine::recognize_special } }, // "II\0x2A\0x00"
    { "tiff_mm",    {0x000000002A004D4D, 4, Engine::recognize_special } }, // "MM\0x00\0x2A"
    { "tga_tc32",   {0x0000000000020000, 4, Engine::recognize_special } }, // "\0x00\0x00\0x02\0x00"
    { "jfif_soi",   {0x00000000E0FFD8FF, 4, Engine::recognize_special } }, // "\0xFF\0xD8\0xFF\0xE0"
    { "jfif_eoi",   {0x000000000000D9FF, 2, Engine::recognize_special } }, // "\0xFF\0xD9" маркер конца изображения
};

const u32i Engine::special_signature = signatures["special"].as_u64i;

Engine::Engine(WalkerThread *walker_parent)
    : my_walker_parent(walker_parent)
{
    command = &my_walker_parent->command;
    control_mutex = my_walker_parent->walker_control_mutex;
    scrupulous = my_walker_parent->walker_config.scrupulous;
    read_buffer_size = Settings::getBufferSizeByIndex(my_walker_parent->walker_config.bfr_size_idx) * 1024 * 1024;
    qInfo() << "READ_BUFFER_SIZE" << read_buffer_size;
    total_buffer_size = read_buffer_size + MAX_SIGNATURE_SIZE/*4*/;
    amount_dw = my_walker_parent->amount_dw;
    amount_w  = my_walker_parent->amount_w;

    scanbuf_ptr = new u8i[total_buffer_size];
    auxbuf_ptr  = new u8i[AUX_BUFFER_SIZE];
    fillbuf_ptr = scanbuf_ptr + MAX_SIGNATURE_SIZE/*4*/;
}

Engine::~Engine()
{
    delete [] scanbuf_ptr;
    delete [] auxbuf_ptr;
    qInfo() << "Engine destructor called in thread id" << QThread::currentThreadId();
}

void Engine::scan_file_v1(const QString &file_name)
{
    file.setFileName(file_name);
    if ( ( !file.open(QIODeviceBase::ReadOnly) ) or ( (file.size() < MIN_RESOURCE_SIZE) ) )
    {
        return;
    }

    ////// объявление рабочих переменных на стеке (так эффективней, чем в классе) //////
    s64i last_read_amount; // количество прочитанного из файла за последнюю операцию чтения
    bool zero_phase; // индикатор нулевой фазы, когда первые 4 байта заполнены специальной сигнатурой
    u64i scanbuf_offset; // текущее смещение в буфере scanbuf_ptr
    u32i analyzed_dword;
    TreeNode *node;
    TreeNode *tree_dw = my_walker_parent->tree_dw;
    TreeNode *tree_w  = my_walker_parent->tree_w;
    u64i resource_size;
    s64i save_restore_seek; // перед вызовом recognizer'а запоминаем последнюю позицию в файле, т.к. recognizer может перемещать позицию для дополнительных чтений
    s64i file_size = file.size();
    //////////////////////////////////////////////////////////////

    ////// выставление начальных значений важных переменных //////
    previous_file_progress = 0;
    previous_msecs = QDateTime::currentMSecsSinceEpoch();
    zero_phase = true;
    *(u32i *)(&scanbuf_ptr[0]) = special_signature;
    signature_file_pos = 0 - MAX_SIGNATURE_SIZE; // начинаем со значения -4, чтобы компенсировать нулевую фазу
    //////////////////////////////////////////////////////////////

    do /// цикл итерационных чтений из файла
    {

        last_read_amount = file.read((char *)fillbuf_ptr, read_buffer_size); // размер хвоста
        // на нулевой фазе минимальный хвост должен быть >= MIN_RESOURCE_SIZE;
        // на ненелувой фазе должен быть >= (MIN_RESOURCE_SIZE - 4), т.к. в начальных 4 байтах буфера уже что-то есть и это не спец-заполнитель
        if ( last_read_amount < (MIN_RESOURCE_SIZE - (!zero_phase) * MAX_SIGNATURE_SIZE) ) // ищём только в достаточном отрезке, иначе пропускаем короткий файл или недостаточно длинный хвост
        {
            break; // слишком короткий хвост -> выход из do-while итерационных чтений
        }

        for (scanbuf_offset = 0; scanbuf_offset < last_read_amount; ++scanbuf_offset) // цикл прохода по буферу dword-сигнатур
        {
            node = &tree_dw[0]; // для каждого очередного dword'а выставляем текущий узел в [0] индекс дерева, т.к. у бинарных деревьев это корень
            analyzed_dword = *(u32i *)(scanbuf_ptr + scanbuf_offset);

            do /// цикл обхода авл-дерева
            {
                if ( node != nullptr )
                {
                    if ( analyzed_dword == node->signature.as_u64i ) // совпадение сигнатуры!
                    {
                        if ( file.size() - signature_file_pos >= MIN_RESOURCE_SIZE )
                        {
                            save_restore_seek = file.pos();
                            resource_size = node->signature.recognizer_ptr(this); // вызов функции-распознавателя через указатель
                            file.seek(save_restore_seek);
                        }
                        break;
                    }
                    if ( analyzed_dword > node->signature.as_u64i )
                    { // переход в правое поддерево
                        node = node->right;
                    }
                    else
                    { // иначе переход в левое поддерево
                        node = node->left;
                    }
                }
                else // достигли nullptr, значит в дереве не было совпадений
                {
                    break;
                }
            } while ( true );
            /// end of avl-tree do-while

            ++signature_file_pos; // счётчик позиции сигнатуры в файле
         }
        /// end of for

        //qInfo() << "  :: iteration reading from file to buffer";
        //QThread::msleep(2000);

        switch (*command) // проверка на поступление команды управления
        {
        case WalkerCommand::Stop:
            qInfo() << "-> Engine: i'm stopped due to Stop command";
            last_read_amount = 0;  // условие выхода из внешнего цикла do-while итерационных чтений файла
            break;
        case WalkerCommand::Pause:
            qInfo() << "-> Engine: i'm paused due to Pause command";
            emit my_walker_parent->txImPaused();
            control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
            // тут вдруг в главном потоке разблокировали mutex, поэтому пошли выполнять код ниже (пришла неявная команда Resume(Run))
            control_mutex->unlock();
            if ( *command == WalkerCommand::Stop ) // вдруг, пока мы стояли на паузе, была нажата кнопка Stop?
            {
                last_read_amount = 0; // условие выхода из внешнего цикла do-while итерационных чтений файла
                break;
            }
            emit my_walker_parent->txImResumed();
            qInfo() << " >>>> Engine : received Resume(Run) command, when Engine was running!";
            break;
        case WalkerCommand::Skip:
            qInfo() << " >>>> Engine : current file skipped :" << file_name;
            *command = WalkerCommand::Run;
            last_read_amount = 0;  // условие выхода из внешнего цикла do-while итерационных чтений файла
            break;
        default:; // сюда в случае WalkerCommand::Run
        }

        update_file_progress(file_name, file_size, signature_file_pos + 4); // посылаем сигнал обновить progress bar для файла
        *(u32i *)(&scanbuf_ptr[0]) = *(u32i *)(&scanbuf_ptr[read_buffer_size]); // копируем последние 4 байта в начало буфера scanbuf_ptr
        zero_phase = false;

    } while ( last_read_amount == read_buffer_size );
    /// end of do-while итерационных чтений из файла

    qInfo() << "closing file";
    qInfo() << "-> Engine: returning from scan_file() to caller WalkerThread";
    file.close();
}

void Engine::scan_file_v2(const QString &file_name)
{
    file.setFileName(file_name);
    if ( ( !file.open(QIODeviceBase::ReadOnly) ) or ( (file.size() < MIN_RESOURCE_SIZE) ) )
    {
        return;
    }

    ////// объявление рабочих переменных на стеке (так эффективней, чем в классе) //////
    s64i last_read_amount; // количество прочитанного из файла за последнюю операцию чтения
    bool zero_phase; // индикатор нулевой фазы, когда первые 4 байта заполнены специальной сигнатурой
    u64i scanbuf_offset; // текущее смещение в буфере scanbuf_ptr
    u32i analyzed_dword;
    u64i resource_size;
    s64i save_restore_seek; // перед вызовом recognizer'а запоминаем последнюю позицию в файле, т.к. recognizer может перемещать позицию для дополнительных чтений
    s64i file_size = file.size();

    u64i selected_signatures_array[amount_dw]; // на стэке; массив будет неупорядоченным
    u64i selected_recognizers_array[amount_dw];

    int s_idx = 0;
    for (auto &&signature_name: my_walker_parent->uniq_signature_names_dw)
    {
        qInfo() << "uniq_signature_selected:" << signature_name;
        selected_signatures_array[s_idx] = signatures[signature_name].as_u64i;
        selected_recognizers_array[s_idx] = (u64i) signatures[signature_name].recognizer_ptr;
        ++s_idx;
    }
    qInfo() << "amount_dw:" << amount_dw;
    //////////////////////////////////////////////////////////////

    ////// выставление начальных значений важных переменных //////
    previous_file_progress = 0;
    previous_msecs = QDateTime::currentMSecsSinceEpoch();
    zero_phase = true;
    *(u32i *)(&scanbuf_ptr[0]) = special_signature;
    signature_file_pos = 0 - MAX_SIGNATURE_SIZE; // начинаем со значения -4, чтобы компенсировать нулевую фазу
    //////////////////////////////////////////////////////////////

    do /// цикл итерационных чтений из файла
    {
        last_read_amount = file.read((char *)fillbuf_ptr, read_buffer_size); // размер хвоста
        // на нулевой фазе минимальный хвост должен быть >= MIN_RESOURCE_SIZE;
        // на ненелувой фазе должен быть >= (MIN_RESOURCE_SIZE - 4), т.к. в начальных 4 байтах буфера уже что-то есть и это не спец-заполнитель
        if ( last_read_amount < (MIN_RESOURCE_SIZE - (!zero_phase) * MAX_SIGNATURE_SIZE) ) // ищём только в достаточном отрезке, иначе пропускаем короткий файл или недостаточно длинный хвост
        {
            break; // слишком короткий хвост -> выход из do-while итерационных чтений
        }

        for (scanbuf_offset = 0; scanbuf_offset < last_read_amount; ++scanbuf_offset) // цикл прохода по буферу dword-сигнатур
        {
            analyzed_dword = *(u32i *)(scanbuf_ptr + scanbuf_offset);

            for (int s_idx = 0; s_idx < amount_dw; ++s_idx)
            {
                if ( analyzed_dword == selected_signatures_array[s_idx])
                {
                    recognize_special(this);
                    break;
                }
            }

            ++signature_file_pos; // счётчик позиции сигнатуры в файле
        }
        /// end of for

        //qInfo() << "  :: iteration reading from file to buffer";
        //QThread::msleep(2000);

        switch (*command) // проверка на поступление команды управления
        {
        case WalkerCommand::Stop:
            qInfo() << "-> Engine: i'm stopped due to Stop command";
            last_read_amount = 0;  // условие выхода из внешнего цикла do-while итерационных чтений файла
            break;
        case WalkerCommand::Pause:
            qInfo() << "-> Engine: i'm paused due to Pause command";
            emit my_walker_parent->txImPaused();
            control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
            // тут вдруг в главном потоке разблокировали mutex, поэтому пошли выполнять код ниже (пришла неявная команда Resume(Run))
            control_mutex->unlock();
            if ( *command == WalkerCommand::Stop ) // вдруг, пока мы стояли на паузе, была нажата кнопка Stop?
            {
                last_read_amount = 0; // условие выхода из внешнего цикла do-while итерационных чтений файла
                break;
            }
            emit my_walker_parent->txImResumed();
            qInfo() << " >>>> Engine : received Resume(Run) command, when Engine was running!";
            break;
        case WalkerCommand::Skip:
            qInfo() << " >>>> Engine : current file skipped :" << file_name;
            *command = WalkerCommand::Run;
            last_read_amount = 0;  // условие выхода из внешнего цикла do-while итерационных чтений файла
            break;
        default:; // сюда в случае WalkerCommand::Run
        }

        update_file_progress(file_name, file_size, signature_file_pos + 4); // посылаем сигнал обновить progress bar для файла
        *(u32i *)(&scanbuf_ptr[0]) = *(u32i *)(&scanbuf_ptr[read_buffer_size]); // копируем последние 4 байта в начало буфера scanbuf_ptr
        zero_phase = false;

    } while ( last_read_amount == read_buffer_size );
    /// end of do-while итерационных чтений из файла

    qInfo() << "closing file";
    qInfo() << "-> Engine: returning from scan_file() to caller WalkerThread";
    file.close();
}

void Engine::scan_file_v4(const QString &file_name)
{
    file.setFileName(file_name);
    file_size = file.size();
    qInfo() << "file_size:" << file_size;
    if ( ( !file.open(QIODeviceBase::ReadOnly) ) or ( ( file.size() < MIN_RESOURCE_SIZE ) ) )
    {
        return;
    }
    uchar *map_scanbuf_ptr = file.map(0, file_size);
    if ( map_scanbuf_ptr == nullptr )
    {
        qInfo() << "unsuccesfull file to memory mapping";
        return;
    }
    ////// объявление рабочих переменных на стеке (так эффективней, чем в классе) //////
    u64i start_offset;
    u64i last_offset = 0; // =0 - важно!
    u32i analyzed_dword;
    u64i iteration;
    u64i granularity = Settings::getBufferSizeByIndex(my_walker_parent->walker_config.bfr_size_idx) * 1024 * 1024;
    u64i tale_size = file_size % granularity;
    u64i max_iterations = file_size / granularity + ((tale_size == 0) ? 0 : 1);
    qInfo() << "max_iterations:" << max_iterations;
    //////////////////////////////////////////////////////////////

    for (iteration = 1; iteration <= max_iterations; ++iteration)
    {
        start_offset = last_offset;
        last_offset = ( iteration != max_iterations ) ? (last_offset += granularity) : (last_offset += tale_size);
        for (scanbuf_offset = start_offset; scanbuf_offset < last_offset; ++scanbuf_offset)
        {
            analyzed_dword = *(u32i*)(map_scanbuf_ptr + scanbuf_offset);
            /// сравнение с сигнатурами
            {
        words:
            switch ((u16i)analyzed_dword) // усечение старших 16 бит, чтобы остались только младшие 16
            {
                case 0x4D42:
                    recognize_special(this);
                    break;
                case 0xD9FF:
                    recognize_special(this);
                    break;
            }
        dwords:
            if ( analyzed_dword >= 0x2A004D4D ) goto second_half;
        first_half:
            switch (analyzed_dword)
            {
                case 0x00020000:
                    recognize_special(this);
                    break;
                case 0x002A4949:
                    recognize_special(this);
                    break;
                case 0x0801040A:
                    recognize_special(this);
                    break;
                case 0x0801050A:
                    recognize_special(this);
                    break;
                case 0x1801040A:
                    recognize_special(this);
                    break;
                case 0x1801050A:
                    recognize_special(this);
                    break;
            }
            goto end;
        second_half:
            switch (analyzed_dword)
            {
                case 0x2A004D4D:
                    recognize_special(this);
                    break;
                case 0x38464947:
                    recognize_special(this);
                    break;
                case 0x46464952:
                    recognize_special(this);
                    break;
                case 0x474E5089:
                    recognize_special(this);
                    break;
                case 0x4D524F46:
                    recognize_special(this);
                    break;
                case 0xE0FFD8FF:
                    recognize_special(this);
                    break;
            }
        end:
            ;
            }
        }
        switch (*command) // проверка на поступление команды управления
        {
            case WalkerCommand::Stop:
                qInfo() << "-> Engine: i'm stopped due to Stop command";
                iteration = max_iterations;  // условие выхода из внешнего цикла do-while итерационных чтений файла
                break;
            case WalkerCommand::Pause:
                qInfo() << "-> Engine: i'm paused due to Pause command";
                emit my_walker_parent->txImPaused();
                control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
                // тут вдруг в главном потоке разблокировали mutex, поэтому пошли выполнять код ниже (пришла неявная команда Resume(Run))
                control_mutex->unlock();
                if ( *command == WalkerCommand::Stop ) // вдруг, пока мы стояли на паузе, была нажата кнопка Stop?
                {
                    iteration = max_iterations; // условие выхода из внешнего цикла do-while итерационных чтений файла
                    break;
                }
                emit my_walker_parent->txImResumed();
                qInfo() << " >>>> Engine : received Resume(Run) command, when Engine was running!";
                break;
            case WalkerCommand::Skip:
                qInfo() << " >>>> Engine : current file skipped :" << file_name;
                *command = WalkerCommand::Run;
                iteration = max_iterations;  // условие выхода из внешнего цикла do-while итерационных чтений файла
                break;
            default:; // сюда в случае WalkerCommand::Run
        }
        update_file_progress(file_name, file_size, scanbuf_offset); // посылаем сигнал обновить progress bar для файла
     }

    qInfo() << "closing file";
    qInfo() << "-> Engine: returning from scan_file() to caller WalkerThread";
    file.unmap(map_scanbuf_ptr);
    file.close();
}

inline void Engine::update_file_progress(const QString &file_name, u64i file_size, s64i total_readed_bytes)
{
    s64i current_file_progress = (total_readed_bytes * 100) / file_size;
    if ( current_file_progress == previous_file_progress )
    {
        return;
    }
    s64i current_msecs = QDateTime::currentMSecsSinceEpoch();
    if ( ( current_msecs < previous_msecs ) or ( current_msecs - previous_msecs >= MIN_SIGNAL_INTERVAL_MSECS ) )
    {
        previous_msecs = current_msecs;
        previous_file_progress = current_file_progress;
        emit txFileProgress(file_name, current_file_progress);
        //return; // нужен только в случае, если далее идёт qInfo(), иначе return можно удалить
    }
//    qInfo() << "    !!! Engine :: NO! I WILL NOT SEND SIGNALS CAUSE IT'S TOO OFTEN!";
}

bool Engine::enough_room_to_proceed(u64i min_size)
{
    if ( file_size - scanbuf_offset >= min_size )
    {
        return true;
    }
    return false;
}

// функция-заглушка для обработки технической сигнатуры
RECOGNIZE_FUNC_RETURN Engine::recognize_special RECOGNIZE_FUNC_HEADER
{
    if ( e->enough_room_to_proceed(10) )
    {
        ++(e->hits);
    }

    return 0;
}
