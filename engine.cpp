#include "engine.h"
#include "walker.h"
#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <QByteArray>
extern const QMap <u32i, QString> wave_codecs;

QMap <QString, Signature> signatures { // в QMap значения будут автоматически упорядочены по ключам
    // ключ (он же сигнатура формата)
    // |
    // V                        00RREEZZ
    { "special",    { 0x000000003052455A, 4, Engine::recognize_special } }, // "ZER0" zero-phase buffer filler
    { "bmp",        { 0x0000000000004D42, 2, Engine::recognize_bmp     } }, // "BM"
    { "pcx_05",     { 0x000000000000050A, 2, Engine::recognize_pcx     } }, // "\0x0A\x05"
    { "png",        { 0x00000000474E5089, 4, Engine::recognize_png     } }, // "\x89PNG"
    { "riff",       { 0x0000000046464952, 4, Engine::recognize_riff    } }, // "RIFF"
    { "iff",        { 0x000000004D524F46, 4, Engine::recognize_iff     } }, // "FORM"
    { "gif",        { 0x0000000038464947, 4, Engine::recognize_gif     } }, // "GIF8"
    { "tiff_ii",    { 0x00000000002A4949, 4, Engine::recognize_special } }, // "II\0x2A\0x00"
    { "tiff_mm",    { 0x000000002A004D4D, 4, Engine::recognize_special } }, // "MM\0x00\0x2A"
    { "tga_tc32",   { 0x0000000000020000, 4, Engine::recognize_special } }, // "\0x00\0x00\0x02\0x00"
    { "jpg",        { 0x00000000E0FFD8FF, 4, Engine::recognize_jpg     } }, // "\0xFF\0xD8\0xFF\0xE0"
    { "mid",        { 0x000000006468544D, 4, Engine::recognize_mid     } }, // "MThd"
    { "mod_m.k.",   { 0x000000002E4B2E4D, 4, Engine::recognize_mod_mk  } }, // "M.K." SoundTracker 2.2 by Unknown/D.O.C. [Michael Kleps] and ProTracker/NoiseTracker/etc...
    { "xm",         { 0x0000000065747845, 4, Engine::recognize_xm      } }, // "Exte"
};

const u32i Engine::special_signature = signatures["special"].as_u64i;

u16i be2le(u16i be) {
    union {
        u16i as_le;
        u8i  bytes[2];
    };
    bytes[0] = be >> 8;
    bytes[1] = be;
    return as_le;
}

