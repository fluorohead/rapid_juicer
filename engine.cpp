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
            analyzed_dword = *(u32i*)(mmf_scanbuf + scanbuf_offset);
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
                    recognize_png(this);
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
{
    if ( e->enough_room_to_continue(10) )
    {
        ++(e->hits);
    }

    return 0;
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
        u32i data_len;
        u32i type;
    };
    struct IHDRData
    {
        u32i width;
        u32i height;
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
    static const u32i format_index = fformats["png"].index;
    static constexpr u64i min_room_need = sizeof(FileHeader) + sizeof(ChunkHeader) + sizeof(IHDRData) + sizeof(CRC);
    static const QSet <u8i>  VALID_BIT_DEPTH  {1, 2, 4, 8, 16};
    static const QSet <u8i>  VALID_COLOR_TYPE {0, 2, 3, 4, 6};
    static const QSet <u8i>  VALID_INTERLACE  {0, 1};
    static const QSet <u32i> VALID_CHUNK_TYPE {    0x54414449 /*IDAT*/, 0x4D524863 /*cHRM*/, 0x414D4167 /*gAMA*/, 0x54494273 /*sBIT*/, 0x45544C50 /*PLTE*/, 0x44474B62 /*bKGD*/,
                                                   0x54534968 /*hIST*/, 0x534E5274 /*tRNS*/, 0x7346466F /*oFFs*/, 0x73594870 /*pHYs*/, 0x4C414373 /*sCAL*/, 0x54414449 /*IDAT*/, 0x454D4974 /*tIME*/,
                                                   0x74584574 /*tEXt*/, 0x7458547A /*zTXt*/, 0x63415266 /*fRAc*/, 0x67464967 /*gIFg*/, 0x74464967 /*gIFt*/, 0x78464967 /*gIFx*/, 0x444E4549 /*IEND*/,
                                                   0x42475273 /*sRGB*/, 0x52455473 /*sTER*/, 0x4C414370 /*pCAL*/, 0x47495364 /*dSIG*/, 0x50434369 /*iCCP*/, 0x50434963 /*iICP*/, 0x7643446D /*mDCv*/,
                                                   0x694C4C63 /*cLLi*/, 0x74585469 /*iTXt*/, 0x744C5073 /*sPLt*/, 0x66495865 /*eXIf*/, 0x52444849 /*iHDR*/};

    if ( ( !e->selected_formats[format_index] ) ) return 0;
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
                                                color_type);
    emit e->txResourceFound("png", e->file.fileName(), base_index, resource_size, info);
    return last_index;
}