u32i be2le(u32i be) {
    union {
        u32i as_le;
        u8i  bytes[4];
    };
    bytes[0] = be >> 24;
    bytes[1] = be >> 16;
    bytes[2] = be >> 8 ;
    bytes[3] = be;
    return as_le;
}

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

    selected_formats = my_walker_parent->selected_formats_fast;

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
    mmf_scanbuf = file.map(0, file_size);
    if ( mmf_scanbuf == nullptr )
    {
        qInfo() << "unsuccesful file to memory mapping";
        return;
    }
    ////// объявление рабочих переменных на стеке (так эффективней, чем в классе) //////
    u64i start_offset;
    u64i last_offset = 0; // =0 - важно!
    u32i analyzed_dword;
    u64i iteration;
    u64i granularity = Settings::getBufferSizeByIndex(my_walker_parent->walker_config.bfr_size_idx) * 1024 * 1024;
    u64i tale_size = file_size % granularity;
    u64i max_iterations = file_size / granularity + ((tale_size >= 4) ? 1 : 0);
    u64i absolute_last_offset = file_size - 3; // -3, а не -4, потому что last_offset не включительно в цикле for : [start_offset, last_offset)
    u64i resource_size = 0;
    //////////////////////////////////////////////////////////////

    for (iteration = 1; iteration <= max_iterations; ++iteration)
    {
        start_offset = last_offset;
        last_offset = ( iteration != max_iterations ) ? (last_offset += granularity) : absolute_last_offset;

        for (scanbuf_offset = start_offset; scanbuf_offset < last_offset; ++scanbuf_offset)
        {
            analyzed_dword = *(u32i*)(mmf_scanbuf + scanbuf_offset);
            resource_size = 0;
            /// сравнение с сигнатурами
            {
        words:
            switch ((u16i)analyzed_dword) // "усечение" старших 16 бит, чтобы остались только младшие 16
            {
            case 0x050A: // PCX
                // ToDo: можно здесь же проверить на encoding==1, не вызывая recognizer
                resource_size = recognize_pcx(this);
                if ( resource_size ) goto end;
                break;
            case 0x4D42: // BMP
                // ToDo: можно здесь же проверить поля заголовка, не вызывая recognizer
                resource_size = recognize_bmp(this);
                if ( resource_size ) goto end;
                break;
            case 0xD9FF:
                resource_size = recognize_special(this);
                if ( resource_size ) goto end;
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
                case 0x2E4B2E4D: // MOD "M.K."
                    resource_size = recognize_mod_mk(this);
                    break;
                case 0x38464947: // GIF
                    resource_size = recognize_gif(this);
                    break;
                case 0x46464952: // RIFF
                    resource_size = recognize_riff(this);
                    break;
                case 0x474E5089: // PNG
                    resource_size = recognize_png(this);
                    break;
                case 0x4D524F46: // IFF
                    resource_size = recognize_iff(this);
                    break;
                case 0x6468544D: // MID
                    resource_size = recognize_mid(this);
                    break;
                case 0x65747845: // XM
                    resource_size = recognize_xm(this);
                    break;
                case 0xE0FFD8FF: // JPG
                    resource_size = recognize_jpg(this);
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
    file.unmap(mmf_scanbuf);
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

bool Engine::enough_room_to_continue(u64i min_size)
{
    if ( file_size - scanbuf_offset >= min_size )
    {
        return true;
    }
    return false;
}

// функция-заглушка для обработки технической сигнатуры
RECOGNIZE_FUNC_RETURN Engine::recognize_special RECOGNIZE_FUNC_HEADER
{/*
    if ( e->enough_room_to_continue(10) )
    {
        ++(e->hits);
    }*/

    return 0;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_bmp RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct FileHeader
    {
        u16i file_type; // "BM"
        u32i file_size;
        u32i reserved;  // u16i reserved1 + u16i reserved2; = 0
        u32i bitmap_offset;
        // включая BitmapHeader
        u32i bitmapheader_size; // 12, 40, 108
        s32i width;
        s32i height;
        u16i planes;
        u16i bits_per_pixel; // 1, 4, 8, 16, 24, 32
        //
    };
#pragma pack(pop)
    static const u32i bmp_id = fformats["bmp"].index;
    static constexpr u64i min_room_need = sizeof(FileHeader);
    static const QSet <u32i> VALID_BMP_HEADER_SIZE { 12, 40, 108 };
    static const QSet <u16i> VALID_BITS_PER_PIXEL { 1, 4, 8, 16, 24, 32 };
    if ( !e->selected_formats[bmp_id] ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    FileHeader *info_header = (FileHeader*)(&buffer[base_index]);
    if ( info_header->reserved != 0 ) return 0;
    if ( !VALID_BMP_HEADER_SIZE.contains(info_header->bitmapheader_size) ) return 0;
    if ( info_header->planes != 1 ) return 0;
    if ( !VALID_BITS_PER_PIXEL.contains(info_header->bits_per_pixel) ) return 0;
    s64i file_size = e->file_size;
    u64i last_index = base_index + info_header->file_size;
    qInfo() << "here";
    if ( last_index > file_size ) return 0; // неверное поле file_size
    QString info = QString("%1x%2 %3-bpp").arg( QString::number(std::abs(info_header->width)),
                                                QString::number(std::abs(info_header->height)),
                                                QString::number(info_header->bits_per_pixel));
    u64i resource_size = last_index - base_index;
    e->resource_offset = base_index;
    emit e->txResourceFound("bmp", e->file.fileName(), base_index, resource_size, info);
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_png RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct FileHeader
    {
        u64i signature;
    };
    struct ChunkHeader
    {
        u32i data_len; // need BE<>LE swap
        u32i type;
    };
    struct IHDRData
    {
        u32i width;  // need BE<>LE swap
        u32i height; // need BE<>LE swap
        u8i  bit_depth;
        u8i  color_type;
        u8i  compression;
        u8i  filter;
        u8i  interlace;
    };
    struct CRC
    {
        u32i crc;
    };
#pragma pack(pop)
    static const u32i png_id = fformats["png"].index;
    static const u64i min_room_need = sizeof(FileHeader) + sizeof(ChunkHeader) + sizeof(IHDRData) + sizeof(CRC);
    static const QSet <u8i>  VALID_BIT_DEPTH  {1, 2, 4, 8, 16};
    static const QSet <u8i>  VALID_COLOR_TYPE {0, 2, 3, 4, 6};
    static const QSet <u8i>  VALID_INTERLACE  {0, 1};
    static const QSet <u32i> VALID_CHUNK_TYPE {    0x54414449 /*IDAT*/, 0x4D524863 /*cHRM*/, 0x414D4167 /*gAMA*/, 0x54494273 /*sBIT*/, 0x45544C50 /*PLTE*/, 0x44474B62 /*bKGD*/,
                                                   0x54534968 /*hIST*/, 0x534E5274 /*tRNS*/, 0x7346466F /*oFFs*/, 0x73594870 /*pHYs*/, 0x4C414373 /*sCAL*/, 0x54414449 /*IDAT*/, 0x454D4974 /*tIME*/,
                                                   0x74584574 /*tEXt*/, 0x7458547A /*zTXt*/, 0x63415266 /*fRAc*/, 0x67464967 /*gIFg*/, 0x74464967 /*gIFt*/, 0x78464967 /*gIFx*/, 0x444E4549 /*IEND*/,
                                                   0x42475273 /*sRGB*/, 0x52455473 /*sTER*/, 0x4C414370 /*pCAL*/, 0x47495364 /*dSIG*/, 0x50434369 /*iCCP*/, 0x50434963 /*iICP*/, 0x7643446D /*mDCv*/,
                                                   0x694C4C63 /*cLLi*/, 0x74585469 /*iTXt*/, 0x744C5073 /*sPLt*/, 0x66495865 /*eXIf*/, 0x52444849 /*iHDR*/};

    if ( ( !e->selected_formats[png_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    if ( *(u64i*)buffer != 0x0A1A0A0D474E5089 /*.PNG\r\n \n*/ ) return 0;
    if ( *((u64i*)(&buffer[0x08])) != 0x524448490D000000 ) return 0;
    IHDRData *ihdr_data = (IHDRData*)(&buffer[0x10]);
    if ( !VALID_BIT_DEPTH.contains(ihdr_data->bit_depth) ) return 0;
    if ( !VALID_COLOR_TYPE.contains(ihdr_data->color_type) ) return 0;
    if ( !VALID_INTERLACE.contains(ihdr_data->interlace) ) return 0;
    u64i last_index = base_index + 8; // last_offset (индекс в массиве), выставляем на самый первый chunk iHDR
    s64i file_size = e->file_size;
    while (true)
    {
        if ( file_size - last_index < (sizeof(ChunkHeader) + sizeof(CRC)) ) return 0; // хватает ли места для очередного чанка?
        if ( !VALID_CHUNK_TYPE.contains(((ChunkHeader*)(&buffer[last_index]))->type) ) return 0; // корректен ли id чанка?
        if ( ((ChunkHeader*)(&buffer[last_index]))->type == 0x444E4549 ) // у чанка iEND не анализируем размер поля data_len, а сразу считаем, что нашли конец ресурса
        {
            last_index += sizeof(ChunkHeader) + sizeof(CRC);
            break;
        }
        last_index += ( sizeof(ChunkHeader) + be2le(((ChunkHeader*)(&buffer[last_index]))->data_len) + sizeof(CRC) );
        if ( last_index > file_size) return 0;
    };
    u64i resource_size = last_index - base_index;
    e->resource_offset = base_index;
    QString color_type;
    switch(ihdr_data->color_type) {
    case 0 :
        color_type = "grayscale";
        break;
    case 2:
        color_type = "truecolor";
        break;
    case 3:
        color_type = "color w/palette";
        break;
    case 4:
        color_type = "grayscale w/alpha";
        break;
    case 6:
        color_type = "truecolor w/alpha";
    }
    QString info = QString(R"(%1x%2 (%3))").arg(QString::number(be2le(ihdr_data->width)),
                                                QString::number(be2le(ihdr_data->height)),
                                                color_type );
    emit e->txResourceFound("png", e->file.fileName(), base_index, resource_size, info);
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_riff RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct ChunkHeader
    {
        u32i chunk_id; // "RIFF"
        u32i chunk_size;
        u64i subchunk_id; // "AVI LIST", "WAVEfmt ", "RMIDdata", "ACONLIST", "ACONanih"
        u32i subchunk_size;
    };
    struct AviInfoHeader
    {
        u64i hdrl_avih_sign;
        u32i sub_subchunk_size;
        u32i time_between_frames, max_date_rate, padding, flags, total_num_of_frames, num_of_init_frames, num_of_streams, suggested_buf_size;
        u32i width, height, time_scale, data_rate, start_time, data_len;
    };
    struct WavInfoHeader
    {
        u16i fmt_code;
        u16i chans;
        u32i sample_rate, avg_bytes_per_sec;
        u16i blk_align, bits_per_sample, ext_size, valid_bits_per_sample;
        u32i chan_mask;
        u8i  sub_format[16];
    };
    struct MidiInfoHeader
    {
        u32i chunk_type;  // "MThd"
        u32i chunk_size;
        u16i format;
        u16i ntrks; // number of tracks
        u16i division;
    };
#pragma pack(pop)
    static u32i avi_id {fformats["avi"].index};
    static u32i wav_id {fformats["wav"].index};
    static u32i rmi_id {fformats["rmi"].index};
    static u32i ani_id {fformats["animcur"].index};
    static const u64i min_room_need = sizeof(ChunkHeader);
    static const QSet <u64i> VALID_SUBCHUNK_TYPE {  0x5453494C20495641 /*AVI LIST*/,
                                                    0x20746D6645564157 /*WAVEfmt */,
                                                    0x6174616444494D52 /*RMIDdata*/,
                                                    0x5453494C4E4F4341 /*ACONLIST*/,
                                                    0x68696E614E4F4341 /*ACONanih*/
                                                    };

    if ( ( !e->selected_formats[avi_id] ) and ( !e->selected_formats[wav_id] ) and ( e->selected_formats[rmi_id] ) and ( e->selected_formats[ani_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    u64i last_index = base_index + sizeof(ChunkHeader::chunk_id) + sizeof(ChunkHeader::chunk_size) + (*((ChunkHeader*)(&buffer[base_index]))).chunk_size; // сразу переставляем last_index в конец ресурса
    if ( last_index > file_size ) return 0; // неверное поле chunk_size
    if ( !VALID_SUBCHUNK_TYPE.contains(((ChunkHeader*)(&buffer[base_index]))->subchunk_id) ) return 0; // проверка на валидные id сабчанков
    u64i resource_size = last_index - base_index;
    switch (((ChunkHeader*)(&buffer[base_index]))->subchunk_id)
    {
    case 0x5453494C20495641: // avi, "AVI LIST" subchunk
    {
        if ( ( !e->selected_formats[avi_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(AviInfoHeader)) ) return 0;
        AviInfoHeader *avi_info_header = (AviInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        if ( avi_info_header->hdrl_avih_sign != 0x686976616C726468 /*hdrlavih*/ ) return 0;
        QString info = QString(R"(%1x%2)").arg( QString::number(avi_info_header->width),
                                                QString::number(avi_info_header->height) );
        emit e->txResourceFound("avi", e->file.fileName(), base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x20746D6645564157: // wav, "WAVEfmt " subchunk
    {
        if ( ( !e->selected_formats[wav_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(WavInfoHeader)) ) return 0;
        WavInfoHeader *wav_info_header = (WavInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        QString codec_name = ( wave_codecs.contains(wav_info_header->fmt_code) ) ? wave_codecs[wav_info_header->fmt_code] : "unknown";
        QString info = QString(R"(%1 codec : %2-bit %3Hz %4-ch)").arg(  codec_name,
                                                                        QString::number(wav_info_header->bits_per_sample),
                                                                        QString::number(wav_info_header->sample_rate),
                                                                        QString::number(wav_info_header->chans) );
        emit e->txResourceFound("wav", e->file.fileName(), base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x6174616444494D52: // rmi - midi data encapsulated in riff, RMIDdata subchunk
    {
        if ( ( !e->selected_formats[rmi_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(MidiInfoHeader)) ) return 0;
        MidiInfoHeader *midi_info_header = (MidiInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        QString info = QString(R"(%1 tracks)").arg(be2le(midi_info_header->ntrks));
        emit e->txResourceFound("rmi", e->file.fileName(), base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x5453494C4E4F4341: // ani - animated cursor with ACONLIST subchunk
    {
        if ( ( !e->selected_formats[ani_id] ) ) return 0;
        emit e->txResourceFound("ani", e->file.fileName(), base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x68696E614E4F4341: // ani - animated cursor with ACONanih subchunk
    {
        if ( ( !e->selected_formats[ani_id] ) ) return 0;
        emit e->txResourceFound("ani", e->file.fileName(), base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    }
    return 0;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_mid RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct MidiChunk
    {
        u32i type;
        u32i size; // need BE<>LE swap
    };
    struct HeaderData
    {
        u16i format;
        u16i ntrks; // number of tracks; need BE<>LE swap
        u16i division;
    };
#pragma pack(pop)
    static u32i mid_id {fformats["mid"].index};
    static const u64i min_room_need = sizeof(MidiChunk) + sizeof(HeaderData);
    static const QSet <u32i> VALID_CHUNK_TYPE { 0x6468544D /*MThd*/, 0x6B72544D /*MTrk*/};
    if ( !e->selected_formats[mid_id] ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    HeaderData *info_header = (HeaderData*)(&buffer[base_index + sizeof(MidiChunk)]);
    u64i last_index = base_index + be2le((*((MidiChunk*)(&buffer[base_index]))).size) + sizeof(MidiChunk); // сразу переставляем last_index после заголовка на первый чанк типа MTrk
    if ( last_index >= file_size ) return 0; // прыгнули за пределы файла => явно неверное поле size в заголовке
    while (true)
    {
        if ( file_size - last_index < sizeof(MidiChunk) ) break; // не хватает места для анализа очередного чанка => достигли конца ресурса
        if ( !VALID_CHUNK_TYPE.contains((*(MidiChunk*)(&buffer[last_index])).type) ) break; // встретили неизвестный чанк => достигли конца ресурса
        u64i possible_last_index;
        possible_last_index = last_index + be2le((*(MidiChunk*)(&buffer[last_index])).size) + sizeof(MidiChunk);
        if ( possible_last_index > file_size ) break; // возможно кривое поле .size, либо усечённый файл, либо type попался (внезапно) корректный, но size некорректный и т.д.
        last_index = possible_last_index;
    }
    u64i resource_size = last_index - base_index;
    QString info = QString(R"(%1 tracks)").arg(be2le(info_header->ntrks));
    emit e->txResourceFound("mid", e->file.fileName(), base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_iff RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct ChunkHeader
    {
        u32i chunk_id; // "FORM"
        u32i chunk_size; // need BE<>LE swap
        u32i format_type; // "ILBM", "AIFF", "XDIR"
    };
    struct ILBM_InfoHeader
    {
        u32i local_chunk_id; // "BMHD"
        u32i size; // need BE<>LE swap
        u16i width;
        u16i height;
        u16i left, top;
        u8i  bitplanes, masking, compress, padding;
        u16i transparency;
        u8i  x_ar, y_ar;
        u16i page_width, page_height;
    };
    struct AIFF_CommonInfoHeader
    {
        u32i local_chunk_id; // "COMM"
        u32i size; // need BE<>LE swap
        u16i channels;
        u32i sample_frames;
        u16i sample_size;
        u64i sample_rate_64bits_extended; // оба поля хранят "80 bit IEEE Standard 754 floating point number"
        u16i sample_rate_16bits_extended; //
    };
    struct XDIR_CatInfoHeader
    {
        u32i cat_signature; // "CAT "
        u32i size; // need BE<>LE swap
    };
#pragma pack(pop)
    static u32i lbm_id {fformats["lbm"].index};
    static u32i aif_id {fformats["aif"].index};
    static u32i xmi_id {fformats["xmi"].index};
    static const u64i min_room_need = sizeof(ChunkHeader);
    static const QSet <u32i> VALID_FORMAT_TYPE { 0x4D424C49 /*ILBM*/, 0x46464941 /*AIFF*/, 0x52494458 /*XDIR*/ };
    if ( ( !e->selected_formats[lbm_id] ) and ( !e->selected_formats[aif_id] ) and ( !e->selected_formats[xmi_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    u64i last_index = base_index + sizeof(ChunkHeader::chunk_id) + sizeof(ChunkHeader::chunk_size) + be2le((*((ChunkHeader*)(&buffer[base_index]))).chunk_size); // сразу переставляем last_index в конец ресурса (но для XMI это лишь начало структуры CAT
    if ( last_index > file_size ) return 0; // неверное поле chunk_size
    if ( !VALID_FORMAT_TYPE.contains(((ChunkHeader*)(&buffer[base_index]))->format_type) ) return 0; // проверка на валидные форматы
    u64i resource_size = last_index - base_index;
    switch (((ChunkHeader*)(&buffer[base_index]))->format_type)
    {
    case 0x4D424C49: // "ILBM" picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(ILBM_InfoHeader)) ) return 0;
        ILBM_InfoHeader *bitmap_info_header = (ILBM_InfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        if ( bitmap_info_header->local_chunk_id != 0x44484D42 /*BMHD*/ ) return 0;
        QString info = QString(R"(%1x%2 %3-bpp)").arg(  QString::number(be2le(bitmap_info_header->width)),
                                                        QString::number(be2le(bitmap_info_header->height)),
                                                        QString::number(bitmap_info_header->bitplanes));
        emit e->txResourceFound("lbm", e->file.fileName(), base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x46464941: // "AIFF" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(AIFF_CommonInfoHeader)) ) return 0;
        AIFF_CommonInfoHeader *aiff_info_header = (AIFF_CommonInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        QString info;
        if ( aiff_info_header->local_chunk_id == 0x4D4D4F43 ) info = QString("%1-bit %2-ch").arg(   QString::number(be2le(aiff_info_header->sample_size)),
                                                                                                    QString::number(be2le(aiff_info_header->channels)));
        emit e->txResourceFound("aif", e->file.fileName(), base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x52494458: // "XDIR" midi music
    {
        if ( ( !e->selected_formats[xmi_id] ) ) return 0;
        // у формата XMI особое строение, поэтому last_index сейчас стоит не на конце ресурса, а на структуре "CAT "
        if ( last_index + sizeof(XDIR_CatInfoHeader) > file_size ) return 0;
        XDIR_CatInfoHeader* cat_info_header = (XDIR_CatInfoHeader*)(&buffer[last_index]);
        if ( cat_info_header->cat_signature != 0x20544143 /*CAT */) return 0;
        last_index += (be2le(cat_info_header->size) + sizeof(XDIR_CatInfoHeader));
        if ( last_index > file_size) return 0;
        u64i resource_size = last_index - base_index;
        emit e->txResourceFound("xmi", e->file.fileName(), base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    }
    return 0;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_pcx RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct PcxHeader
    {
        u8i  identifier;
        u8i  version;
        u8i  encoding;
        u8i  bits_per_plane;
        u16i x_start, y_start, x_end, y_end;
        u16i hor_res, ver_rez;
        u8i  palette[48], reserved;
        u8i  bitplanes;
        u16i bytes_per_line, palette_type, hor_scr_size, ver_scr_size;
        u8i  padding[54];
    };
#pragma pack(pop)
    static u32i pcx_id {fformats["pcx"].index};
    static const u64i min_room_need = sizeof(PcxHeader);
    if ( ( !e->selected_formats[pcx_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    PcxHeader* pcx_info_header = (PcxHeader*)(&(e->mmf_scanbuf[e->scanbuf_offset]));
    if ( pcx_info_header->encoding != 1 ) return 0;
    switch ( pcx_info_header->bits_per_plane )
    {
    case 1: case 2: case 4: case 8: case 24:
        break;
    default:
        return 0;
    }
    if ( pcx_info_header->x_start > pcx_info_header-> x_end ) return 0;
    if ( pcx_info_header->y_start > pcx_info_header-> y_end ) return 0;
    switch ( pcx_info_header->bitplanes )
    {
    case 1: case 3: case 4:
        break;
    default:
        return 0;
    }
    switch ( pcx_info_header->palette_type )
    {
    case 1: case 2:
        break;
    default:
        return 0;
    }
    u16i width  = pcx_info_header->x_end - pcx_info_header->x_start + 1;
    u16i height = pcx_info_header->y_end - pcx_info_header->y_start + 1;
    u8i  bpp    = pcx_info_header->bits_per_plane * pcx_info_header->bitplanes;
    u64i empiric_size;
    if ( bpp < 24 )
    {
        empiric_size = sizeof(PcxHeader) + width * height + 768 /*possible vga-palette at end*/;
    }
    else
    {
        empiric_size = sizeof(PcxHeader) + width * height * (bpp / 8);
    }
    u64i base_index = e->scanbuf_offset;
    u64i last_index = base_index + empiric_size;
    if ( last_index > e->file_size) last_index = e->file_size;
    u64i resource_size = last_index - base_index;
    QString info = QString("%1x%2 %3-bpp").arg( QString::number(width),
                                            QString::number(height),
                                            QString::number(bpp));
    emit e->txResourceFound("pcx", e->file.fileName(), base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_gif RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct FileHeader
    {
        u32i signature;
        u16i version;
        // включая Logical Screen Descriptor
        u16i width, height;
        u8i  packed, bg_color, ar;
        //
    };
    struct LocalImageDescriptor // Local Image Descriptor
    {
        u8i  separator; // 0x2C
        u16i left, top, width, height;
        u8i  packed;
        u8i  lzw_min_code_size;
    };
    struct ControlExtension
    {
        u8i separator; // 0x21
        u8i label;
    };
    struct Trailer
    {
        u8i separator; // 0x3B
    };
#pragma pack(pop)
    static u32i gif_id {fformats["gif"].index};
    static constexpr u64i min_room_need = sizeof(FileHeader) + 1; // где +1 на u8i separator
    static const QSet <u16i> VALID_VERSIONS  { 0x6137 /*7a*/, 0x6139 /*9a*/ };
    static const QSet <u8i>  EXT_LABELS_TO_PROCESS { 0x01, 0xCE, 0xFE, 0xFF };
    if ( ( !e->selected_formats[gif_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    FileHeader *info_header = (FileHeader*)(&buffer[base_index]);
    u64i colors_num = 1ULL << ((info_header->packed & 0b00000111) + 1);
    bool global_palette = info_header->packed >> 7;
    u64i last_index = base_index + sizeof(FileHeader) + 3ULL * colors_num * global_palette; // перескок через глобальную цветовую таблицу, если она есть; выставляем last_index на первый вероятный separator
    u8i block_size;
    LocalImageDescriptor *lid_info_header;
    ControlExtension *ce_info_header;
    while (true)
    {
        if ( last_index >= file_size ) return 0; // если не осталось места для separator
        switch(buffer[last_index])
        {
        case 0x2C: // Local Image Descriptor
        {
            if ( last_index + sizeof(LocalImageDescriptor) > file_size ) return 0;
            lid_info_header = (LocalImageDescriptor*)(&buffer[last_index]);
            last_index += ( sizeof(LocalImageDescriptor) +  ( 3ULL * ( 1ULL << ((lid_info_header->packed & 0b00000111) + 1)) * (lid_info_header->packed >> 7) ) ); // перескок через локальную цветовую таблицу, если она есть
            // теперь last_index стоит на данных, а точнее на счётчик первого блока данных
            while(true) // читаем данные итерациями : в начале каждого блока данных стоит счётчик u8i с размером блока; если размер = 0, значит это последний блок
            {
                if ( last_index >= file_size ) return 0; // капитуляция, если не осталось места для счётчика блока //// по идее тут можно попытаться сохранить усечённый файл, но пока не будем с этим заморачиваться
                block_size = buffer[last_index];
                ++last_index; // передвинули last_index на данные, либо на следующий separator
                if ( block_size == 0 ) break; // данные завершились
                last_index += block_size; // если не завершились, то передвинулись на следующий счётчик блока
            }
            break;
        }
        case 0x21: // Control extensions
        {
            if ( last_index + sizeof(ControlExtension) > file_size ) return 0;
            ce_info_header = (ControlExtension*)(&buffer[last_index]);
            last_index += sizeof(ControlExtension);
            switch(ce_info_header->label) // тут будут перескоки на начало данных label'а, либо уже на следующий separator, если extension = 0xF9
            {
            case 0xF9: // graphics control extension
                last_index += 6; //ok // перескочили на следующий separator
                break;
            case 0xFE: // commentary extension
                last_index += 0; // ok
                break;
            case 0xFF: // app extension
                last_index += 12; // ok
                break;
            case 0x01: // plain text extension
                last_index += 13; //ok
                break;
            case 0xCE: // gifsicle frame's name
                last_index += 0;
            default:
                // неизвестный label, капитуляция //// по идее тут можно попытаться сохранить усечённый файл, но пока не будем с этим заморачиваться
                return 0;
            }
            if ( !EXT_LABELS_TO_PROCESS.contains(ce_info_header->label) ) break;
            while(true)
            {
                if ( last_index >= file_size ) return 0; // капитуляция, если не осталось ни одного байта для тела label'а
                block_size = buffer[last_index];
                ++last_index; // передвинули last_index на данные, либо на следующий separator
                if ( block_size == 0 ) break; // данные завершились
                last_index += block_size; // если не завершились, то передвинулись на следующий счётчик блока
            }
            break;
        }
        case 0x3B: // trailer (завершитель файла, самый последний байт любого правильного GIF'а)
            // тут надо выйти из while, потому что достигли конца ресурса, но сразу из case не получится, поэтому ниже есть if
            break;
        default:  // неизвестный separator
            return 0;
        }
        if ( buffer[last_index] == 0x3B )
        {
            ++last_index;
           break; // достигли конечного дескриптора (trailer); выход из while
        }
    }
    u64i resource_size = last_index - base_index;
    QString info = QString("%1x%2").arg(QString::number(info_header->width),
                                        QString::number(info_header->height));
    emit e->txResourceFound("gif", e->file.fileName(), base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_jpg RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct JFIF_Header
    {
        u16i soi;
        u16i app0_marker;
        u16i len;
        u32i identifier_4b;
        u8i  identifier_1b;
        u16i version;
        u8i  units;
        u16i xdens, ydens;
        u8i  xthumb, ythumb;
    };
#pragma pack(pop)
    static u32i jpg_id {fformats["jpg"].index};
    static constexpr u64i min_room_need = sizeof(JFIF_Header);
    static const QSet <u16i> VALID_VERSIONS  { 0x0100, 0x0101, 0x0102 };
    static const QSet <u8i> VALID_UNITS { 0x00, 0x01, 0x02 };
    if ( ( !e->selected_formats[jpg_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    JFIF_Header *info_header = (JFIF_Header*)(&buffer[base_index]);
    if ( be2le(info_header->len) != 16 ) return 0;
    if ( info_header->identifier_4b != 0x4649464A ) return 0;
    if ( info_header->identifier_1b != 0 ) return 0;
    if ( !VALID_VERSIONS.contains(be2le(info_header->version)) ) return 0;
    if ( !VALID_UNITS.contains(info_header->units) ) return 0;
    u64i last_index = base_index + sizeof(JFIF_Header) + 3 * (info_header->xthumb * info_header->ythumb);
    if ( last_index >= file_size ) return 0; // капитуляция, если сразу за заголовком (или thumbnail'ом) файл закончился
    while(true) // ищем SOS (start of scan) \0xFF\0xDA
    {
        if (last_index + 2 > file_size) return 0; // не нашли SOS
        if ( *((u16i*)(&buffer[last_index])) == 0xDAFF ) break; // нашли SOS
        ++last_index;
    }
    last_index += 2; // на размер SOS-идентификатора
    while(true) // ищем EOI (end of image) \0xFF\0xD9
    {
        if (last_index + 2 > file_size) return 0; // не нашли SOS
        if ( *((u16i*)(&buffer[last_index])) == 0xD9FF ) break; // нашли SOS
        ++last_index;
    }
    last_index += 2; // на размер EOI-идентификатора
    u64i resource_size = last_index - base_index;
    emit e->txResourceFound("jpg", e->file.fileName(), base_index, resource_size, "");
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_mod_mk RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct SampleDescriptor
    {
        u8i  name[22];
        u16i len;
        u8i  finetune;
        u8i  volume;
        u16i repeat_offset;
        u16i repeat_len;
    };
    struct MOD_31_Header // header with 31 samples info
    {
        u8i  song_name[20];
        SampleDescriptor sample_descriptors[31];
        u8i  patterns_number;
        u8i  end_jump_position;
        u8i  pattern_table[128];
    };
#pragma pack(pop)
    static u32i mod_mk_id {fformats["mod_m.k."].index};
    if ( ( !e->selected_formats[mod_mk_id] ) ) return 0;
    if ( e->scanbuf_offset < sizeof(MOD_31_Header) ) return 0;
    uchar *buffer = e->mmf_scanbuf;
    u32i signature = *((u32i*)(&buffer[e->scanbuf_offset]));
    u64i base_index = e->scanbuf_offset - sizeof(MOD_31_Header);
    MOD_31_Header *info_header = (MOD_31_Header*)(&buffer[base_index]);
    u64i samples_block_size = 0;
    for (int sample_id = 0; sample_id < 31; ++sample_id) // калькуляция размера блока сэмплов
    {
        samples_block_size += be2le(info_header->sample_descriptors[sample_id].len);
    }
    samples_block_size *= 2; // т.к. размер указывается в словах word (u16i)
    u8i most_pattern_number = 0;
    for (int pattern_id = 0; pattern_id < 128; ++pattern_id) // калькуляция количества уникальных паттернов на основе их номеров
    {
        if ( info_header->pattern_table[pattern_id] > most_pattern_number ) most_pattern_number = info_header->pattern_table[pattern_id];
    }
    // нашли самый большой номер -> это общее количество паттернов минус 1
    most_pattern_number += 1; // поэтому +1
    u64i steps_in_pattern;
    u64i channels;
    u64i one_note_size;
    switch(signature)
    {
    case 0x2E4B2E4D: // "M.K." - 64 steps, 4 channels, one note is 4 bytes
        steps_in_pattern = 64;
        channels = 4;
        one_note_size = 4;
        break;
    }
    u64i patterns_block_size = (steps_in_pattern * channels * one_note_size) * most_pattern_number;
    u64i last_index = base_index + sizeof(MOD_31_Header) + 4 /*signature size*/ + patterns_block_size + samples_block_size;
    if ( last_index > e->file_size) return 0; // капитуляция при неверном размере; обрезанные не сохраняем
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 20; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [19]
    {
        if ( info_header->song_name[song_name_len] == 0 ) break;
    }
    QString info = QString("%1-ch, song name: '%2'").arg(   QString::number(channels),
                                                            QString(QByteArray((char*)(info_header->song_name), song_name_len))
                                                        );
    emit e->txResourceFound("mod", e->file.fileName(), base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_xm RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct XM_Header
    {
        /// размер этих полей 60 байт
        u64i sign1; // "Extended"
        u64i sign2; // " Module:"
        u8i  sign3; // 0x20 "space"
        u8i  module_name[20];
        u8i  ox1a; // 0x1A
        u8i  tracker_name[20];
        u16i version; // 0x0104
        ///
        u32i header_size;
        u16i song_len;
        u16i song_restart_pos;
        u16i channels_number;
        u16i patterns_number;
        u16i instruments_number;
        u16i flags;
        u16i default_tempo;
        u16i default_bpm;
        u8i  pattern_order_table[256];
    };
    struct PatternHeader
    {
        u32i header_size;
        u8i  packing_type;
        u16i rows_number;
        u16i packed_pattern_data_size;
    };
    struct InstrHeader
    {
        u32i header_size; // по сути указывает инкрементальный jump отсюда на начало заголовков сэммлов
        u8i  instrument_name[22];
        u8i  instrument_type;
        u16i samples_number;
    };
    struct ExtInstrHeader
    {
        u32i sample_header_size;
        u8i  sample_nums_for_notes[96];
        u8i  volume_envelope[48];
        u8i  panning_envelope[48];
        u8i  volume_points_num, panning_points_num;
        u8i  vol_sustain_point, vol_loop_start_point, vol_loop_end_point;
        u8i  pan_sustain_point, pan_loop_start_point, pan_loop_end_point;
        u8i  vol_type, pan_type;
        u8i  vibrato_type, vibrato_sweep, vibrato_depth, vibrato_rate;
        u16i vol_fadeout;
        u16i reserved;
    };
    struct SampleHeader
    {
        u32i sample_len;
        u32i loop_start;
        u32i loop_len;
        u8i  volume;
        u8i  finetune;
        u8i  type;
        u8i  panning;
        u8i  note_number;
        u8i  reserved;
        u8i  name[22];
    };
#pragma pack(pop)
    static u32i xm_id {fformats["xm"].index};
    static constexpr u64i min_room_need = sizeof(XM_Header);
    if ( ( !e->selected_formats[xm_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    uchar *buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    XM_Header *info_header = (XM_Header*)(&buffer[base_index]);
    if ( info_header->sign1 != 0x6465646E65747845 ) return 0;
    if ( info_header->sign2 != 0x3A656C75646F4D20 ) return 0;
    if ( info_header->sign3 != 0x20 ) return 0;
    if ( info_header->ox1a != 0x1A ) return 0;
    if ( info_header->version != 0x0104 ) return 0;
    if ( info_header->header_size < 276 ) return 0;
    if ( info_header->channels_number == 0 ) return 0;
    if ( info_header->channels_number > 64 ) return 0;
    if ( info_header->patterns_number == 0 ) return 0;
    if ( info_header->patterns_number > 256 ) return 0;
    if ( info_header->instruments_number > 128 ) return 0;
    //qInfo() << "instruments number:" << info_header->instruments_number;
    u64i last_index = base_index + 60 + info_header->header_size; // переставили last_index на заголовок самого первого паттерна
    PatternHeader *pattern_header;
    for (u16i i = 0; i < info_header->patterns_number; ++i) // идём по паттернам
    {
        if ( last_index + sizeof(PatternHeader) > file_size ) return 0; // недостаточно места для анализа заголовка паттерна
        pattern_header = (PatternHeader*)(&buffer[last_index]);
        if ( pattern_header->rows_number > 256 ) return 0; // чё-то не то, надо капитулировать
        last_index += (pattern_header->header_size + pattern_header->packed_pattern_data_size); // переставили last_index на следующий pattern header или, если паттерны закончились, на заголовки инструментов
    }
    // qInfo() << "instrument headers start at:" << last_index;
    InstrHeader *instr_header;
    ExtInstrHeader *ext_instr_header;
    SampleHeader *sample_header;
    u64i sample_block_size;
    // здесь last_index уже стоит на самом первом заголовке InstHeader
    for (u16i i = 0; i < info_header->instruments_number; ++i) // идём по инструментам
    {
        sample_block_size = 0; // обнуление размера блока сэмплов для каждого инструмента
        if ( last_index + sizeof(InstrHeader) > file_size ) return 0; // недостаточно места для анализа заголовка инструмента
        instr_header = (InstrHeader*)(&buffer[last_index]);
        //qInfo() << ":: instrument id:" << i << "at offset:" << QString::number(last_index, 16) << " has" << instr_header->samples_number << "samples";
        if ( instr_header->samples_number > 0 ) // значит дальше есть ExtInstrHeader и заголовки сэмплов, за которыми идут сами сэмплы
        {
            ext_instr_header = (ExtInstrHeader*)(&buffer[last_index + sizeof(InstrHeader)]); // из расширенного заголовка нам нужно поле sample_header_size для дальнеших рассчётов
            if ( last_index + sizeof(InstrHeader) + sizeof(ExtInstrHeader) > file_size ) return 0; // недостаточно места для анализа расширенного заголовка
            last_index += instr_header->header_size; // прыгаем на заголовок самого первого сэмпла
            for (u16i s = 0; s < instr_header->samples_number; ++s) // идём по заголовкам сэмплов
            {
                if ( last_index + ext_instr_header->sample_header_size > file_size ) return 0; // не хватает места на заголовок сэмпла
                sample_header = (SampleHeader*)(&buffer[last_index]);
                sample_block_size += sample_header->sample_len;
                //qInfo() << " ---> sample id:" << s << "; size:" << sample_header->sample_len;
                last_index += ext_instr_header->sample_header_size;
            }
            // после выхода из for индекс last_index стоит на блоке сэмплов
            //qInfo() << "sample_block_size:" << sample_block_size;
            if ( last_index + sample_block_size > file_size ) return 0; // не хватает места на сэмплы

            last_index += sample_block_size; // индексируем last_index, переставляя его на заголовок следующего инструмента, либо в конец файла, если больше нет инструментов
        }
        else // если нет сэмплов в инструменте, то просто прыгаем на следующий инструмент
        {
            last_index += instr_header->header_size;
        }
    }
    // если дошли сюда, значит достигли конца ресурса
    if ( last_index > file_size ) return 0; // последняя проверка
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 20; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [19]
    {
        if ( info_header->module_name[song_name_len] == 0 ) break;
    }
    QString info = QString("%1-ch, song name: '%2'").arg(   QString::number(info_header->channels_number),
                                                            QString(QByteArray((char*)(info_header->module_name), song_name_len))
                                                         );
    emit e->txResourceFound("xm", e->file.fileName(), base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}
