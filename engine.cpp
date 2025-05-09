  //   lbl|sign | AHAL | BHBL
  //   ---|-----|------|------
  //   0  |ico  : 0000 : 0100
  //   0  |tga  : 0000 : 0200
  //   0  |cur  : 0000 : 0200 <- crossing with tga
  //   0  |tga  : 0000 : 0A00
  //   0  |ttf  : 0001 : 0000
  //   31 |wmf  : 0100 : 0900
  //   32 |wmf  : 0200 : 0900
  //   1  |pcx  : 0A05
  //   2  |fli  : 11AF
  //   3  |flc  : 12AF
  //   30 |emf  : 2045 : 4D46
  //   34 |stm  : 2153 : 6372
  //   25 |au   : 2E73 : 6E64
  //   28 |asf  : 3026 : b275
  //   4  |bik  : 4249
  //   4  |bmp  : 424D
  //   4  |bmod : 424D : 4F44 <- crossing with bmp
  //   24 |ch   : 4348
  //   24 |voc  : 4372 : 6561
  //   6  |dbm0 : 4442 : 4D30
  //   6  |dds  : 4444 : 5320
  //   6  |flx  : 44AF
  //   7  |xm   : 4578 : 7465
  //   8  |iff  : 464F : 524D
  //   8  |flt4 : 464C : 5434
  //   8  |flt8 : 464C : 5438
  //   9  |gif  : 4749 : 4638
  //   10 |id3v2: 4944 : 33
  //   10 |tifi : 4949 : 2A00
  //   10 |it   : 494D : 504D
  //   23 |669  : 4A4E
  //   12 |bk2  : 4B42
  //   13 |mk   : 4D2E : 4B2E
  //   13 |tifm : 4D4D : 002A
  //   13 |mmd0 : 4D4D : 4430
  //   13 |mmd1 : 4D4D : 4431
  //   13 |mmd2 : 4D4D : 4432
  //   13 |mmd3 : 4D4D : 4433
  //   13 |mid  : 4D54 : 6864
  //   27 |otc  : 4F54 : 544F
  //   27 |ogg  : 4F67 : 6753
  //   16 |rif  : 5249 : 4646
  //   17 |s3m  : 5343 : 524D
  //   17 |sm2  : 534D : 4B32
  //   17 |sm4  : 534D : 4B34
  //   22 |669  : 6966
  //   26 |mdat : 6D64 : 6174
  //   26 |moov : 6D6F : 6F76
  //   33 |ttc  : 7474 : 6366
  //   20 |png  : 8950 : 4E47
  //   29 |wmf  : D7CD : C69A
  //   21 |jpg  : FFD8 : FFE0
  //   21 |mp3  : FFFA
  //   21 |mp3  : FFFB

#include "engine.h"
#include "walker.h"
#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <QByteArray>
#include <QMimeDatabase>

// #define ASMJIT_STATIC
// #define ASMJIT_NO_AARCH64
// #define ASMJIT_NO_FOREIGN

#define ASM_LABELS_MAX 40

extern const QMap <u32i, QString> wave_codecs;
extern QMap <QString, FileFormat> fformats;

QMap <QString, Signature> signatures // в QMap значения автоматически упорядочены по ключам
{
    // ключ
    // |
    // V
    { "bmp",        { 0x0000000000004D42, 2, Engine::recognize_bmp      } }, // "BM"
    { "pcx_05",     { 0x000000000000050A, 2, Engine::recognize_pcx      } }, // "\x0A\x05"
    { "png",        { 0x00000000474E5089, 4, Engine::recognize_png      } }, // "\x89PNG"
    { "riff",       { 0x0000000046464952, 4, Engine::recognize_riff     } }, // "RIFF"
    { "iff",        { 0x000000004D524F46, 4, Engine::recognize_iff      } }, // "FORM"
    { "gif",        { 0x0000000038464947, 4, Engine::recognize_gif      } }, // "GIF8"
    { "tiff_ii",    { 0x00000000002A4949, 4, Engine::recognize_tif_ii   } }, // "II*\x00"
    { "tiff_mm",    { 0x000000002A004D4D, 4, Engine::recognize_tif_mm   } }, // "MM\x00*"
    { "tga_tp2",    { 0x0000000000020000, 4, Engine::recognize_tga      } }, // "\x00\x00\x02\x00"
    { "tga_tp10",   { 0x00000000000A0000, 4, Engine::recognize_tga      } }, // "\x00\x00\x0A\x00"
    { "jpg",        { 0x00000000E0FFD8FF, 4, Engine::recognize_jpg      } }, // "\xFF\xD8\xFF\xE0"
    { "mid",        { 0x000000006468544D, 4, Engine::recognize_mid      } }, // "MThd"
    { "mod_m.k.",   { 0x000000002E4B2E4D, 4, Engine::recognize_mod      } }, // "M.K." SoundTracker 2.2 by Unknown/D.O.C. [Michael Kleps] and ProTracker/NoiseTracker/etc...
    { "xm",         { 0x0000000065747845, 4, Engine::recognize_xm       } }, // "Exte"
    { "stm",        { 0x0000000072635321, 4, Engine::recognize_stm      } }, // "!Scream!"
    { "stm_bmod",   { 0x00000000444F4D42, 4, Engine::recognize_stm      } }, // "BMOD2STM"
    { "s3m",        { 0x000000004D524353, 4, Engine::recognize_s3m      } }, // "SCRM"
    { "it",         { 0x000000004D504D49, 4, Engine::recognize_it       } }, // "IMPM"
    { "bink1",      { 0x0000000000004942, 2, Engine::recognize_bink     } }, // "BI"
    { "bink2",      { 0x000000000000424B, 2, Engine::recognize_bink     } }, // "KB"
    { "smk2",       { 0x00000000324B4D53, 4, Engine::recognize_smk      } }, // "SMK2"
    { "smk4",       { 0x00000000344B4D53, 4, Engine::recognize_smk      } }, // "SMK4"
    { "fli_af11",   { 0x000000000000AF11, 2, Engine::recognize_flc      } }, // "\x11\xAF"
    { "flc_af12",   { 0x000000000000AF12, 2, Engine::recognize_flc      } }, // "\x12\xAF"
    { "flx_af44",   { 0x000000000000AF44, 2, Engine::recognize_flc      } }, // "\x44\xAF" Dave's Targa Animator (DTA) software
    { "669_if",     { 0x0000000000006669, 2, Engine::recognize_669      } }, // "if"
    { "669_jn",     { 0x0000000000004E4A, 2, Engine::recognize_669      } }, // "JN"
    { "mod_ch",     { 0x0000000000004843, 2, Engine::recognize_mod      } }, // "CH"
    { "au",         { 0x00000000646E732E, 4, Engine::recognize_au       } }, // ".snd"
    { "qt_mdat",    { 0x000000007461646D, 4, Engine::recognize_mov_qt   } }, // "mdat"
    { "qt_moov",    { 0x00000000766F6F6D, 4, Engine::recognize_mov_qt   } }, // "moov"
    { "qt_moov",    { 0x00000000766F6F6D, 4, Engine::recognize_mov_qt   } }, // "moov"
    { "ico_win",    { 0x0000000000010000, 4, Engine::recognize_ico_cur  } }, // "\x00\x00\x01\x00"
    { "cur_win",    { 0x0000000000020000, 4, Engine::recognize_ico_cur  } }, // "\x00\x00\x02\x00"
    { "mp3_fffa",   { 0x000000000000FAFF, 2, Engine::recognize_mp3      } }, // "\xFF\xFA"
    { "mp3_fffb",   { 0x000000000000FAFF, 2, Engine::recognize_mp3      } }, // "\xFF\xFB"
    { "mp3_id3v2",  { 0x0000000000334449, 3, Engine::recognize_mp3      } }, // "\x49\x44\x33"
    { "ogg",        { 0x000000005367674F, 4, Engine::recognize_ogg      } }, // "OggS"
    { "mmd0",       { 0x0000000030444D4D, 4, Engine::recognize_med      } }, // "MMD0" OctaMED
    { "mmd1",       { 0x0000000031444D4D, 4, Engine::recognize_med      } }, // "MMD1" OctaMED
    { "mmd2",       { 0x0000000032444D4D, 4, Engine::recognize_med      } }, // "MMD2" OctaMED
    { "mmd3",       { 0x0000000033444D4D, 4, Engine::recognize_med      } }, // "MMD3" OctaMED
    { "dbm0",       { 0x00000000304D4244, 4, Engine::recognize_dbm0     } }, // "DBM0" DigiBooster Pro
    { "ttf",        { 0x0000000000000100, 4, Engine::recognize_ttf      } }, // "\x00\x01\x00\x00"
    { "ttc",        { 0x0000000066637474, 4, Engine::recognize_ttc      } }, // "ttcf"
    { "otf",        { 0x000000004F54544F, 4, Engine::recognize_ttf      } }, // "OTTO"
    { "asf",        { 0x0000000075B22630, 4, Engine::recognize_asf      } }, // "\x30\x26\xB2\x75"
    { "wmf_plc",    { 0x000000009AC6CDD7, 4, Engine::recognize_wmf      } }, // "\xD7\xCD\xC6\x9A"
    { "wmf_mem",    { 0x0000000000090001, 4, Engine::recognize_wmf      } }, // "\x01\x00\x09\x00"
    { "wmf_disk",   { 0x0000000000090002, 4, Engine::recognize_wmf      } }, // "\x02\x00\x09\x00"
    { "emf",        { 0x00000000464D4520, 4, Engine::recognize_emf      } }, // "\x20\x45\x4D\x46"
    { "dds",        { 0x0000000020534444, 4, Engine::recognize_dds      } }, // "DDS "
    { "mod_flt4",   { 0x0000000034544C46, 4, Engine::recognize_mod      } }, // "FLT4" - Startrekker
    { "mod_flt8",   { 0x0000000038544C46, 4, Engine::recognize_mod      } }, // "FLT8" - Startrekker
};

// u16i be2le(u16i be)
// {
//     union {
//         u16i as_le;
//         u8i  bytes[2];
//     };
//     bytes[0] = be >> 8;
//     bytes[1] = be;
//     return as_le;
// }

// u32i be2le(u32i be)
// {
//     union {
//         u32i as_le;
//         u8i  bytes[4];
//     };
//     bytes[0] = be >> 24;
//     bytes[1] = be >> 16;
//     bytes[2] = be >> 8 ;
//     bytes[3] = be;
//     return as_le;
// }

// u32i be2le(u64i be)
// {
//     union {
//         u64i as_le;
//         u8i  bytes[8];
//     };
//     bytes[0] = be >> 56;
//     bytes[1] = be >> 48;
//     bytes[2] = be >> 40 ;
//     bytes[3] = be >> 32;
//     bytes[4] = be >> 24;
//     bytes[5] = be >> 16;
//     bytes[6] = be >> 8;
//     bytes[7] = be;
//     return as_le;
// }

u64i be2le(u64i value)
{
    __asm__ volatile (
    R"(
    mov %[qword_value], %%rax
    bswap %%rax
    mov %%rax, %[qword_value]
    )"
    :
    : [qword_value] "m" (value)
    : "rax", "memory"
    );
    return value;
}

u32i be2le(u32i value)
{
    __asm__ volatile (
    R"(
    mov %[dword_value], %%eax
    bswap %%eax
    mov %%eax, %[dword_value]
    )"
    :
    : [dword_value] "m" (value)
    : "eax", "memory"
    );
    return value;
}

u16i be2le(u16i value)
{
    __asm__ volatile (
    R"(
    mov %[word_value], %%ax
    xchg %%ah, %%al
    mov %%ax, %[word_value]
    )"
    :
    : [word_value] "m" (value)
    : "ax", "memory"
    );
    return value;
}

Engine::Engine(WalkerThread *walker_parent)
    : my_walker_parent(walker_parent)
{
    command = &my_walker_parent->command;
    control_mutex = my_walker_parent->walker_control_mutex;
    scrupulous = my_walker_parent->walker_config.scrupulous;
    selected_formats = my_walker_parent->selected_formats_fast;

    generate_comparation_func();
}

Engine::~Engine()
{
    //qInfo() << "Engine destructor called in thread id" << QThread::currentThreadId();
    aj_runtime.release(comparation_func);
}

void Engine::generate_comparation_func()
{
    CodeHolder aj_code;
    StringLogger aj_logger;
    aj_code.init(aj_runtime.environment(), aj_runtime.cpuFeatures());
    aj_code.setLogger(&aj_logger);
    x86::Assembler aj_asm(&aj_code);

    Label aj_prolog_label = aj_asm.newLabel();
    Label aj_loop_start_label = aj_asm.newLabel();
    Label aj_signat_labels[ASM_LABELS_MAX]; // 40 - с запасом
    Label aj_sub_labels[ASM_LABELS_MAX]; // 40 - с запасом
    Label aj_scrup_mode_check_label = aj_asm.newLabel();
    Label aj_scrup_mode_off_label = aj_asm.newLabel();
    Label aj_loop_check_label = aj_asm.newLabel();
    Label aj_epilog_label = aj_asm.newLabel();

    for (int s_idx = 0; s_idx < ASM_LABELS_MAX; ++s_idx) // готовим лейблы
    {
        aj_signat_labels[s_idx] = aj_asm.newLabel();
        aj_sub_labels[s_idx] = aj_asm.newLabel();
    }

    //x86::Mem rbp_plus_16 = x86::ptr(x86::rbp, 16); // указатель на наш "home rcx"
    x86::Mem rdi_edx_mul8 = x86::ptr(x86::rdi, x86::edx, 3); // shift = 3, равноценно *8 : [rdi + edx*8]

    //  входные параметры
    //  rcx - start_offset
    //  rdx - last_offset

    // используемые регистры :
    // rax(eax) - временно используется для первой половины analyzed_dword'а
    // rbx(ebx) - временно используется для второй половины analyzed_dword'а
    // rcx - временно используется для передачи первого параметра в recognizer-функцию
    // rdx(edx) - временно используется для индексации вектора переходов
    // rdi - постоянно исп-ся : абсолютный указатель на начало вектора переходов
    // rsi - постоянно исп-ся : абсолютный указатель на текущий analyzed_dword
    // r12 - постоянно исп-ся : обсолютный указатель (не включаемый) на конечный analyzed_dword (конечный в рамках текущего окна гранулярности)
    // r13 - постоянно исп-ся : абсолютный указатель на переменную this->scanbuf_offset
    // r14 - постоянно исп-ся : относительный счётчик внутри буфера сканирования (смещение от начала буфера)
    // r15 - постоянно исп-ся : флаг scrupulous-режима

// ; prolog
    aj_asm.bind(aj_prolog_label);
    aj_asm.push(x86::rbp);
    aj_asm.mov(x86::rbp, x86::rsp);
    aj_asm.push(x86::rdi);
    aj_asm.push(x86::rsi);
    aj_asm.push(x86::r12);
    aj_asm.push(x86::r13);
    aj_asm.push(x86::r14);
    aj_asm.push(x86::r15);
    aj_asm.push(x86::rbx);
    aj_asm.sub(x86::rsp, imm(256*8)); // резервируем под вектор переходов
    aj_asm.mov(x86::rdi, x86::rsp); // фиксируем rdi на начале вектора
    aj_asm.sub(x86::rsp, 40); // сразу формируем home-регион для callee-функций : 40 вместо 32, чтобы выровнять по 16-байт (там уже лежит 8 байт возврата, поэтому 32+8 будет плохо, а 40+8 в самый раз)

    aj_asm.mov(x86::rsi, imm(&this->mmf_scanbuf));
    aj_asm.mov(x86::rsi, x86::qword_ptr(x86::rsi)); // теперь адрес буфера сканирования в rsi
    aj_asm.mov(x86::r12, x86::rsi); // и тот же адрес в r12
    aj_asm.add(x86::rsi, x86::rcx); // теперь в rsi абсолюный начальный адрес с учётом start_offset;
    aj_asm.add(x86::r12, x86::rdx); // теперь в r12 абсолютный конечный адрес, не включаемый; rdx далее не нужен (возможно будем использовать в будущем)
    aj_asm.mov(x86::r14, x86::rcx); // теперь в r14 относительный счётчик (смещение в mmf); далее rcx будем использовать только для передачи параметров в callee
    aj_asm.mov(x86::r13, imm(&this->scanbuf_offset)); // теперь в [r13] адрес переменной this->scanbuf_offset
    aj_asm.mov(x86::r15, scrupulous ? 1 : 0); // в r15 постоянно храним флаг scrupulous-режима

    // заполняем вектор адресом по-умолчанию (aj_loop_check_label);
    // rdi стоит на начале вектора;
    aj_asm.lea(x86::rax, x86::ptr(aj_loop_check_label)); // грузим абсолютный адрес лейбла в rax
    aj_asm.mov(x86::rcx, imm(256)); // количество повторений
    aj_asm.cld(); // сброс Direction Flag = увеличение rdi
    aj_asm.rep();
    aj_asm.stosq();
    aj_asm.sub(x86::rdi, imm(256*8)); // возвращаем rdi на начало вектора

    // выборочно заполняем вектор адресами предобработчиков
    if ( selected_formats[fformats["tga_tc"].index] or selected_formats[fformats["ico_win"].index] or selected_formats[fformats["cur_win"].index] or selected_formats[fformats["ttf"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[0])); // tag_tc, ico_win, cur_win, ttf
        aj_asm.mov(x86::ptr(x86::rdi, 0x00 * 8), x86::rax);
    }

    if ( selected_formats[fformats["wmf"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[31])); // wmf non-placeable
        aj_asm.mov(x86::ptr(x86::rdi, 0x01 * 8), x86::rax);
    }

    if ( selected_formats[fformats["wmf"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[32])); // wmf non-placeable
        aj_asm.mov(x86::ptr(x86::rdi, 0x02 * 8), x86::rax);
    }

    if ( selected_formats[fformats["pcx"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[1])); // pcx
        aj_asm.mov(x86::ptr(x86::rdi, 0x0A * 8), x86::rax);
    }

    if ( selected_formats[fformats["flc"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[2])); // fli
        aj_asm.mov(x86::ptr(x86::rdi, 0x11 * 8), x86::rax);

        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[3])); // flc
        aj_asm.mov(x86::ptr(x86::rdi, 0x12 * 8), x86::rax);
    }

    if ( selected_formats[fformats["emf"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[30])); // emf
        aj_asm.mov(x86::ptr(x86::rdi, 0x20 * 8), x86::rax);
    }

    if ( selected_formats[fformats["stm"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[34])); // stm
        aj_asm.mov(x86::ptr(x86::rdi, 0x21 * 8), x86::rax);
    }

    if ( selected_formats[fformats["au"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[25])); // au
        aj_asm.mov(x86::ptr(x86::rdi, 0x2E * 8), x86::rax);
    }

    if ( selected_formats[fformats["wma"].index] or selected_formats[fformats["wmv"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[28])); // asf
        aj_asm.mov(x86::ptr(x86::rdi, 0x30 * 8), x86::rax);
    }

    if ( selected_formats[fformats["bik"].index] or selected_formats[fformats["bmp"].index] or selected_formats[fformats["stm"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[4])); // bik, bmp, stm_bmod
        aj_asm.mov(x86::ptr(x86::rdi, 0x42 * 8), x86::rax);
    }

    if ( selected_formats[fformats["mod"].index] or selected_formats[fformats["voc"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[24])); // mod 'CH', voc
        aj_asm.mov(x86::ptr(x86::rdi, 0x43 * 8), x86::rax);
    }

    if ( selected_formats[fformats["flc"].index] or selected_formats[fformats["dbm0"].index] or selected_formats[fformats["dds"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[6])); // flx, dbm0, dds
        aj_asm.mov(x86::ptr(x86::rdi, 0x44 * 8), x86::rax);
    }

    if ( selected_formats[fformats["xm"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[7])); // xm
        aj_asm.mov(x86::ptr(x86::rdi, 0x45 * 8), x86::rax);
    }

    if ( selected_formats[fformats["xmi"].index] or selected_formats[fformats["lbm"].index] or selected_formats[fformats["aif"].index] or selected_formats[fformats["mod"].index])
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[8])); // iff, flt4, flt8
        aj_asm.mov(x86::ptr(x86::rdi, 0x46 * 8), x86::rax);
    }

    if ( selected_formats[fformats["gif"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[9])); // gif
        aj_asm.mov(x86::ptr(x86::rdi, 0x47 * 8), x86::rax);
    }

    if ( selected_formats[fformats["tif"].index] or selected_formats[fformats["it"].index] or selected_formats[fformats["mp3"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[10])); // tif_ii, it, mp3_id3v2
        aj_asm.mov(x86::ptr(x86::rdi, 0x49 * 8), x86::rax);
    }

    if ( selected_formats[fformats["669"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[23])); // 669_jn
        aj_asm.mov(x86::ptr(x86::rdi, 0x4A * 8), x86::rax);
    }

    if ( selected_formats[fformats["bk2"].index]  )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[12])); // bk2
        aj_asm.mov(x86::ptr(x86::rdi, 0x4B * 8), x86::rax);
    }

    if ( selected_formats[fformats["mod"].index] or selected_formats[fformats["tif"].index] or selected_formats[fformats["mid"].index] or selected_formats[fformats["med"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[13])); // mod 'M.K.', tif_mm, mid, mmd0, mmd1, mmd2, mmd3
        aj_asm.mov(x86::ptr(x86::rdi, 0x4D * 8), x86::rax);
    }

    if ( selected_formats[fformats["ogg"].index] or selected_formats[fformats["ogm"].index] or selected_formats[fformats["ttf"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[27])); // ogg, otc(ttf)
        aj_asm.mov(x86::ptr(x86::rdi, 0x4F * 8), x86::rax);
    }

    if ( selected_formats[fformats["avi"].index] or selected_formats[fformats["wav"].index] or selected_formats[fformats["rmi"].index] or selected_formats[fformats["ani_riff"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[16])); // riff
        aj_asm.mov(x86::ptr(x86::rdi, 0x52 * 8), x86::rax);
    }

    if ( selected_formats[fformats["s3m"].index] or selected_formats[fformats["smk"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[17])); // s3m, sm2, sm4
        aj_asm.mov(x86::ptr(x86::rdi, 0x53 * 8), x86::rax);
    }

    if ( selected_formats[fformats["669"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[22])); // 669_if
        aj_asm.mov(x86::ptr(x86::rdi, 0x69 * 8), x86::rax);
    }

    if ( selected_formats[fformats["mov_qt"].index] or selected_formats[fformats["mp4_qt"].index])
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[26])); // mdat, moov
        aj_asm.mov(x86::ptr(x86::rdi, 0x6D * 8), x86::rax);
    }

    if ( selected_formats[fformats["ttf"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[33])); // ttc
        aj_asm.mov(x86::ptr(x86::rdi, 0x74 * 8), x86::rax);
    }

    if ( selected_formats[fformats["png"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[20])); // png
        aj_asm.mov(x86::ptr(x86::rdi, 0x89 * 8), x86::rax);
    }

    if ( selected_formats[fformats["wmf"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[29])); // wmf
        aj_asm.mov(x86::ptr(x86::rdi, 0xD7 * 8), x86::rax);
    }

    if ( selected_formats[fformats["jpg"].index] or selected_formats[fformats["mp3"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[21])); // jpg, mp3
        aj_asm.mov(x86::ptr(x86::rdi, 0xFF * 8), x86::rax);
    }

// ; loop_start
    aj_asm.bind(aj_loop_start_label);
    aj_asm.mov(x86::eax, x86::dword_ptr(x86::rsi)); // в eax лежит анализируемый dword
    aj_asm.bswap(x86::eax);
    aj_asm.mov(x86::bx, x86::ax);
    aj_asm.shr(x86::eax, 16);
    // теперь в :ah - 1-й байт, al - 2-й байт, bh - 3-й байт, bl - 4-й байт.
    aj_asm.movzx(x86::edx, x86::ah); // теперь в edx помещён индекс для вектора переходов
    aj_asm.jmp(rdi_edx_mul8);

// ; 0x00
aj_asm.bind(aj_signat_labels[0]);
    // ; ico : 0x00'00 : 0x01'00
    // ; tga : 0x00'00 : 0x02'00
    // ; cur : 0x00'00 : 0x02'00
    // ; tga : 0x00'00 : 0x0A'00
    // ; ttf : 0x00'01 : 0x00'00
    aj_asm.cmp(x86::al, 0x00);
    aj_asm.je(aj_sub_labels[20]);
    aj_asm.cmp(x86::al, 0x01);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x00'00);
    aj_asm.je(aj_sub_labels[21]);
    aj_asm.jmp(aj_loop_check_label);
aj_asm.bind(aj_sub_labels[20]);
    if ( selected_formats[fformats["ico_win"].index] )
    {
        aj_asm.cmp(x86::bx, 0x01'00);
        aj_asm.je(aj_sub_labels[14]);
    }
    if ( selected_formats[fformats["tga_tc"].index] or selected_formats[fformats["cur_win"].index] )
    {
        aj_asm.cmp(x86::bx, 0x02'00);
        aj_asm.je(aj_sub_labels[13]);
        aj_asm.cmp(x86::bx, 0x0A'00);
        aj_asm.je(aj_sub_labels[13]);
    }
    aj_asm.jmp(aj_loop_check_label);
aj_asm.bind(aj_sub_labels[13]);
    if ( selected_formats[fformats["tga_tc"].index] or selected_formats[fformats["cur_win"].index] )
    {
        // вызов recognize_tga
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_tga));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);
aj_asm.bind(aj_sub_labels[14]);
    if ( selected_formats[fformats["ico_win"].index] )
    {
        // вызов recognize_ico_cur
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_ico_cur));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);
aj_asm.bind(aj_sub_labels[21]);
    if ( selected_formats[fformats["ttf"].index] )
    {
        // вызов recognize_ttf
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_ttf));
        // scrup mode для ttf всегда отключен
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_off_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; 0x01
aj_asm.bind(aj_signat_labels[31]);
    // ; wmf : 0x01'00 :0x09'00 : mem type
    aj_asm.cmp(x86::al, 0x00);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x09'00);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_wmf
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_wmf));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x02
aj_asm.bind(aj_signat_labels[32]);
    // ; wmf : 0x02'00 :0x09'00 : disk type
    aj_asm.cmp(x86::al, 0x00);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x09'00);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_wmf
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_wmf));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x0A
aj_asm.bind(aj_signat_labels[1]);
    // ; pcx : 0x0A'05
    aj_asm.cmp(x86::al, 0x05);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_pcx
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_pcx));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x11
aj_asm.bind(aj_signat_labels[2]);
    // ; fli : 0x11'AF
    aj_asm.cmp(x86::al, 0xAF);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_flc
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_flc));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x12
aj_asm.bind(aj_signat_labels[3]);
    // ; flc : 0x12'AF
    aj_asm.cmp(x86::al, 0xAF);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_flc
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_flc));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x20
aj_asm.bind(aj_signat_labels[30]);
    // ; emf : 0x20'45 : 0x4D'46
    aj_asm.cmp(x86::al, 0x45);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x4D'46);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_emf
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_emf));
    // для emf всегда выключен scrupulous mode.
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_off_label);
    //
    aj_asm.jmp(aj_loop_check_label);


// ; 0x21
aj_asm.bind(aj_signat_labels[34]);
    // ; stm : 0x21'53 :0x63'72
    aj_asm.cmp(x86::al, 0x53);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x63'72);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_stm
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_stm));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x2E
aj_asm.bind(aj_signat_labels[25]);
    // ; au : 0x2E'73 : 0x6E'64
    aj_asm.cmp(x86::al, 0x73);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x6E'64);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_au
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_au));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x30
aj_asm.bind(aj_signat_labels[28]);
    // ; asf: 0x30'26 : 0xB2'75
    aj_asm.cmp(x86::al, 0x26);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0xB2'75);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_asf
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_asf));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x42
aj_asm.bind(aj_signat_labels[4]);
    // ; bik  : 0x42'49
    // ; bmp  : 0x42'4D
    // ; bmod : 0x42'4D : 0x4F'44
aj_asm.bind(aj_sub_labels[0]); // bik ?
    if ( selected_formats[fformats["bik"].index] )
    {
        aj_asm.cmp(x86::al, 0x49);
        aj_asm.jne(aj_sub_labels[1]);
        // вызов recognize_bink
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_bink));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[1]); // bmp ?
    if ( selected_formats[fformats["bmp"].index] or selected_formats[fformats["stm"].index] )
    {
        aj_asm.cmp(x86::al, 0x4D);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_bmp
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_bmp));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; 0x43
aj_asm.bind(aj_signat_labels[24]);
    // ;  ch : 0x43'48
    // ; voc : 0x43'72 : 0x65'61
    aj_asm.bind(aj_sub_labels[9]); // mod_ch ?
    if ( selected_formats[fformats["mod"].index] )
    {
        aj_asm.cmp(x86::al, 0x48);
        aj_asm.jne(aj_sub_labels[10]);
        // вызов recognize_mod
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_mod));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[10]); // voc ?
    if ( selected_formats[fformats["voc"].index] )
    {
        aj_asm.cmp(x86::al, 0x72);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bx, 0x65'61);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_voc
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_voc));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; 0x44
aj_asm.bind(aj_signat_labels[6]);
    // ; dbm0 : 0x44'42 : 0x4D'30
    // ;  dds : 0x44'44 : 0x53'20
    // ;  flx : 0x44'AF
aj_asm.bind(aj_sub_labels[18]); // dbm0?
    if ( selected_formats[fformats["dbm0"].index] )
    {
        aj_asm.cmp(x86::al, 0x42);
        aj_asm.jne(aj_sub_labels[23]);
        aj_asm.cmp(x86::bx, 0x4D'30);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_dbm0
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_dbm0));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[23]); // dds
    if ( selected_formats[fformats["dds"].index] )
    {
        aj_asm.cmp(x86::al, 0x44);
        aj_asm.jne(aj_sub_labels[19]);
        aj_asm.cmp(x86::bx, 0x53'20);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_dds
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_dds));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[19]); // flx?
    if ( selected_formats[fformats["flc"].index] )
    {
        aj_asm.cmp(x86::al, 0xAF);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_flc
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_flc));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
aj_asm.jmp(aj_loop_check_label);

// ; 0x45
aj_asm.bind(aj_signat_labels[7]);
    // ; xm : 0x45'78 : 0x74'65
    aj_asm.cmp(x86::al, 0x78);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x74'65);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_xm
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_xm));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x46
aj_asm.bind(aj_signat_labels[8]);
    // ; flt4 : 0x46'4C : 0x54'34
    // ; flt8 : 0x46'4C : 0x54'38
    // ;  iff : 0x46'4F : 0x52'4D
aj_asm.bind(aj_sub_labels[24]); // flt4/flt8 ?
    if ( selected_formats[fformats["mod"].index] )
    {
        aj_asm.cmp(x86::al, 0x4C);
        aj_asm.jne(aj_sub_labels[26]);
        aj_asm.cmp(x86::bh, 0x54);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bl, 0x34);
        aj_asm.je(aj_sub_labels[25]);
        aj_asm.cmp(x86::bl, 0x38);
        aj_asm.jne(aj_loop_check_label);
aj_asm.bind(aj_sub_labels[25]);
        // вызов recognize_mod
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_mod));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[26]); // iff ?
    if ( selected_formats[fformats["xmi"].index] or selected_formats[fformats["lbm"].index] or selected_formats[fformats["aif"].index] )
    {
        aj_asm.cmp(x86::al, 0x4F);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bx, 0x52'4D);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_iff
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_iff));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
        aj_asm.jmp(aj_loop_check_label);

// ; 0x47
aj_asm.bind(aj_signat_labels[9]);
    // ; gif : 0x47'49 : 0x46'38
    aj_asm.cmp(x86::al, 0x49);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x46'38);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_gif
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_gif));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x49
aj_asm.bind(aj_signat_labels[10]);
    // ; id3v2 : 0x49'44 : 0x33
    // ;  tfi  : 0x49'49 : 0x2A'00
    // ;  it   : 0x49'4D : 0x50'4D
aj_asm.bind(aj_sub_labels[16]); // id3v2 ?
    if ( selected_formats[fformats["mp3"].index] )
    {
        aj_asm.cmp(x86::al, 0x44);
        aj_asm.jne(aj_sub_labels[2]);
        aj_asm.cmp(x86::bh, 0x33);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_mp3
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_mp3));
        // для mp3 всегда выключен scrupulous mode.
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_off_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[2]); // tif_ii ?
    if ( selected_formats[fformats["tif"].index] )
    {
        aj_asm.cmp(x86::al, 0x49);
        aj_asm.jne(aj_sub_labels[3]);
        aj_asm.cmp(x86::bx, 0x2A'00);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_tif_ii
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_tif_ii));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[3]); // it ?
    if ( selected_formats[fformats["it"].index] )
    {
        aj_asm.cmp(x86::al, 0x4D);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bx, 0x50'4D);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_it
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_it));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; 4A
aj_asm.bind(aj_signat_labels[23]);
    // ; 669_jn : 0x4A'4E
    aj_asm.cmp(x86::ax, 0x4A'4E);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_669
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_669));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x4B
aj_asm.bind(aj_signat_labels[12]);
    // ; bk2 : 0x4B'42
    aj_asm.cmp(x86::al, 0x42);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_bink
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_bink));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x4D
aj_asm.bind(aj_signat_labels[13]);
    // ;   mk : 0x4D'2E : 0x4B'2E   ('M.K.')
    // ;  tfm : 0x4D'4D : 0x00'2A   ('II\x00\x2A')
    // ; mmd0 : 0x4D'4D : 0x44'30   ('MMD0')
    // ; mmd1 : 0x4D'4D : 0x44'31   ('MMD1')
    // ; mmd2 : 0x4D'4D : 0x44'32   ('MMD2')
    // ; mmd3 : 0x4D'4D : 0x44'33   ('MMD3')
    // ;  mid : 0x4D'54 : 0x68'64   ('MThd')
aj_asm.bind(aj_sub_labels[4]); // mod_m.k. ?
    if ( selected_formats[fformats["mod"].index] )
    {
        aj_asm.cmp(x86::al, 0x2E);
        aj_asm.jne(aj_sub_labels[5]);
        aj_asm.cmp(x86::bx, 0x4B'2E);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_mod
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_mod));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[5]); // tif_mm ?
    if ( selected_formats[fformats["tif"].index] )
    {
        aj_asm.cmp(x86::al, 0x4D);
        aj_asm.jne(aj_sub_labels[6]);
        aj_asm.cmp(x86::bx, 0x00'2A);
        aj_asm.jne(aj_sub_labels[17]);
        // вызов recognize_tif_mm
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_tif_mm));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[17]); // med MMDX ?
    if ( selected_formats[fformats["med"].index] )
    {
        aj_asm.cmp(x86::bh, 0x44);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bl, 0x30);
        aj_asm.jb(aj_loop_check_label);
        aj_asm.cmp(x86::bl, 0x33);
        aj_asm.ja(aj_loop_check_label);
        // вызов recognize_med
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_med));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[6]); // mid ?
    if ( selected_formats[fformats["mid"].index] )
    {
        aj_asm.cmp(x86::al, 0x54);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bx, 0x68'64);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_mid
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_mid));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; 0x4F
aj_asm.bind(aj_signat_labels[27]);
    // ; otc  : 0x4F'54 : 0x54'4F ('OTTO')
    // ; ogg  : 0x4F'67 : 0x67'53 ('OggS')
    if ( selected_formats[fformats["ttf"].index] )
    {
        aj_asm.cmp(x86::al, 0x54);
        aj_asm.jne(aj_sub_labels[22]);
        aj_asm.cmp(x86::bx, 0x54'4F);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_ttf
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_ttf));
        // для ttf всегда выключен scrupulous mode.
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_off_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[22]); // ogg ?
    if ( selected_formats[fformats["ogg"].index] or selected_formats[fformats["ogm"].index])
    {
        aj_asm.cmp(x86::al, 0x67);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bx, 0x67'53);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_ogg
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_ogg));
        // для ogg всегда выключен scrupulous mode.
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_off_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; 0x52
aj_asm.bind(aj_signat_labels[16]);
    // ; riff: 0x52'49 : 0x46'46
    aj_asm.cmp(x86::al, 0x49);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x46'46);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_riff
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_riff));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x53
aj_asm.bind(aj_signat_labels[17]);
    // ; s3m : 0x53'43 : 0x52'4D
    // ; sm2 : 0x53'4D : 0x4B'32
    // ; sm4 : 0x53'4D : 0x4B'34
aj_asm.bind(aj_sub_labels[7]); // s3m ?
    if ( selected_formats[fformats["s3m"].index] )
    {
        aj_asm.cmp(x86::al, 0x43);
        aj_asm.jne(aj_sub_labels[8]);
        aj_asm.cmp(x86::bx, 0x52'4D);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_s3m
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_s3m));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[8]); // smk ?
    if ( selected_formats[fformats["smk"].index] )
    {
        aj_asm.cmp(x86::al, 0x4D);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bh, 0x4B);
        aj_asm.jne(aj_loop_check_label);
        aj_asm.cmp(x86::bl, 0x32);
        aj_asm.jb(aj_loop_check_label);
        aj_asm.cmp(x86::bl, 0x34);
        aj_asm.ja(aj_loop_check_label);
        // вызов recognize_smk
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_smk));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; 0x69
aj_asm.bind(aj_signat_labels[22]);
    // ; 669_if : 0x69'66
    aj_asm.cmp(x86::ax, 0x69'66);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_669
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_669));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x6D
aj_asm.bind(aj_signat_labels[26]);
    // ; mdat: 6D'64 : 61'74
    // ; moov: 6D'6F : 6F'76
    aj_asm.cmp(x86::al, 0x64);
    aj_asm.je(aj_sub_labels[11]);
    aj_asm.cmp(x86::al, 0x6F);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x6F'76);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.jmp(aj_sub_labels[12]);
aj_asm.bind(aj_sub_labels[11]);
    aj_asm.cmp(x86::bx, 0x61'74);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.bind(aj_sub_labels[12]);
    // вызов recognize_mov_qt
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_mov_qt));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x74
aj_asm.bind(aj_signat_labels[33]);
    // ; ttc : 74'74 : 63'66
    aj_asm.cmp(x86::al, 0x74);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x63'66);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_ttc
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_ttc));
    // для ttc всегда выключен scrupulous mode.
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_off_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x89
aj_asm.bind(aj_signat_labels[20]);
    // ; png : 0x89'50 : 0x4E'47
    aj_asm.cmp(x86::al, 0x50);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x4E'47);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_png
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_png));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0xD7
aj_asm.bind(aj_signat_labels[29]);
    // ; wmf : 0xD7'CD : 0xC6'9A : placeable header
    aj_asm.cmp(x86::al, 0xCD);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0xC6'9A);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_wmf
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_wmf));
    // для placeable wmf всегда выключен scrupulous mode.
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_off_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0xFF
aj_asm.bind(aj_signat_labels[21]);
    // ; jpg : 0xFF'D8 : 0xFF'E0
    // ; mp3 : 0xFF'FA
    // ; mp3 : 0xFF'FB
    if ( selected_formats[fformats["jpg"].index] )
    {
        aj_asm.cmp(x86::al, 0xD8);
        aj_asm.jne(aj_sub_labels[15]);
        aj_asm.cmp(x86::bx, 0xFF'E0);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_jpg
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_jpg));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[15]);
    if ( selected_formats[fformats["mp3"].index] )
    {
        aj_asm.and_(x86::al, 0b11111110);
        aj_asm.cmp(x86::al, 0xFA);
        aj_asm.jne(aj_loop_check_label);
        // вызов recognize_mp3
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_mp3));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_off_label); // для mp3 всегда выключен scrupulous mode:
        //
    }
    aj_asm.jmp(aj_loop_check_label);

// ; "scrup mode on?" check
aj_asm.bind(aj_scrup_mode_check_label);
    aj_asm.cmp(x86::r15, 1); // включен scrup_mode ?
    aj_asm.je(aj_loop_check_label); // если включен, то перескока в буфере на размер найденного ресурса не делаем
// иначе
// ; scrup mode off -> значит делаем перескок на размер ресурса
aj_asm.bind(aj_scrup_mode_off_label);
    // в rax лежит размер ресурса,
    // в e->resource_offset смещение ресурса в файле (mmf-буфере).
    // надо переставить rsi и r14 на положение после ресурса минус 1 байт :
    aj_asm.mov(x86::r14, x86::qword_ptr((u64i)&this->resource_offset)); // суём в r14 смещение найденного ресурса относительно начала mmf (смещение ресурса не всегда равно первоначальному значению r14!)
    aj_asm.mov(x86::rsi, imm(&mmf_scanbuf));        // сбрасываем rsi на начало mmf-буфера (это абсолютный указатель)
    aj_asm.mov(x86::rsi, x86::dword_ptr(x86::rsi)); //
    aj_asm.add(x86::r14, x86::rax); // переставляем r14 на положение после ресурса
    aj_asm.dec(x86::r14);  // делаем -1, т.к. код под loop_check сделает +1 и выставит указатель ровно после ресурса
    aj_asm.add(x86::rsi, x86::r14); // корректируем rsi, чтобы абсолютный указатель тоже был после ресурса минус 1 байт

// ; loop_check
aj_asm.bind(aj_loop_check_label);
    aj_asm.inc(x86::rsi); // обновляем абсолютный адрес анализируемой ячейки
    aj_asm.inc(x86::r14); // обновляем смещение анализируемой ячейки, относительно начала mmf
    aj_asm.cmp(x86::rsi, x86::r12);
    aj_asm.jb(aj_loop_start_label); // jb - jump if below - переход если строго меньше

// ; epilog
aj_asm.bind(aj_epilog_label);
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из rsi в this->scanbuf_offset, который по адресу из r13
    aj_asm.add(x86::rsp, 40); // удаляем home-регион для callee-функций (для recognizer'ов)
    aj_asm.add(x86::rsp, imm(256*8));
    aj_asm.pop(x86::rbx);
    aj_asm.pop(x86::r15);
    aj_asm.pop(x86::r14);
    aj_asm.pop(x86::r13);
    aj_asm.pop(x86::r12);
    aj_asm.pop(x86::rsi);
    aj_asm.pop(x86::rdi);
    aj_asm.pop(x86::rbp);
    aj_asm.ret();

    Error err = aj_runtime.add(&comparation_func, &aj_code);
    // qInfo() << "runtime_add_error:" << err;
    //qInfo() << "---- ASMJIT Code ----";
    //qInfo() << aj_logger.data();
    //////////////////////////////////////////////////////////////
}

void Engine::scan_file_win64(const QString &file_name)
{
    file.setFileName(file_name);
    file_size = file.size();
    if ( !file.open(QIODeviceBase::ReadOnly) or ( file.size() < MIN_RESOURCE_SIZE ) )
    {
        file.close();
        return;
    }
    mmf_scanbuf = file.map(0, file_size);
    if ( mmf_scanbuf == nullptr )
    {
        qInfo() << "unsuccesful file to memory mapping";
        file.close();
        return;
    }
    //////////////////////////////////////////////////////////////
    u64i start_offset;
    u64i last_offset = 0; // =0 - важно!
    u64i iteration;
    u64i granularity = Settings::getBufferSizeByIndex(my_walker_parent->walker_config.bfr_size_idx) * 1024 * 1024;
    u64i tale_size = file_size % granularity;
    u64i max_iterations = file_size / granularity + ((tale_size >= 4) ? 1 : 0);
    u64i absolute_last_offset = file_size - 3; // -3, а не -4, потому что last_offset не включительно в цикле for : [start_offset, last_offset)
    u64i resource_size = 0;
    //////////////////////////////////////////////////////////////

    previous_msecs = QDateTime::currentMSecsSinceEpoch();
    done_cause_skip = false;
    Q_EMIT txFileChange(file_name, file.size()); // путь очередного файла всегда оправляем в SessionWindow для отображения
    for (iteration = 1; iteration <= max_iterations; ++iteration)
    {
        start_offset = last_offset;
        last_offset = ( iteration != max_iterations ) ? (last_offset += granularity) : absolute_last_offset;
        /// вызов сгенерированного кода
        comparation_func(start_offset, last_offset);
        ///
        switch (*command) // проверка на поступление команды управления
        {
        case WalkerCommand::Stop:
            //qInfo() << "-> Engine: i'm stopped due to Stop command";
            iteration = max_iterations;  // условие выхода из внешнего цикла do-while итерационных чтений файла
            break;
        case WalkerCommand::Pause:
            //qInfo() << "-> Engine: i'm paused due to Pause command";
            Q_EMIT my_walker_parent->txImPaused();
            control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
            // тут вдруг в главном потоке разблокировали mutex, поэтому пошли выполнять код ниже (пришла неявная команда Resume(Run))
            control_mutex->unlock();
            if ( *command == WalkerCommand::Stop ) // вдруг, пока мы стояли на паузе, была нажата кнопка Stop?
            {
                iteration = max_iterations; // условие выхода из внешнего цикла do-while итерационных чтений файла
                break;
            }
            Q_EMIT my_walker_parent->txImResumed();
            //qInfo() << " >>>> Engine : received Resume(Run) command, when Engine was running!";
            break;
        case WalkerCommand::Skip:
            //qInfo() << " >>>> Engine : current file skipped :" << file_name;
            *command = WalkerCommand::Run;
            iteration = max_iterations;  // условие выхода из внешнего цикла do-while итерационных чтений файла
            done_cause_skip = true; // это нужно в WalkerThread, чтобы понять почему сканирование завершилось
            break;
        default:; // сюда в случае WalkerCommand::Run
        }
        update_file_progress(file_name, file_size, scanbuf_offset); // посылаем сигнал обновить progress bar для файла
    }
    Q_EMIT txFileProgress(100);
    // qInfo() << "closing file";
    // qInfo() << "-> Engine: returning from scan_file() to caller WalkerThread";
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
        Q_EMIT txFileProgress(current_file_progress);
    }
}

bool Engine::enough_room_to_continue(u64i min_size)
{
    if ( file_size - scanbuf_offset >= min_size )
    {
        return true;
    }
    return false;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_bmp RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct FileHeader
    {
        u16i file_type; // "BM"
        u32i file_size;
        u16i reserved1;  // u16i reserved1 + u16i reserved2; = 0
        u16i reserved2;
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
    //qInfo() << " BMP recognizer called!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(FileHeader);
    static const QSet <u32i> VALID_BMP_HEADER_SIZE { 12, 40, 108 };
    static const QSet <u16i> VALID_BITS_PER_PIXEL { 1, 4, 8, 16, 24, 32 };
    static u32i stm_id {fformats["stm"].index}; // формат stm_bmod пересекается с bmp по сигнатуре, поэтому проверка сначала производится здесь
    static u32i bmp_id {fformats["bmp"].index};
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (FileHeader*)(&buffer[base_index]);
    if ( e->selected_formats[stm_id] )
    {
        if ( ( info_header->file_size == 0x5332444F /*OD2S*/ ) and ( info_header->reserved1 == 0x4D54 /*TM*/) ) return recognize_stm(e);
    }
    if ( !e->selected_formats[bmp_id] ) return 0;
    if ( ( info_header->reserved1 != 0 ) and ( info_header->reserved1 != 0 ) ) return 0;
    if ( !VALID_BMP_HEADER_SIZE.contains(info_header->bitmapheader_size) ) return 0;
    if ( info_header->planes != 1 ) return 0;
    if ( !VALID_BITS_PER_PIXEL.contains(info_header->bits_per_pixel) ) return 0;
    s64i file_size = e->file_size;
    u64i last_index = base_index + info_header->file_size;
    if ( last_index > file_size ) return 0; // неверное поле file_size
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    auto info = QString("%1x%2 %3-bpp").arg(QString::number(std::abs(info_header->width)),
                                            QString::number(std::abs(info_header->height)),
                                            QString::number(info_header->bits_per_pixel));
    e->resource_offset = base_index;
    Q_EMIT e->txResourceFound("bmp", "", base_index, resource_size, info);
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
    //qInfo() << "\nPNG recognizer called!\n";
    static const u64i min_room_need = sizeof(FileHeader) + sizeof(ChunkHeader) + sizeof(IHDRData) + sizeof(CRC);
    static const QSet <u8i> VALID_BIT_DEPTH  { 1, 2, 4, 8, 16 };
    static const QSet <u8i> VALID_COLOR_TYPE { 0, 2, 3, 4, 6 };
    static const QSet <u8i> VALID_INTERLACE  { 0, 1 };
    static const QSet <u32i> VALID_CHUNK_TYPE { 0x54414449 /*IDAT*/, 0x4D524863 /*cHRM*/, 0x414D4167 /*gAMA*/, 0x54494273 /*sBIT*/, 0x45544C50 /*PLTE*/, 0x44474B62 /*bKGD*/,
                                                0x54534968 /*hIST*/, 0x534E5274 /*tRNS*/, 0x7346466F /*oFFs*/, 0x73594870 /*pHYs*/, 0x4C414373 /*sCAL*/, 0x54414449 /*IDAT*/, 0x454D4974 /*tIME*/,
                                                0x74584574 /*tEXt*/, 0x7458547A /*zTXt*/, 0x63415266 /*fRAc*/, 0x67464967 /*gIFg*/, 0x74464967 /*gIFt*/, 0x78464967 /*gIFx*/, 0x444E4549 /*IEND*/,
                                                0x42475273 /*sRGB*/, 0x52455473 /*sTER*/, 0x4C414370 /*pCAL*/, 0x47495364 /*dSIG*/, 0x50434369 /*iCCP*/, 0x50434963 /*iICP*/, 0x7643446D /*mDCv*/,
                                                0x694C4C63 /*cLLi*/, 0x74585469 /*iTXt*/, 0x744C5073 /*sPLt*/, 0x66495865 /*eXIf*/, 0x52444849 /*iHDR*/ };
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    if ( *((u64i*)&buffer[base_index]) != 0x0A1A0A0D474E5089 /*.PNG\r\n \n*/ ) return 0;
    if ( *((u64i*)(&buffer[base_index + 0x08])) != 0x524448490D000000 ) return 0;
    auto ihdr_data = (IHDRData*)(&buffer[base_index + 0x10]);
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
    if ( last_index <= base_index ) return 0;
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
    auto info = QString(R"(%1x%2 (%3))").arg(   QString::number(be2le(ihdr_data->width)),
                                                QString::number(be2le(ihdr_data->height)),
                                                color_type );
    Q_EMIT e->txResourceFound("png", "", base_index, resource_size, info);
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
    struct FmtWavInfoHeader
    {
        u32i subchunk_id;
        u32i subchunk_size;
        WavInfoHeader info;
    };
#pragma pack(pop)
    //qInfo() << " RIFF RECOGNIZER CALLED:";
    static u32i avi_id {fformats["avi"].index};
    static u32i wav_id {fformats["wav"].index};
    static u32i rmi_id {fformats["rmi"].index};
    static u32i ani_id {fformats["ani_riff"].index};
    static const u64i min_room_need = sizeof(ChunkHeader);
    static const QSet <u64i> VALID_SUBCHUNK_TYPE {  0x5453494C20495641 /*AVI LIST*/,
                                                    0x20746D6645564157 /*WAVEfmt */,
                                                    0x4B4E554A45564157 /*WAVEJUNK*/,
                                                    0x6174616444494D52 /*RMIDdata*/,
                                                    0x5453494C4E4F4341 /*ACONLIST*/,
                                                    0x68696E614E4F4341 /*ACONanih*/
                                                    };
    static const QSet <u16i> VALID_WAV_BPS { 4, 8, 11, 12, 16, 18, 20, 24, 32, 48, 64 }; // https://en.wikipedia.org/wiki/Audio_bit_depth
    if ( ( !e->selected_formats[avi_id] ) and ( !e->selected_formats[wav_id] ) and ( e->selected_formats[rmi_id] ) and ( e->selected_formats[ani_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    u64i last_index = base_index + sizeof(ChunkHeader::chunk_id) + sizeof(ChunkHeader::chunk_size) + (*((ChunkHeader*)(&buffer[base_index]))).chunk_size; // сразу переставляем last_index в конец ресурса
    if ( last_index > file_size ) return 0; // неверное поле chunk_size
    if ( !VALID_SUBCHUNK_TYPE.contains(((ChunkHeader*)(&buffer[base_index]))->subchunk_id) ) return 0; // проверка на валидные id сабчанков
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    switch (((ChunkHeader*)(&buffer[base_index]))->subchunk_id)
    {
    case 0x5453494C20495641: // avi, "AVI LIST" subchunk
    {
        if ( ( !e->selected_formats[avi_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(AviInfoHeader)) ) return 0;
        auto avi_info_header = (AviInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        if ( avi_info_header->hdrl_avih_sign != 0x686976616C726468 /*hdrlavih*/ ) return 0;
        auto info = QString(R"(%1x%2)").arg(QString::number(avi_info_header->width),
                                            QString::number(avi_info_header->height));
        Q_EMIT e->txResourceFound("avi", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x20746D6645564157: // wav, "WAVEfmt " subchunk
    {
        if ( ( !e->selected_formats[wav_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(WavInfoHeader)) ) return 0;
        auto wav_info_header = (WavInfoHeader*)&buffer[base_index + sizeof(ChunkHeader)];
        if ( ( wav_info_header->chans == 0 ) or ( wav_info_header->chans > 6 ) ) return 0;
        if ( !VALID_WAV_BPS.contains(wav_info_header->bits_per_sample) ) return 0;
        QString codec_name = ( wave_codecs.contains(wav_info_header->fmt_code) ) ? wave_codecs[wav_info_header->fmt_code] : "unknown";
        auto info = QString(R"(%1 codec : %2-bit %3Hz %4-ch)").arg( codec_name,
                                                                    QString::number(wav_info_header->bits_per_sample),
                                                                    QString::number(wav_info_header->sample_rate),
                                                                    QString::number(wav_info_header->chans));
        Q_EMIT e->txResourceFound("wav", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x4B4E554A45564157: // wav, "WAVEJUNK" subchunk
    {
        if ( ( !e->selected_formats[wav_id] ) ) return 0;
        u64i fmt_offset = base_index + sizeof(ChunkHeader) + ((ChunkHeader*)(&buffer[base_index]))->subchunk_size;
        if ( file_size - fmt_offset < sizeof(FmtWavInfoHeader) ) return 0; // не хватает места под заголовок информационного сабчанка
        auto fmt_wav_info_header = (FmtWavInfoHeader*)&buffer[fmt_offset];
        if ( fmt_wav_info_header->subchunk_id != 0x20746D66 ) return 0;
        if ( ( fmt_wav_info_header->info.chans == 0 ) or ( fmt_wav_info_header->info.chans > 6 ) ) return 0;
        if ( !VALID_WAV_BPS.contains(fmt_wav_info_header->info.bits_per_sample) ) return 0;
        QString codec_name = ( wave_codecs.contains(fmt_wav_info_header->info.fmt_code) ) ? wave_codecs[fmt_wav_info_header->info.fmt_code] : "unknown";
        auto info = QString(R"(%1 codec : %2-bit %3Hz %4-ch)").arg( codec_name,
                                                                    QString::number(fmt_wav_info_header->info.bits_per_sample),
                                                                    QString::number(fmt_wav_info_header->info.sample_rate),
                                                                    QString::number(fmt_wav_info_header->info.chans));
        Q_EMIT e->txResourceFound("wav", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x6174616444494D52: // rmi - midi data encapsulated in riff, RMIDdata subchunk
    {
        if ( ( !e->selected_formats[rmi_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(MidiInfoHeader)) ) return 0;
        auto midi_info_header = (MidiInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        auto info = QString(R"(%1 track(s))").arg(be2le(midi_info_header->ntrks));
        Q_EMIT e->txResourceFound("rmi", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x5453494C4E4F4341: // ani - animated cursor with ACONLIST subchunk
    {
        if ( ( !e->selected_formats[ani_id] ) ) return 0;
        Q_EMIT e->txResourceFound("ani_riff", "", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x68696E614E4F4341: // ani - animated cursor with ACONanih subchunk
    {
        if ( ( !e->selected_formats[ani_id] ) ) return 0;
        Q_EMIT e->txResourceFound("ani_riff", "", base_index, resource_size, "");
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
    static const u64i min_room_need = sizeof(MidiChunk) + sizeof(HeaderData);
    static const QSet <u32i> VALID_CHUNK_TYPE { 0x6468544D /*MThd*/, 0x6B72544D /*MTrk*/};
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    auto info_header = (HeaderData*)(&buffer[base_index + sizeof(MidiChunk)]);
    if ( be2le((*((MidiChunk*)(&buffer[base_index]))).size) > 6 ) return 0;
    u64i last_index = base_index + 6 + sizeof(MidiChunk); // сразу переставляем last_index после заголовка на первый чанк типа MTrk
    if ( last_index >= file_size ) return 0; // прыгнули за пределы файла => явно неверное поле size в заголовке
    u64i possible_last_index;
    u64i tracks_counter = 0; // не доверяем информации из заголовка, а считаем треки сами
    while (true)
    {
        if ( last_index + sizeof(MidiChunk) > file_size ) break; // не хватает места для анализа очередного чанка => достигли конца ресурса
        if ( !VALID_CHUNK_TYPE.contains((*(MidiChunk*)(&buffer[last_index])).type) ) break; // встретили неизвестный чанк => достигли конца ресурса
        if ( (*(MidiChunk*)(&buffer[last_index])).type == 0x6B72544D ) ++tracks_counter;
        possible_last_index = last_index + be2le((*(MidiChunk*)(&buffer[last_index])).size) + sizeof(MidiChunk);
        if ( possible_last_index > file_size ) break; // возможно кривое поле .size, либо усечённый файл, либо type попался (внезапно) корректный, но size некорректный и т.д.
        last_index = possible_last_index;
    }
    if ( last_index <= base_index ) return 0;
    if ( tracks_counter == 0 ) return 0; // файл без треков не имеет смысла
    u64i resource_size = last_index - base_index;
    auto info = QString(R"(%1 track(s))").arg(tracks_counter);
    Q_EMIT e->txResourceFound("mid", "", base_index, resource_size, info);
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
        u32i format_type; // "ILBM", "AIFF", "XDIR"...
    };
    struct BMHD_InfoHeader
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
    static const QSet <u32i> VALID_FORMAT_TYPE {0x204D4250 /*PBM */, 0x4D424C49 /*ILBM*/, 0x38424752 /*RGB8*/, 0x4E424752 /*RGBN*/,
                                                0x4D424341 /*ACBM*/, 0x46464941 /*AIFF*/, 0x50454544 /*DEEP*/, 0x52494458 /*XDIR*/,
                                                0x43464941 /*AIFC*/, 0x58565338 /*8SVX*/, 0x56533631 /*16SV*/};
    if ( ( !e->selected_formats[lbm_id] ) and ( !e->selected_formats[aif_id] ) and ( !e->selected_formats[xmi_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    auto chunk_header = (ChunkHeader*)&buffer[base_index];
    u64i last_index = base_index + sizeof(ChunkHeader::chunk_id) + sizeof(ChunkHeader::chunk_size) + be2le((*((ChunkHeader*)(&buffer[base_index]))).chunk_size); // сразу переставляем last_index в конец ресурса (но для XMI это лишь начало структуры CAT
    if ( last_index > file_size ) return 0; // неверное поле chunk_size
    if ( !VALID_FORMAT_TYPE.contains(((ChunkHeader*)(&buffer[base_index]))->format_type) ) return 0; // проверка на валидные форматы
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    if ( resource_size <= 8 ) return 0;
    switch (((ChunkHeader*)(&buffer[base_index]))->format_type)
    {
    case 0x204D4250 : // "PBM " picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        QString info;
        if ( resource_size >= (sizeof(ChunkHeader) + sizeof(BMHD_InfoHeader)) )
        {
            auto bitmap_info_header = (BMHD_InfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
            if ( bitmap_info_header->local_chunk_id == 0x44484D42 /*BMHD*/ )
            {
                info = QString(R"(%1x%2 %3-bpp)").arg(  QString::number(be2le(bitmap_info_header->width)),
                                                        QString::number(be2le(bitmap_info_header->height)),
                                                        QString::number(bitmap_info_header->bitplanes));
            }
        }
        Q_EMIT e->txResourceFound("lbm", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x43464941: // "AIFC" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        Q_EMIT e->txResourceFound("aif", "", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x46464941: // "AIFF" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        QString info;
        if ( resource_size >= (sizeof(ChunkHeader) + sizeof(AIFF_CommonInfoHeader)) )
        {
            auto aiff_info_header = (AIFF_CommonInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
            if ( aiff_info_header->local_chunk_id == 0x4D4D4F43 /*'COMM'*/)
            {
                info = QString("%1-bit %2-ch").arg( QString::number(be2le(aiff_info_header->sample_size)),
                                                    QString::number(be2le(aiff_info_header->channels)));
            }
        }
        Q_EMIT e->txResourceFound("aif", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x38424752: // "RGB8" picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        QString info;
        if ( resource_size >= (sizeof(ChunkHeader) + sizeof(BMHD_InfoHeader)) )
        {
            auto bitmap_info_header = (BMHD_InfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
            if ( bitmap_info_header->local_chunk_id == 0x44484D42 /*BMHD*/ )
            {
                info = QString(R"(%1x%2 %3-bpp)").arg(  QString::number(be2le(bitmap_info_header->width)),
                                                        QString::number(be2le(bitmap_info_header->height)),
                                                        QString::number(bitmap_info_header->bitplanes - 1));
            }
        }
        Q_EMIT e->txResourceFound("lbm", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x4D424341: // "ACBM" picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        QString info;
        if ( resource_size >= (sizeof(ChunkHeader) + sizeof(BMHD_InfoHeader)) )
        {
            auto bitmap_info_header = (BMHD_InfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
            if ( bitmap_info_header->local_chunk_id == 0x44484D42 /*BMHD*/ )
            {
                info = QString(R"(%1x%2 %3-bpp)").arg(  QString::number(be2le(bitmap_info_header->width)),
                                                        QString::number(be2le(bitmap_info_header->height)),
                                                        QString::number(bitmap_info_header->bitplanes));
            }
        }
        Q_EMIT e->txResourceFound("lbm", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x4D424C49: // "ILBM" picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        QString info;
        if ( resource_size >= (sizeof(ChunkHeader) + sizeof(BMHD_InfoHeader)) )
        {
            auto bitmap_info_header = (BMHD_InfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
            if ( bitmap_info_header->local_chunk_id == 0x44484D42 /*BMHD*/ )
            {
                info = QString(R"(%1x%2 %3-bpp)").arg(  QString::number(be2le(bitmap_info_header->width)),
                                                        QString::number(be2le(bitmap_info_header->height)),
                                                        QString::number(bitmap_info_header->bitplanes));
            }
        }
        Q_EMIT e->txResourceFound("lbm", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x4E424752: // "RGBN" picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(BMHD_InfoHeader)) ) return 0;
        BMHD_InfoHeader *bitmap_info_header = (BMHD_InfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        QString info;
        if ( resource_size >= (sizeof(ChunkHeader) + sizeof(BMHD_InfoHeader)) )
        {
            auto bitmap_info_header = (BMHD_InfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
            if ( bitmap_info_header->local_chunk_id == 0x44484D42 /*BMHD*/ )
            {
                info = QString(R"(%1x%2 %3-bpp)").arg(  QString::number(be2le(bitmap_info_header->width)),
                                                        QString::number(be2le(bitmap_info_header->height)),
                                                        QString::number(bitmap_info_header->bitplanes - 1));
            }
        }
        Q_EMIT e->txResourceFound("lbm", "", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x50454544: // "DEEP" picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        Q_EMIT e->txResourceFound("lbm", "", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x52494458: // "XDIR" midi music
    {
        if ( ( !e->selected_formats[xmi_id] ) ) return 0;
        // у формата XMI особое строение, поэтому last_index сейчас стоит не на конце ресурса, а на структуре "CAT "
        if ( last_index + sizeof(XDIR_CatInfoHeader) > file_size ) return 0;
        auto cat_info_header = (XDIR_CatInfoHeader*)(&buffer[last_index]);
        if ( cat_info_header->cat_signature != 0x20544143 /*CAT */) return 0;
        last_index += (be2le(cat_info_header->size) + sizeof(XDIR_CatInfoHeader));
        if ( last_index > file_size) return 0;
        u64i resource_size = last_index - base_index;
        Q_EMIT e->txResourceFound("xmi", "", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x58565338: // "8SVX" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        Q_EMIT e->txResourceFound("aif", "", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x56533631: // "16SV" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        Q_EMIT e->txResourceFound("aif", "", base_index, resource_size, "");
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
    //qInfo() << " PCX RECOGNIZER CALLED!";
    static const u64i min_room_need = sizeof(PcxHeader);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    auto pcx_info_header = (PcxHeader*)(&(e->mmf_scanbuf[e->scanbuf_offset]));
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
    if ( ( width >= 4096 ) or ( height >= 4096 ) ) return 0; // старый формат, вряд ли будут большие изображения
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
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    auto info = QString("%1x%2 %3-bpp").arg(QString::number(width),
                                            QString::number(height),
                                            QString::number(bpp));
    Q_EMIT e->txResourceFound("pcx", "", base_index, resource_size, info);
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
    //qInfo() << " GIF recognizer called!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(FileHeader) + 1; // где +1 на u8i separator
    static const QSet <u16i> VALID_VERSIONS  { 0x6137 /*7a*/, 0x6139 /*9a*/ };
    static const QSet <u8i>  EXT_LABELS_TO_PROCESS { 0x01, 0xCE, 0xFE, 0xFF };
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    auto info_header = (FileHeader*)(&buffer[base_index]);
    if ( !VALID_VERSIONS.contains(info_header->version) ) return 0;
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
            // теперь last_index стоит на данных, а точнее на счётчике первого блока данных
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
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    auto info = QString("%1x%2").arg(   QString::number(info_header->width),
                                        QString::number(info_header->height));
    Q_EMIT e->txResourceFound("gif", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_jpg RECOGNIZE_FUNC_HEADER
{
// https://en.wikipedia.org/wiki/JPEG#Syntax_and_structure
// + JPG_diagram_by_Ange_Albertini.png
#pragma pack(push,1)
    struct SOI_Identifier
    {
        u16i soi;
    };
    struct MarkerHeader
    {
        u16i marker_id;
        u16i len;
    };
    struct APP0_Marker
    {
        u32i identifier_4b;
        u8i  identifier_1b;
        u16i version;
        u8i  units;
        u16i xdens, ydens;
        u8i  xthumb, ythumb;
    };
    struct SOF_Marker
    {
        u8i  bpp;
        u16i height;
        u16i width;
        u8i  components;
    };
#pragma pack(pop)
    //qInfo() << "JPG RECOGNIZER CALLED: " << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(SOI_Identifier) + sizeof(MarkerHeader);
    static const QSet <u16i> VALID_VERSIONS  { 0x0100, 0x0101, 0x0102 };
    static const QSet <u8i> VALID_UNITS { 0x00, 0x01, 0x02 };
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    u64i last_index = base_index + sizeof(SOI_Identifier); // встали на самый первый маркер
    bool progressive = false;
    u16i image_width = 0;
    u16i image_height = 0;
    u16i image_bpp = 0;
    bool info_derived = false;
    bool sos_found = false;
    while(!sos_found) // задача найти маркер SOS
    {
        if ( last_index + sizeof(MarkerHeader) > file_size ) return 0; // нет места на marker id
        u16i marker_id = be2le((*((MarkerHeader*)&buffer[last_index])).marker_id);
        u16i marker_len= be2le((*((MarkerHeader*)&buffer[last_index])).len);
        if ( (marker_id & 0b1111111111110000) == 0xFFE0 ) // Application marker : 0xFFE0 - 0xFFEF
        {
            if ( marker_id == 0xFFE0) // APP0 marker
            {
                if ( last_index + sizeof(MarkerHeader) + sizeof(APP0_Marker) > file_size ) return 0; // нет места на маркер APP0
                auto app0_marker = (APP0_Marker*)&buffer[last_index + sizeof(MarkerHeader)];
                if ( app0_marker->identifier_4b != 0x4649464A /*JFIF*/ ) return 0;
                if ( app0_marker->identifier_1b != 0 ) return 0;
                if ( !VALID_VERSIONS.contains(be2le(app0_marker->version)) ) return 0;
                if ( !VALID_UNITS.contains(app0_marker->units) ) return 0;
            }
            //qInfo() << QString("APP%1 at:").arg(QString::number(marker_id & 0b1111)) << last_index;
            last_index += (sizeof(MarkerHeader::marker_id) + marker_len);
            continue;
        }

        switch(marker_id)
        {
        case 0xFFDB: // DQT - define quantization table
        {
            if ( last_index + sizeof(MarkerHeader) > file_size ) return 0; // нет места на маркер DQT
            //qInfo() << "DQT at:" << last_index;
            last_index += (sizeof(MarkerHeader::marker_id) + marker_len);
            break;
        }
        case 0xFFC2: // SOF0 - start of frame
        {
            progressive = true;
        }
        case 0xFFC0: // SOF2
        {
            if ( last_index + sizeof(MarkerHeader) + sizeof(SOF_Marker) > file_size ) return 0; // нет места на маркер SOF2
            auto sof_marker = (SOF_Marker*)&buffer[last_index + sizeof(MarkerHeader)];
            //qInfo() << "SOF(0/2) at:" << last_index << ">>> bpp:" << sof_marker->bpp << "; comp:" << sof_marker->components << "; width:" << be2le(sof_marker->width) << "; height:" << be2le(sof_marker->height);
            image_width = be2le(sof_marker->width);
            if ( image_width == 0 ) return 0;
            image_height = be2le(sof_marker->height);
            if ( image_height == 0 ) return 0;
            image_bpp = sof_marker->bpp * sof_marker->components;
            if ( image_bpp != 24 ) return 0;
            info_derived = true;
            last_index += (sizeof(MarkerHeader::marker_id) + marker_len);
            break;
        }
        case 0xFFC4: // DHT - define huffman table
        {
            if ( last_index + sizeof(MarkerHeader) > file_size ) return 0; // нет места на маркер DHT
            //qInfo() << "DHT at:" << last_index;
            last_index += (sizeof(MarkerHeader::marker_id) + marker_len);
            break;
        }
        case 0xFFDA: // SOS - start of scan
        {
            if ( last_index + sizeof(MarkerHeader) > file_size ) return 0; // нет места на маркер SOS
            //qInfo() << "SOS at:" << last_index;
            last_index += (sizeof(MarkerHeader::marker_id) + marker_len);
            sos_found = true;
            break;
        }
        default: // неизвестный маркер
            // не нашли SOS, значит капитуляция
            //qInfo() << "Unknown marker at:" << last_index;
            return 0;
        }
    }
    // здесь last_index стоит на SOS-данных
    if ( !info_derived ) return 0; // в ресурсе не было маркера SOF0 или SOF2? тогда ресурс некорректный
    while(true) // ищем EOI (end of image) \0xFF\0xD9
    {
        if ( last_index + 2 > file_size ) return 0; // есть место под EOI ?
        if ( *((u16i*)(&buffer[last_index])) == 0xD9FF ) break; // нашли EOI
        ++last_index;
    }
    last_index += 2; // на размер EOI-идентификатора
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    QString info = QString("%1x%2 %3-bpp (%4 DCT)").arg(QString::number(image_width),
                                                    QString::number(image_height),
                                                    QString::number(image_bpp),
                                                    progressive ? "progressive" : "baseline");
    Q_EMIT e->txResourceFound("jpg", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_mod RECOGNIZE_FUNC_HEADER
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
    //qInfo() << " MOD RECOGNIZER CALLED: " << e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    u64i base_index = e->scanbuf_offset;
    // определяем положительную поправку для размера заголовка :
    // +0 - для 'M.K.', 'FLT4', 'FLT8'
    // +1 - для 'xCHN'
    // +2 - для 'xxCH'
    u64i offset_correction = 0;
    u64i channels = 4;             // это стандартное значение для M.K.-модуля (далее может меняться)
    u64i steps_in_pattern = 64;    // это стандартное значение для M.K.-модуля (далее может меняться)
    u64i one_note_size = 4;        // это стандартное значение для M.K.-модуля (далее может меняться)
    if ( *((u16i*)&buffer[base_index]) == 0x4843 /*'CH'*/)
    {
        if ( buffer[base_index + 2] == 'N' ) // 'xCHN'
        {
            offset_correction = 1;
            if ( base_index < offset_correction) return 0;
            channels = buffer[base_index - offset_correction];
            if ( ( channels < 0x31 ) or ( channels > 0x39) ) return 0;
            channels -= 0x30;
            if ( channels % 2 ) return 0; // отсекаем с нечётным количеством каналов
        }
        else // 'xxCH'
        {
            offset_correction = 2;
            if ( base_index < offset_correction ) return 0;
            u8i tens_place_ch_num;  // разряд десятков
            u8i units_place_ch_num; // разряд единиц
            tens_place_ch_num =  buffer[base_index - offset_correction];
            units_place_ch_num = buffer[base_index - offset_correction + 1];
            if ( ( tens_place_ch_num < 0x31 ) or ( tens_place_ch_num > 0x33) ) return 0;
            if ( ( units_place_ch_num < 0x30 ) or ( units_place_ch_num > 0x39) ) return 0;
            channels = (tens_place_ch_num - 0x30) * 10 + (units_place_ch_num - 0x30);
            if ( channels % 2 ) return 0; // отсекаем с нечётным количеством каналов
            if ( channels > 32 ) return 0;
        }
    }
    if ( *((u32i*)&buffer[base_index]) == 0x38544C46 /*'FLT8'*/)
    {
        channels = 8;
    }
    //
    if ( e->scanbuf_offset < sizeof(MOD_31_Header) + offset_correction ) return 0;
    base_index = e->scanbuf_offset - (sizeof(MOD_31_Header) + offset_correction);
    auto info_header = (MOD_31_Header*)(&buffer[base_index]);
    // сигнатура 'CH' часто встречается, поэтому надёжный метод проверить название модуля на запрещённые символы
    for (u8i idx = 0; idx < sizeof(MOD_31_Header::song_name); ++idx)
    {
        if ( ( info_header->song_name[idx] != 0 ) and ( info_header->song_name[idx] < 32 )) return 0;
        if ( ( info_header->song_name[idx] != 0 ) and ( info_header->song_name[idx] > 126 )) return 0;
    }
    //
    u64i samples_block_size = 0;
    for (int sample_id = 0; sample_id < 31; ++sample_id) // калькуляция размера блока сэмплов
    {
        // qInfo() << " sample id:" << sample_id << " size:" << u32i(be2le(info_header->sample_descriptors[sample_id].len)) * 2 << " repeat_offset:" << u32i(be2le(info_header->sample_descriptors[sample_id].repeat_offset)) * 2
        //         << " repeat_len:" << u32i(be2le(info_header->sample_descriptors[sample_id].repeat_len)) * 2;
        if ( be2le(info_header->sample_descriptors[sample_id].len) > 0 ) // проверка на корректность заголовка сэмпла
        {
            if ( u32i(be2le(info_header->sample_descriptors[sample_id].repeat_offset)) * 2 + u32i(be2le(info_header->sample_descriptors[sample_id].repeat_len)) * 2 > u32i(be2le(info_header->sample_descriptors[sample_id].len)) * 2 ) return 0;
        }
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
    u64i patterns_block_size = (steps_in_pattern * channels * one_note_size) * most_pattern_number;
    u64i last_index = base_index + sizeof(MOD_31_Header) + 4 /*signature size*/ + patterns_block_size + samples_block_size;
    if ( last_index > e->file_size) return 0; // капитуляция при неверном размере; обрезанные не сохраняем
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 20; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [19]
    {
        if ( info_header->song_name[song_name_len] == 0 ) break;
    }
    auto info = QString("%1-chan. song '%2'").arg(QString::number(channels),
                                                    QString(QByteArray((char*)(info_header->song_name), song_name_len)));
    Q_EMIT e->txResourceFound("mod", "", base_index, resource_size, info);
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
        u16i song_len, song_restart_pos, channels_number, patterns_number, instruments_number;
        u16i flags, default_tempo, default_bpm;
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
    static const u64i min_room_need = sizeof(XM_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    auto info_header = (XM_Header*)(&buffer[base_index]);
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
            // после выхода из цикла по заголовком сэмплов last_index стоит уже на блоке самих сэмплов
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
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 20; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [19]
    {
        if ( info_header->module_name[song_name_len] == 0 ) break;
    }
    auto info = QString("%1-chan. song '%2'").arg(QString::number(info_header->channels_number),
                                                    QString(QByteArray((char*)(info_header->module_name), song_name_len)));
    Q_EMIT e->txResourceFound("xm", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_stm RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct InstrHeader
    {
        u8i  filename[12];
        u8i  zero;
        u8i  disk;
        u16i sample_offset; // in paragraphs
        u16i sample_len;
        u16i loop_start;
        u16i loop_end;
        u8i  volume;
        u8i  reserved1;
        u16i c4speed;
        u32i reserved2;
        u16i para_len;
    };
    struct STM_Header
    {
        u8i  song_name[20];
        u64i tracker_name; //  !Scream! или BMOD2STM
        u8i  str_terminator; // 0x1A
        u8i  file_type;
        u8i  maj_ver;
        u8i  min_ver;
        u8i  default_tempo;
        u8i  pat_num;
        u8i  default_volume;
        u8i  reserved[13];
        InstrHeader instr_headers[31];
        u8i  pattern_order[128]; // но в доках почему-то 64 - возможно так для ScreamTracker 1 ?
    };
#pragma pack(pop)
    //qInfo() << "STM RECOGNIZER CALLED: " << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(STM_Header) - sizeof(STM_Header::song_name);
    if ( e->scanbuf_offset < sizeof(STM_Header::song_name) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset - sizeof(STM_Header::song_name); // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (STM_Header*)&buffer[base_index];
    bool bmod2stm = false;
    if ( info_header->tracker_name != 0x216D616572635321 /*!Scream!*/ )
    {
        if ( info_header->tracker_name != 0x4D545332444F4D42 /*BMOD2STM*/ )
        {
            return 0;
        }
        else
        {
            bmod2stm = true;
        }
    }
    if ( info_header->str_terminator != 0x1A ) return 0;
    if ( ( info_header->file_type != 1 ) and ( info_header->file_type != 2 ) ) return 0;
    if ( info_header->maj_ver != 2 ) return 0;
    // qInfo() << "patterns_num:" << info_header->pat_num;
    auto instr_headers = (InstrHeader*)&buffer[base_index + sizeof(STM_Header) - sizeof(STM_Header::instr_headers) - sizeof(STM_Header::pattern_order)];
    QMap<u32i, u32i> db; // база данных смещений сэмплов (а так же смещения блока паттернов, т.к. может существовать модуль без сэмлов)
    db[sizeof(STM_Header)] = 1024 * info_header->pat_num; // сразу заносим смещение (относительно base_index) и размер блока паттернов
    u32i minimal_samples_offset = db.lastKey() + db.last(); // минимальное смещение начала сэмплов
    // qInfo() << "minimal_samples_offset:" << minimal_samples_offset;
    for(u8i idx = 0; idx < 31; ++idx)
    {
        u32i long_sample_offset = instr_headers[idx].sample_offset * 16;
        // qInfo() << "instr_idx:" << idx << "; sample_offset:" << long_sample_offset << "; sample_len:" << instr_headers[idx].sample_len
        //         << "; loop_start:" << instr_headers[idx].loop_start << "; loop_end:" << instr_headers[idx].loop_end << "; volume:" << instr_headers[idx].volume
        //         << "; c4speed:" << instr_headers[idx].c4speed;
        if ( ( long_sample_offset >= minimal_samples_offset ) and ( instr_headers[idx].sample_len > 1) ) // повсеместно встречаются сэмплы размером 1, но по факту это то же самое, что и 0
        {
            db[long_sample_offset] = instr_headers[idx].sample_len;
        }
    }
    //qInfo() << db;
    u64i resource_size = db.lastKey() + db[db.lastKey()];
    s64i file_size = e->file_size;
    if ( file_size - base_index < resource_size ) return 0; // вычисленный размер не вмещается в исходник
    if ( bmod2stm ) // bmod2stm выравнивает размер модуля на границу 16 байт с помощью zero padding
    {
        resource_size = (resource_size + 15) & 0xFFFFFFFF'FFFFFFF0; // эквив. формуле ((resource_size + 15) / 16)*16 или ((resource_size + 15) >> 4 ) << 4
        if ( file_size - base_index < resource_size ) resource_size = file_size - base_index;
    }
    int song_name_len;
    for (song_name_len = 0; song_name_len < 20; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [19]
    {
        if ( info_header->song_name[song_name_len] == 0 ) break;
    }
    auto info = QString("song '%1'").arg(QString(QByteArray((char*)(info_header->song_name), song_name_len)));
    Q_EMIT e->txResourceFound("stm", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_s3m RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct S3M_Header
    {
        u8i  song_name[28];
        u8i  Ox1A; // 0x1A
        u8i  type;
        u16i not_used, ordnum, insnum, patnum, flags, cr_with_ver, fmt_ver;
        u32i signature; // "SCRM"
        u8i  global_vol, initial_speed, initial_tempo, master_vol;
        u8i  reserved[10];
        u16i special_ptr;
        u8i  channel_settings[32];
    };
    struct SampleHeader
    {
        u8i  type;
        u8i  dos_filename[12];
        u8i  memseg_hi;
        u16i memseg_lo;
        u32i len, loop_begin, loop_end;
        u8i  volume, reserved2, pack, flags;
        u32i middle_c_herz;
        u8i  reserved3[4];
        u16i int_gp, int_512;
        u32i int_last_used;
        u8i  sample_name[28];
        u32i signature; // "SCRS"
    };
#pragma pack(pop)
    // предполагается, что порядок организации ресурса всегда такой же, как указано в документе s3m-form.txt
    // - header
    // - instruments in order
    // - patterns in order
    // - samples in order
    // поэтому задача отследить смещение данных самого дальнего сэмпла и его размер
    static const u64i min_room_need = 52;
    static const QSet <u32i> VALID_INSTR_SIGNATURE { 0x53524353 /*SCRS*/, 0x49524353 /*SCRI*/, 0x00000000 /*empty instrument*/};
    if ( e->scanbuf_offset < 44 ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0; // хватит ли места, начиная с поля signature?
    auto buffer = e->mmf_scanbuf;
    u64i base_index = e->scanbuf_offset - 44;
    auto info_header = (S3M_Header*)(&buffer[base_index]);
    if ( info_header->Ox1A != 0x1A ) return 0;
    if ( info_header->type != 16 ) return 0;
    if ( info_header->ordnum % 2 != 0 ) return 0; // длина списка воспроизведения всегда д.б. чётной
    // if ( info_header->insnum == 0 ) return 0; // модуль без инструментов не имеет смысла?
    u64i after_header_block_size = info_header->ordnum + info_header->insnum * 2 + info_header->patnum * 2; // блок с Orders + instruments parapointers + pattern parapointers
    s64i file_size = e->file_size;
    if ( base_index + sizeof(S3M_Header) + after_header_block_size > file_size ) return 0; // не хватает места под orders + parapointers
    QMap<u32i, u32i> pp_db; // бд парапойнтеров на тела сэмплов (либо на тела паттернов, если сэмплов не было); ключ - парапойнтер, значение - размер блока по адресу парапойнтера
    SampleHeader *sample_header;
    u64i pointer32; // для преобразования парапойнтера через *16
    auto parapointer = (u16i*)(&buffer[base_index + sizeof(S3M_Header) + info_header->ordnum]); // формируем указатель на [0] индекс списка instrument parapointers
    u32i long_memseg;
    for (u16i pp_idx = 0; pp_idx < info_header->insnum; ++pp_idx) // и идём по этому списку
    {
        pointer32 = parapointer[pp_idx] * 16; // абсолютное смещение в файле ресура (но не в файле поиска, т.к. ресурс может быть со смещением)
        if ( base_index + pointer32 + sizeof(SampleHeader) > file_size ) return 0; // заголовок сэмпла не помещается в файл? -> капитуляция
        sample_header = (SampleHeader*)(&buffer[base_index + pointer32]);
        if ( !VALID_INSTR_SIGNATURE.contains(sample_header->signature) ) return 0; // неверная сигнатура заголовка инструмента
        if ( sample_header->signature == 0x53524353 ) // надо проверить на сигнатуру, потому что треккер сохраняет даже инструменты без сэмпла только ради отображения sample name, где располагаются комментарии
        {
            long_memseg = u32i(sample_header->memseg_lo) | (u32i(sample_header->memseg_hi) << 16); // согласно S3M.SVG
            if ( base_index + long_memseg * 16 + sample_header->len > file_size ) return 0; // не хватает места под сэмпл
            pp_db[long_memseg] = sample_header->len;
        }
    }
    u64i last_index = file_size + 1; // выставляем заведомо плохое значение : оно изменится (или нет) в следующем блоке if
    // список smp_pp_db упорядочен по ключам : самый старший ключ - наиболее дальний парапойнтер в ресурсе, его значение - длина блока
    if ( pp_db.count() == 0 ) // а были ли сэмплы? иногда присутствуют лишь Adblib-инструменты "SCRI", у которых есть только заголовок и нет тела, поэтому последними в файле идут паттерны -> надо отследить последний паттерн
    {
        parapointer = (u16i*)(&buffer[base_index + sizeof(S3M_Header) + info_header->ordnum + info_header->insnum * 2]); // формируем указатель на [0] индекс списка pattern parapointers
        for (u16i pp_idx = 0; pp_idx < info_header->patnum; ++pp_idx) // и идём по этому списку
        {
            pointer32 = parapointer[pp_idx] * 16; // абсолютное смещение в файле ресурса (но не в файле поиска, т.к. ресурс может быть со смещением)
            if ( base_index + pointer32 + 2 > file_size ) return 0; // заголовок паттерна (а именно поле Length) не помещается в файл? -> капитуляция
            pp_db[parapointer[pp_idx]] = 10240 + 2; // грубо принимаем, что паттерн в распакованном виде не более 1024 байт + 2 байта на поле Length
        }
        if ( pp_db.count() == 0 ) return 0; // ресурс без сэмплов и без паттернов не имеет смыслы -> капитуляция
        if ( pp_db.lastKey() * 16 < sizeof(S3M_Header) + after_header_block_size ) return 0; // проверка парапойнтера на корректность: вдруг он залез вообще на начало ресурса куда-то в заголовок?
        last_index = base_index + pp_db.lastKey() * 16 + pp_db[pp_db.lastKey()];
        if ( last_index > file_size ) last_index = file_size; // т.к. мы брали размер паттерна грубо 10K, то возможен выход за пределы скан-файла -> обрезаем last_index по границе файла
    }
    else // сэмплы есть, значит отслеживаем по бд сэмплов самый последний
    {
        if ( pp_db.lastKey() * 16 < sizeof(S3M_Header) + after_header_block_size ) return 0; // проверка парапойнтера на корректность: вдруг он залез вообще на начало ресурса куда-то в заголовок?
        last_index = base_index + pp_db.lastKey() * 16 + pp_db[pp_db.lastKey()];
    }
    if ( last_index > file_size ) return 0;
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 28; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [27]
    {
        if ( info_header->song_name[song_name_len] == 0 ) break;
    }
    auto info = QString("song '%1'").arg(QString(QByteArray((char*)(info_header->song_name), song_name_len)));
    Q_EMIT e->txResourceFound("s3m", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_it RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct IT_Header
    {
        u32i signature;
        u8i  song_name[26];
        u16i philiht, ordnum, insnum, smpnum, patnum;
        u16i cr_with_ver, compat_ver, flags, special;
        u8i  gv, mv, is, it, sep, pwd;
        u16i message_len;
        u32i message_offset;
        u32i reserved;
        u8i  channel_panning[64];
        u8i  channel_volumes[64];
    };
    struct SampleHeader
    {
        u32i signature; // "IMPS"
        u8i  dos_filename[12];
        u8i  Ox00, gvl, flag, vol;
        u8i  sample_name[26];
        u8i  cvt, dfp;
        u32i len, loop_begin, loop_end, c5_speed;
        u32i sus_loop_begin, sus_loop_end;
        u32i sample_pointer;
        u8i  vis, vid, vir, vit;
    };
    struct PatternHeader
    {
        u16i len;
        u16i rows;
        u8i  reserved[4];
    };
#pragma pack(pop)
    //qInfo() << "\nIT recognizer called!\n";
    static const u64i min_room_need = sizeof(IT_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (IT_Header*)(&buffer[base_index]);
    if ( ( info_header->smpnum == 0 ) and ( info_header->patnum == 0 ) ) return 0; // модуль без сэмплов и одновременно без паттернов не имеет смысла
    if ( info_header->gv > 128 ) return 0; // global volume
    if ( info_header->mv > 128 ) return 0; // mixing volume
    if ( info_header->sep > 128 ) return 0; // panning separation
    u64i after_header_block_size = info_header->ordnum + info_header->insnum * 4 + info_header->smpnum * 4 + info_header->patnum * 4; // блок с Orders + instruments offset + sample headers offset + pattern offset
    s64i file_size = e->file_size;
    if ( base_index + sizeof(IT_Header) + after_header_block_size > file_size ) return 0; // не хватает места для orders + offsets
    QMap<u32i, u32i> offsets_db; // бд указателей на тела сэмплов (либо на тела паттернов, если сэмплов не было); ключ - смещение, значение - размер блока по адресу смещения
    // qInfo() << "   ---> number of samples:" << info_header->smpnum;
    // qInfo() << "   ---> list of sample header offsets starts at: 0x" << QString::number(base_index + sizeof(IT_Header) + info_header->ordnum + info_header->insnum * 4, 16);
    auto pointer = (u32i*)&buffer[base_index + sizeof(IT_Header) + info_header->ordnum + info_header->insnum * 4];
    SampleHeader *sample_header;
    u64i multiplier; // 1 - 8bit, 2 - 16bit'ный сэмпл
    for (u16i idx = 0; idx < info_header->smpnum; ++idx)
    {
        if ( base_index + pointer[idx] + sizeof(SampleHeader) > file_size ) return 0; // нет места для анализа заголовка сэмпла
        sample_header = (SampleHeader*)&buffer[base_index + pointer[idx]];
        if ( sample_header->signature != 0x53504D49 ) return 0; // нет сигнатуры сэмпла
        multiplier = ((sample_header->flag & 2) == 2) ? 2 : 1;
        //qInfo() << "sample id:" << idx << " sample_pointer:" << sample_header->sample_pointer << " sample_len:" << sample_header->len * multiplier << " bits:" << 8 * multiplier;
        if ( !offsets_db.contains(sample_header->sample_pointer) or ( sample_header->len == 0) ) // встречаются модули (SLEEP.IT), где заголовки сэмплов ссылаются на одно и то же смещение -> надо это пресечь и доверять только первому уникальному смещению.
                                                                                                 // сэмплы нулевой длины тоже не учитываем.
        {
            offsets_db[sample_header->sample_pointer] = sample_header->len * multiplier;
            if ( base_index + sample_header->sample_pointer + sample_header->len * multiplier > file_size ) return 0; // сэмпл за пределами скан-файла
        }
    }
    u64i last_index = file_size + 1; // выставляем заведомо плохое значение : оно изменится (или нет) в следующем блоке if
    if ( offsets_db.count() == 0 ) // сэмплов не было, тогда отследим последний паттерн
    {
        if ( info_header->patnum == 0 ) return 0; // нет паттернов -> модуль не имеет смысла
        pointer = (u32i*)&buffer[base_index + sizeof(IT_Header) + info_header->ordnum + info_header->insnum * 4 + info_header->smpnum * 4];
        PatternHeader *pattern_info;
        for (u16i idx = 0; idx < info_header->patnum; ++idx)
        {
            if ( base_index + pointer[idx] + 8 > file_size ) return 0; // нет места для анализа заголовка паттерна
            if ( pointer[idx] != 0 )
            {   // IT-FORM.TXT : Note that if the (long) offset to a pattern = 0, then the
                // IT-FORM.TXT : pattern is assumed to be a 64 row empty pattern.
                // судя по выдержке бывают нулевые указатели на абсолютно пустые паттерны, у которых нет тела в файле,
                // поэтому данный if-else обслуживает корректную логику добавления в бд
                pattern_info = (PatternHeader*)&buffer[base_index + pointer[idx]];
                // qInfo() << "pattern id:" << idx << "at offset:" << base_index + pointer[idx] << " len:" << pattern_info->len;
                if ( base_index + pointer[idx] + 8 + pattern_info->len > file_size ) return 0; // нет места для тела паттерна
                offsets_db[pointer[idx]] = pattern_info->len + 8;
            }
            else
            {
                offsets_db[0] = 0;
            }
        }
        if ( offsets_db.lastKey() < sizeof(IT_Header) + after_header_block_size ) return 0; // проверка указателя на корректность: вдруг он залез вообще на начало ресурса куда-то в заголовок?
        last_index = base_index + offsets_db.lastKey() + offsets_db[offsets_db.lastKey()];
      }
    else
    {
        if ( offsets_db.lastKey() < sizeof(IT_Header) + after_header_block_size ) return 0; // проверка указателя на корректность: вдруг он залез вообще на начало ресурса куда-то в заголовок?
        last_index = base_index + offsets_db.lastKey() + offsets_db[offsets_db.lastKey()];
    }
    if ( last_index > file_size ) return 0; // на всякий случай последняя проверка
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 26; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [25]
    {
        if ( info_header->song_name[song_name_len] == 0 ) break;
    }
    auto info = QString("song '%1'").arg(QString(QByteArray((char*)(info_header->song_name), song_name_len)));
    Q_EMIT e->txResourceFound("it", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_bink RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct BINK_Header
    {
        u16i signature1;
        u8i  signature2;
        u8i  version;
        u32i size; // без учёта полей size, ver, sign1, sign2
        u32i frames_num, largest_frame_size, unknown1;
        u32i video_width, video_height;
        u32i video_fps;
        u32i image_fmt;
        u32i unknown2;
        u32i audio_flag;
    };
    struct AudioHeader
    {
        u16i channels_num;
        u16i unknown1;
        u16i sample_rate;
        u16i unknown2[6];
    };
#pragma pack(pop)
    static u32i bink1_id {fformats["bik"].index};
    static u32i bink2_id {fformats["bk2"].index};
    static const u64i min_room_need = sizeof(BINK_Header);
    if ( ( !e->selected_formats[bink1_id] ) and ( !e->selected_formats[bink2_id] ) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (BINK_Header*)(&buffer[base_index]);
    if ( ( info_header->signature1 == 0x4942 /*BI*/ ) and ( info_header->signature2 != 'K' ) ) return 0;
    if ( ( info_header->signature1 == 0x424B /*KB*/ ) and ( info_header->signature2 != '2' ) ) return 0;
    if ( ( info_header->version < 'a' ) or ( info_header->version > 'z' ) ) return 0;
    if ( ( info_header->audio_flag > 1 ) ) return 0; // только 0 и 1
    if ( ( info_header->video_width == 0 ) or ( info_header->video_height == 0 ) ) return 0;
    if ( ( info_header->video_width > 6000 ) or ( info_header->video_height > 6000 ) ) return 0;
    s64i file_size = e->file_size;
    if ( base_index + sizeof(BINK_Header) + sizeof(AudioHeader) * info_header->audio_flag > file_size ) return 0;
    u64i last_index = base_index + info_header->size + 8;
    if ( last_index > file_size ) return 0;
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    if ( resource_size <= (info_header->size + 8) ) return 0; // ресурс размером с заголовок не имеет смысла
    auto info = QString("%1x%2").arg(   QString::number(info_header->video_width),
                                        QString::number(info_header->video_height));
    switch (info_header->signature2)
    {
    case 'K':
        if ( e->selected_formats[bink1_id] )
        {
            Q_EMIT e->txResourceFound("bik", "", base_index, resource_size, info);
        }
        else
        {
            return 0;
        }
        break;
    case '2':
        if ( e->selected_formats[bink2_id] )
        {
            Q_EMIT e->txResourceFound("bk2", "", base_index, resource_size, info);
        }
        else
        {
            return 0;
        }
    }
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_smk RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    // https://wiki.multimedia.cx/index.php/Smacker
    struct SMK_Header
    {
        u32i signature;
        u32i width, height;
        u32i frames_num, frame_rate, flags;
        u32i audio_size[7];
        u32i trees_size;
        u32i mmap_size, mclr_size, full_size, type_size;
        u32i audio_rate[7];
        u32i dummy;
    };
#pragma pack(pop)
    static const u64i min_room_need = sizeof(SMK_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (SMK_Header*)(&buffer[base_index]);
    if ( ( info_header->width == 0 ) or ( info_header->height == 0 ) ) return 0;
    if ( ( info_header->width > 6000 ) or ( info_header->height > 6000 ) ) return 0;
    s64i file_size = e->file_size;
    if ( base_index + sizeof(SMK_Header) + info_header->frames_num * 4 + info_header->frames_num * 1 > file_size ) return 0; // нет места для таблицы размеров кадров
    auto pointer = (u32i*)&buffer[base_index + sizeof(SMK_Header)];
    u64i total_frames_size = 0; // общий размер всех кадров
    for (u32i idx = 0; idx < info_header->frames_num; ++idx)
    {
        total_frames_size += (pointer[idx] & 0xFFFFFFFC); // согласно описанию, нужно обнулять два младших бита
    }
    u64i last_index = base_index + sizeof(SMK_Header) + info_header->frames_num * 4 + info_header->frames_num * 1 + info_header->trees_size + total_frames_size;
    if ( last_index > file_size ) return 0;
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;
    auto info = QString("%1x%2").arg(   QString::number(info_header->width),
                                        QString::number(info_header->height));
    Q_EMIT e->txResourceFound("smk", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

static const QMap<u32i, QString> TIFF_VALID_COMPRESSION {   {0x0001, "uncompressed"}, {0x0002, "CCITT Group 3"}, {0x0003, "CCITT T.4 bi-level"},
                                                            {0x0004, "CCITT T.6 bi-level"}, {0x0005, "LZW"}, {0x0006, "old-style JPEG"}, {0x0007, "JPEG"},
                                                            {0x0008, "Deflate/Adobe"}, {0x0009, "JBIG ITU-T T.85"}, {0x000A, "JBIG ITU-T T.43"},
                                                            {0x7FFE, "NeXT RLE 2-bit greyscale"}, {0x8003, "uncompressed"}, {0x8005, "PackBits"},
                                                            {0x8029, "ThunderScan RLE 4-bit"}, {0x807F, "RasterPadding CT/MP"}, {0x8080, "RLE for LW"},
                                                            {0x8081, "RLE for HC"}, {0x8082, "RLE for BL"}, {0x80B2, "Deflate PKZIP"}, {0x80B3, "Kodak DCS"},
                                                            {0x8765, "LibTIFF JBIG"}, {0x8798, "JPEG2000"}, {0x8799, "Nikon NEF"}, {0x879B, "JBIG2"},
                                                            {0x8847, "LERC"}, {0x884C, "Lossy non-YCbCr JPEG"}, {0xC350, "ZSTD"}, {0xC351, "WebP"}, {0xCD32, "JPEG XL"},
                                                            {0xFFFFFFFF, "unknown compression"} // фиктивный тип, в стандарте не существует
                                                       };

RECOGNIZE_FUNC_RETURN Engine::recognize_tif_ii RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct TIF_Header
    {
        u32i signature;
        u32i ifd_offset;
    };
    struct TIF_Tag
    {
        u16i tag_id;
        u16i data_type;
        u32i data_count;
        u32i data_offset;
    };
#pragma pack(pop)
    //qInfo() << " TIF_II recognizer called!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(TIF_Header);
    static const QMap<u16i, u8i> VALID_DATA_TYPE { {1, 1}, {2, 1}, {3, 2}, {4, 4}, {5, 8}, {6, 1}, {7, 1}, {8, 2}, {9, 4}, {10, 8}, {11, 4}, {12, 8} }; // ключ - тип данных тега, значение - множитель размера данных
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (TIF_Header*)(&buffer[base_index]);
    if ( info_header->ifd_offset < 8 ) return 0;
    s64i file_size = e->file_size;
    QMap<u32i, u32i> ifds_and_tags_db; // бд : ключ - смещение данных, значение - размер данных.
    u16i num_of_tags;
    TIF_Tag *tag_pointer;
    u64i last_index = base_index + info_header->ifd_offset; // ставим last_index на первый ifd.
    u64i result_tag_data_size;
    u32i next_ifd_offset = info_header->ifd_offset;
    s64i ifd_image_offset;
    s64i ifd_image_size;
    s64i ifd_strip_offsets_table; // смещение таблицы смещений на "отрезки" изображения
    s64i ifd_strip_counts_table; // смещение таблицы размеров "отрезков" изображения
    u32i ifd_strip_num; // количество элементов в талицах ifd_strip_offsets и ifd_strip_counts
    s64i ifd_exif_offset; // смещение exif ifd
    u64i total_tags_counter = 0;
    QSet<u16i> accumulated_tag_ids; // чтобы потом проанализировать есть ли в ресурсе изображение или только служебные теги
    u32i image_width = 0;
    bool image_width_derived = false;
    u32i image_height = 0;
    bool image_height_derived = false;
    u64i image_bps = 0;
    bool image_bps_derived = false;
    u64i image_spp = 0;
    bool image_spp_derived = false;
    u32i image_compression = 0;
    bool image_compression_derived = false;
    QString info;
    while(true) // задача пройти по всем IFD и тегам в них, накопив информацию в бд
    {
        ifd_image_offset = -1; // начальное -1 означает, что действительное значение ещё не найдено (потому что размер тела изобр. и его смещение хранятся в разных тегах)
        ifd_image_size = -1;  //
        ifd_strip_offsets_table = -1;
        ifd_strip_counts_table = -1;
        ifd_strip_num = 0;
        ifd_exif_offset = -1;
        //qInfo() << ": IFD at offset:" << last_index;
        if ( last_index + 6 > file_size ) return 0; // не хватает места на NumDirEntries (2) + NextIFDOffset (4)
        num_of_tags = *((u16i*)&buffer[last_index]); // место есть - считываем количество тегов
        if ( last_index + 6 + num_of_tags * sizeof(TIF_Tag) > file_size ) return 0; // а вот на сами теги места уже не хватает -> капитуляция
        //qInfo() << ": num_of_tags:" << num_of_tags;
        tag_pointer = (TIF_Tag*)&buffer[last_index + 2]; // ставим указатель на самый первый тег под индексом [0]
        if ( ifds_and_tags_db.contains(next_ifd_offset) ) break; // защита от зацикленных IFD (как в файле jpeg_exif_invalid_data_back_pointers.jpg)
        ifds_and_tags_db[next_ifd_offset] = 6 + num_of_tags * sizeof(TIF_Tag); // пихаем в бд сам IFD, т.к. он может быть расположен дальше данных какого-либо тега
        for (u16i tag_idx = 0; tag_idx < num_of_tags; ++tag_idx) // и идём по тегам
        {
            //qInfo() << "   pre tag_id:" << tag_pointer[tag_idx].tag_id << " pre data_type:" << tag_pointer[tag_idx].data_type << " pre tag data_offset:" << tag_pointer[tag_idx].data_offset;
            if ( !VALID_DATA_TYPE.contains(tag_pointer[tag_idx].data_type) ) return 0; // неизвестный тип данных
            result_tag_data_size = tag_pointer[tag_idx].data_count * VALID_DATA_TYPE[tag_pointer[tag_idx].data_type];
            if ( result_tag_data_size > 4 ) // если данные не вмещаются в 4 байта, значит data_offset означает действительное смещение в файле ресурса
            {
                if ( base_index + tag_pointer[tag_idx].data_offset + result_tag_data_size > file_size ) return 0; // данные тега не вмещаются в скан-файл
                if ( tag_pointer[tag_idx].data_offset >= 8 ) // если смещение данных тега < 8, значит у тега нет тела -> не имеет смысл добавлять его в бд, т.к. в бд уже есть ifd, содержащий этот тег
                {
                    ifds_and_tags_db[tag_pointer[tag_idx].data_offset] = result_tag_data_size;
                    if ( tag_pointer[tag_idx].tag_id == 273 ) // запоминаем смещение таблицы смещений на кусочки изображения
                    {
                        ifd_strip_num = tag_pointer[tag_idx].data_count;
                        ifd_strip_offsets_table = tag_pointer[tag_idx].data_offset;
                    }
                    if ( tag_pointer[tag_idx].tag_id == 279 ) // запоминаем смещение таблицы размеров кусочков изображения
                    {
                        ifd_strip_num = tag_pointer[tag_idx].data_count;
                        ifd_strip_counts_table = tag_pointer[tag_idx].data_offset;
                    }
                    if ( tag_pointer[tag_idx].tag_id == 258 ) // у этого тега длина данных может быть меньше 8 : например встречается 6
                    {
                        image_bps_derived = true;
                        image_bps = buffer[base_index + tag_pointer[tag_idx].data_offset]; // считываем только первый байт, т.к. в остальных обычно просто дубли
                    }
                }
            }
            else // иначе, если данные помещаются в 4 байта, то поле data_offset хранит не смещение, а значение тега (например для тега 273 в data_offset хранится начало изображения размером из тега data_offset 279)
            {
                if ( tag_pointer[tag_idx].tag_id == 273 ) ifd_image_offset = tag_pointer[tag_idx].data_offset;
                if ( tag_pointer[tag_idx].tag_id == 279 ) ifd_image_size = tag_pointer[tag_idx].data_offset;
                if ( tag_pointer[tag_idx].tag_id == 34665 ) ifd_exif_offset = tag_pointer[tag_idx].data_offset; // бывает встречается тег "EXIF IFD" - это смещение на таблицу exif-данных, которая имеет структуру обычного IFD

                if ( tag_pointer[tag_idx].tag_id == 256 ) { image_width_derived = true; image_width = tag_pointer[tag_idx].data_offset & (0xFFFFFFFF >> (8 * result_tag_data_size)); }
                if ( tag_pointer[tag_idx].tag_id == 257 ) { image_height_derived = true; image_height = tag_pointer[tag_idx].data_offset & (0xFFFFFFFF >> (8 * result_tag_data_size));}
                if ( tag_pointer[tag_idx].tag_id == 258 ) { image_bps_derived = true; image_bps = tag_pointer[tag_idx].data_offset & (0xFFFFFFFF >> (8 * result_tag_data_size));}
                if ( tag_pointer[tag_idx].tag_id == 259 ) { image_compression_derived = true; image_compression = tag_pointer[tag_idx].data_offset & (0xFFFFFFFF >> (8 * result_tag_data_size));}
                if ( tag_pointer[tag_idx].tag_id == 277 ) { image_spp_derived = true; image_spp = tag_pointer[tag_idx].data_offset & (0xFFFFFFFF >> (8 * result_tag_data_size));}
            }
            //qInfo() << "   tag#"<< tag_idx << " tag_id:" << tag_pointer[tag_idx].tag_id << " data_type:" << tag_pointer[tag_idx].data_type << " data_count:" << tag_pointer[tag_idx].data_count << " multiplier:" << VALID_DATA_TYPE[tag_pointer[tag_idx].data_type] << " data_offset:" <<  tag_pointer[tag_idx].data_offset << " result_data_size:" << result_tag_data_size;
            if ( image_width_derived and image_height_derived and image_bps_derived and image_compression_derived and image_spp_derived)
            {
                image_width_derived = false;
                image_height_derived = false;
                image_bps_derived = false;
                image_compression_derived = false;
                image_spp_derived = false;
                info +=  QString("%1x%2 %3-bpp (%4), ").arg(QString::number(image_width),
                                                            QString::number(image_height),
                                                            QString::number(image_spp * image_bps),
                                                            TIFF_VALID_COMPRESSION[image_compression]);
            }
            accumulated_tag_ids.insert(tag_pointer[tag_idx].tag_id);
            ++total_tags_counter; // если прошли все if'ы, значит тег легитимный
        }
        if ( ( ifd_image_offset > 0 ) and ( ifd_image_size > 0 ) ) ifds_and_tags_db[ifd_image_offset] = ifd_image_size; // если у тегов 273 и 279 количество данных data_count < 4;
        // в ином случае проверяем таблицы ifd_strip_offsets_table и ifd_strip_counts_table
        if ( ( ifd_strip_num > 0 ) and ( ifd_strip_counts_table > 0 ) ) // добавляем кусочки (strips) в бд
        {
            auto strip_offsets = (u32i*)&buffer[base_index + ifd_strip_offsets_table];
            auto strip_counts = (u32i*)&buffer[base_index + ifd_strip_counts_table];
            // qInfo() << "   image strips num:" << ifd_strip_num;
            for (u32i stp_idx = 0; stp_idx < ifd_strip_num; ++stp_idx)
            {
                //qInfo() << "  strip#" << stp_idx << " offset:" << strip_offsets[stp_idx] << " size:" << strip_counts[stp_idx];
                ifds_and_tags_db[strip_offsets[stp_idx]] = strip_counts[stp_idx];
            }
        }
        // тут проходим по структуре EXIF IFD, если она есть; обычно находится в конце файла и содержит ссылки ещё куда-то дальше, ближе к концу файла, поэтому лучше её проанализировать
        if ( ifd_exif_offset > 0 )
        {
            if ( base_index + ifd_exif_offset + 6 > file_size ) return 0; // не хватает места
            u16i num_of_tags = *((u16i*)&buffer[base_index + ifd_exif_offset]);
            if ( base_index + ifd_exif_offset + 6 + num_of_tags * sizeof(TIF_Tag) > file_size ) return 0; // не хватает места
            ifds_and_tags_db[ifd_exif_offset] = 6 + num_of_tags * sizeof(TIF_Tag); // добавляем exif ifd в бд
            auto tag_pointer = (TIF_Tag*)&buffer[base_index + ifd_exif_offset + 2];
            u64i result_tag_data_size;
            for (u16i tag_idx = 0; tag_idx < num_of_tags; ++tag_idx) // и идём по тегам
            {
                if ( !VALID_DATA_TYPE.contains(tag_pointer[tag_idx].data_type) ) return 0; // неизвестный тип данных
                result_tag_data_size = tag_pointer[tag_idx].data_count * VALID_DATA_TYPE[tag_pointer[tag_idx].data_type];
                // qInfo() << "   exif_tag#"<< tag_idx << " exif_tag_id:" << tag_pointer[tag_idx].tag_id << " data_type:" << tag_pointer[tag_idx].data_type << " data_count:" << tag_pointer[tag_idx].data_count
                //         << " multiplier:" << VALID_DATA_TYPE[tag_pointer[tag_idx].data_type] << " data_offset:" <<  tag_pointer[tag_idx].data_offset << " result_data_size:" << result_tag_data_size;
                if ( result_tag_data_size > 4 ) // если данные не вмещаются в 4 байта, значит data_offset означает действительное смещение в файле ресурса
                {
                    if ( base_index + tag_pointer[tag_idx].data_offset + result_tag_data_size > file_size ) return 0; // данные тега не вмещаются в скан-файл
                    if ( tag_pointer[tag_idx].data_offset >= 8 ) // если смещение данных тега < 8, значит у тега нет тела -> не имеет смысл добавлять его в бд, т.к. в бд уже есть ifd, содержащий этот тег
                    {
                        ifds_and_tags_db[tag_pointer[tag_idx].data_offset] = result_tag_data_size;
                    }
                }
            }
        }
        next_ifd_offset = *((u32i*)&buffer[last_index + 2 + num_of_tags * sizeof(TIF_Tag)]); // считываем смещение следующего IFD из конца предыдущего IFD
        //qInfo() << ": next_ifd_offset:" << next_ifd_offset;
        if ( next_ifd_offset == 0 ) break;
        last_index = base_index + next_ifd_offset;
    }
    // на выходе из цикла переменная last_index не будет иметь значения (она будет указывать на последний IFD), т.к. нужно проанализировать бд
    if ( total_tags_counter == 0 ) return 0; // файл без тегов не имеет смысла
    if ( ( ifd_exif_offset > 0 ) and ( total_tags_counter == 1 ) ) return 0; // файл с единственным тегом exif не имеет смысла (часто встречается встронным в jpg и png)
    if ( !accumulated_tag_ids.contains(273) and !accumulated_tag_ids.contains(279) ) return 0; // файл без изображения (strips) не имеет смысла
    if ( ifds_and_tags_db.count() == 0 ) return 0; // проверка на всякий случай
    last_index = base_index + ifds_and_tags_db.lastKey() + ifds_and_tags_db[ifds_and_tags_db.lastKey()];
    if ( last_index > file_size ) return 0;
    //qInfo() << "width:" << image_width << "; height:" << image_height << "; bps:" << image_bps << " compr:" << image_compression << "; spp:" << image_spp;
    if ( ( image_width == 0 ) or ( image_height == 0 ) ) return 0;
    if ( !TIFF_VALID_COMPRESSION.contains(image_compression)) image_compression = 0xFFFFFFFF;
    info.chop(2); // удаляем последние ", "
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound("tif", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_tif_mm RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct TIF_Header
    {
        u32i signature;
        u32i ifd_offset;
    };
    struct TIF_Tag
    {
        u16i tag_id;
        u16i data_type;
        u32i data_count;
        u32i data_offset;
    };
#pragma pack(pop)
    //qInfo() << " TIF_MM recognizer called!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(TIF_Header);
    static const QMap<u16i, u8i> VALID_DATA_TYPE { {1, 1}, {2, 1}, {3, 2}, {4, 4}, {5, 8}, {6, 1}, {7, 1}, {8, 2}, {9, 4}, {10, 8}, {11, 4}, {12, 8} }; // ключ - тип данных тега, значение - множитель размера данных
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (TIF_Header*)(&buffer[base_index]);
    if ( be2le(info_header->ifd_offset) < 8 ) return 0;
    s64i file_size = e->file_size;
    QMap<u32i, u32i> ifds_and_tags_db; // бд : ключ - смещение данных, значение - размер данных.
    u16i num_of_tags;
    TIF_Tag *tag_pointer;
    u32i next_ifd_offset = be2le(info_header->ifd_offset);
    u64i last_index = base_index + next_ifd_offset; // ставим last_index на первый ifd
    u64i result_tag_data_size;
    s64i ifd_image_offset;
    s64i ifd_image_size;
    s64i ifd_strip_offsets_table; // смещение таблицы смещений на "отрезки" изображения
    s64i ifd_strip_counts_table; // смещение таблицы размеров "отрезков" изображения
    u32i ifd_strip_num; // количество элементов в талицах ifd_strip_offsets и ifd_strip_counts
    s64i ifd_exif_offset; // смещение exif ifd
    u64i total_tags_counter = 0;
    QSet<u16i> accumulated_tag_ids; // чтобы потом проанализировать есть ли в ресурсе изображение или только служебные теги
    u32i image_width = 0;
    bool image_width_derived = false;
    u32i image_height = 0;
    bool image_height_derived = false;
    u64i image_bps = 0;
    bool image_bps_derived = false;
    u64i image_spp = 0;
    bool image_spp_derived = false;
    u32i image_compression = 0;
    bool image_compression_derived = false;
    QString info;
    while(true) // задача пройти по всем IFD и тегам в них, накопив информацию в бд
    {
        ifd_image_offset = -1; // начальное -1 означает, что действительное значение ещё не найдено (потому что размер тела изобр. и его смещение хранятся в разных тегах)
        ifd_image_size = -1;  //
        ifd_strip_offsets_table = -1;
        ifd_strip_counts_table = -1;
        ifd_strip_num = 0;
        ifd_exif_offset = -1;
        //qInfo() << ": IFD at offset:" << last_index;
        if ( last_index + 6 > file_size ) return 0; // не хватает места на NumDirEntries (2) + NextIFDOffset (4)
        num_of_tags = be2le(*((u16i*)&buffer[last_index])); // место есть - считываем количество тегов
        if ( last_index + 6 + num_of_tags * 12 > file_size ) return 0; // а вот на сами теги места уже не хватает -> капитуляция
        //qInfo() << ": num_of_tags:" << num_of_tags;
        tag_pointer = (TIF_Tag*)&buffer[last_index + 2]; // ставим указатель на самый первый тег под индексом [0]
        if ( ifds_and_tags_db.contains(next_ifd_offset) ) break; // защита от зацикленных IFD (как в файле jpeg_exif_invalid_data_back_pointers.jpg)
        ifds_and_tags_db[next_ifd_offset] = 6 + num_of_tags * sizeof(TIF_Tag); // пихаем в бд сам IFD, т.к. он может быть расположен дальше данных какого-либо тега
        for (u16i tag_idx = 0; tag_idx < num_of_tags; ++tag_idx) // и идём по тегам
        {
            //qInfo() << "\n   pre tag_id:" << be2le(tag_pointer[tag_idx].tag_id) << " pre data_type:" << be2le(tag_pointer[tag_idx].data_type) << " pre tag data_offset:" << be2le(tag_pointer[tag_idx].data_offset);
            if ( !VALID_DATA_TYPE.contains(be2le(tag_pointer[tag_idx].data_type)) ) return 0; // неизвестный тип данных
            result_tag_data_size = be2le(tag_pointer[tag_idx].data_count) * VALID_DATA_TYPE[be2le(tag_pointer[tag_idx].data_type)];
            //qInfo() << "   result_tag_data_size:" << result_tag_data_size;
            if ( result_tag_data_size > 4 ) // если данные не вмещаются в 4 байта, значит data_offset означает действительное смещение в файле ресурса
            {
                if ( base_index + be2le(tag_pointer[tag_idx].data_offset) + result_tag_data_size > file_size ) return 0; // данные тега не вмещаются в скан-файл
                if ( be2le(tag_pointer[tag_idx].data_offset) >= 8 ) // если смещение данных тега < 8, значит у тега нет тела -> не имеет смысл добавлять его в бд, т.к. в бд уже есть ifd, содержащий этот тег
                {
                    ifds_and_tags_db[be2le(tag_pointer[tag_idx].data_offset)] = result_tag_data_size;
                    if ( be2le(tag_pointer[tag_idx].tag_id) == 273 ) // запоминаем смещение таблицы смещений на кусочки изображения
                    {
                        ifd_strip_num = be2le(tag_pointer[tag_idx].data_count);
                        ifd_strip_offsets_table = be2le(tag_pointer[tag_idx].data_offset);
                    }
                    if ( be2le(tag_pointer[tag_idx].tag_id) == 279 ) // запоминаем смещение таблицы размеров кусочков изображения
                    {
                        ifd_strip_num = be2le(tag_pointer[tag_idx].data_count);
                        ifd_strip_counts_table = be2le(tag_pointer[tag_idx].data_offset);
                    }
                    if ( be2le(tag_pointer[tag_idx].tag_id) == 258 ) // у этого тега длина данных может быть меньше 8 : например встречается 6
                    {
                        image_bps_derived = true;
                        image_bps = buffer[base_index + be2le(tag_pointer[tag_idx].data_offset) + result_tag_data_size - 1]; // считываем только первый байт, т.к. в остальных обычно просто дубли
                    }
                }
            }
            else // иначе, если данные помещаются в 4 байта, то поле data_offset хранит не смещение, а значение тега (например для тега 273 в data_offset хранится начало изображения размером из тега data_offset 279)
            {
                // далее формула (8 * (4 - result_tag_data_size)) применяется, т.к. значащих байт в поле data_offset может быть меньше 4 => нужно из

                if ( be2le(tag_pointer[tag_idx].tag_id) == 273 ) ifd_image_offset = be2le(tag_pointer[tag_idx].data_offset) >> (8 * (4 - result_tag_data_size));
                if ( be2le(tag_pointer[tag_idx].tag_id) == 279 ) ifd_image_size = be2le(tag_pointer[tag_idx].data_offset) >> (8 * (4 - result_tag_data_size));

                // бывает встречается тег "EXIF IFD" - это смещение на таблицу exif-данных, которая имеет структуру обычного IFD
                if ( be2le(tag_pointer[tag_idx].tag_id) == 34665 ) ifd_exif_offset = be2le(tag_pointer[tag_idx].data_offset)  >> (8 * (4 - result_tag_data_size));

                if ( be2le(tag_pointer[tag_idx].tag_id) == 256 ) { image_width_derived = true; image_width = be2le(tag_pointer[tag_idx].data_offset) >> (8 * (4 - result_tag_data_size)); }
                if ( be2le(tag_pointer[tag_idx].tag_id) == 257 ) { image_height_derived = true; image_height = be2le(tag_pointer[tag_idx].data_offset) >> (8 * (4 - result_tag_data_size)); }
                if ( be2le(tag_pointer[tag_idx].tag_id) == 258 ) { image_bps_derived = true; image_bps = be2le(tag_pointer[tag_idx].data_offset) >> (8 * (4 - result_tag_data_size)); }
                if ( be2le(tag_pointer[tag_idx].tag_id) == 259 ) { image_compression_derived = true; image_compression = be2le(tag_pointer[tag_idx].data_offset) >> (8 * (4 - result_tag_data_size)); }
                if ( be2le(tag_pointer[tag_idx].tag_id) == 277 ) { image_spp_derived = true; image_spp = be2le(tag_pointer[tag_idx].data_offset) >> (8 * (4 - result_tag_data_size)); }
            }
            // qInfo() << "   tag#"<< tag_idx << " tag_id:" << be2le(tag_pointer[tag_idx].tag_id) << " data_type:" << be2le(tag_pointer[tag_idx].data_type)
            //          << " data_count:" << be2le(tag_pointer[tag_idx].data_count) << " multiplier:" << VALID_DATA_TYPE[be2le(tag_pointer[tag_idx].data_type)]
            //          << " data_offset:" <<  be2le(tag_pointer[tag_idx].data_offset) << " result_data_size:" << result_tag_data_size;
            if ( image_width_derived and image_height_derived and image_bps_derived and image_compression_derived and image_spp_derived)
            {
                image_width_derived = false;
                image_height_derived = false;
                image_bps_derived = false;
                image_compression_derived = false;
                image_spp_derived = false;
                info +=  QString("%1x%2 %3-bpp (%4), ").arg(QString::number(image_width),
                                                            QString::number(image_height),
                                                            QString::number(image_spp * image_bps),
                                                            TIFF_VALID_COMPRESSION[image_compression]);
            }
            accumulated_tag_ids.insert(be2le(tag_pointer[tag_idx].tag_id));
            ++total_tags_counter;
        }
        if ( ( ifd_image_offset > 0 ) and ( ifd_image_size > 0 ) ) ifds_and_tags_db[ifd_image_offset] = ifd_image_size; // если у тегов 273 и 279 количество данных data_count < 4;
        // в ином случае проверяем таблицы ifd_strip_offsets_table и ifd_strip_counts_table
        if ( ( ifd_strip_num > 0 ) and ( ifd_strip_counts_table > 0 ) ) // добавляем кусочки (strips) в бд
        {
            auto strip_offsets = (u32i*)&buffer[base_index + ifd_strip_offsets_table];
            auto strip_counts = (u32i*)&buffer[base_index + ifd_strip_counts_table];
            // qInfo() << "   image strips num:" << ifd_strip_num;
            for (u32i stp_idx = 0; stp_idx < ifd_strip_num; ++stp_idx)
            {
                //qInfo() << "  strip#" << stp_idx << " offset:" << strip_offsets[stp_idx] << " size:" << strip_counts[stp_idx];
                ifds_and_tags_db[be2le(strip_offsets[stp_idx])] = be2le(strip_counts[stp_idx]);
            }
        }
        // тут проходим по структуре EXIF IFD, если она есть; обычно находится в конце файла и содержит ссылки ещё куда-то дальше, ближе к концу файла, поэтому лучше её проанализировать
        if ( ifd_exif_offset > 0 )
        {
            //qInfo() << "exif ifd present at:" << ifd_exif_offset;
            if ( base_index + ifd_exif_offset + 6 > file_size ) return 0; // не хватает места
            u16i num_of_tags = be2le(*((u16i*)&buffer[base_index + ifd_exif_offset]));
            // qInfo() << "num of exif tags:" << num_of_tags;
            if ( base_index + ifd_exif_offset + 6 + num_of_tags * sizeof(TIF_Tag) > file_size ) return 0; // не хватает места
            ifds_and_tags_db[ifd_exif_offset] = 6 + num_of_tags * sizeof(TIF_Tag); // добавляем exif ifd в бд
            auto tag_pointer = (TIF_Tag*)&buffer[base_index + ifd_exif_offset + 2];
            u64i result_tag_data_size;
            for (u16i tag_idx = 0; tag_idx < num_of_tags; ++tag_idx) // и идём по тегам
            {
                if ( !VALID_DATA_TYPE.contains(be2le(tag_pointer[tag_idx].data_type)) ) return 0; // неизвестный тип данных
                result_tag_data_size = be2le(tag_pointer[tag_idx].data_count) * VALID_DATA_TYPE[be2le(tag_pointer[tag_idx].data_type)];
                // qInfo() << "   tag#"<< tag_idx << " tag_id:" << be2le(tag_pointer[tag_idx].tag_id) << " data_type:" << be2le(tag_pointer[tag_idx].data_type)
                //          << " data_count:" << be2le(tag_pointer[tag_idx].data_count) << " multiplier:" << VALID_DATA_TYPE[be2le(tag_pointer[tag_idx].data_type)]
                //          << " data_offset:" <<  be2le(tag_pointer[tag_idx].data_offset) << " result_data_size:" << result_tag_data_size;
                if ( result_tag_data_size > 4 ) // если данные не вмещаются в 4 байта, значит data_offset означает действительное смещение в файле ресурса
                {
                    if ( base_index + be2le(tag_pointer[tag_idx].data_offset) + result_tag_data_size > file_size ) return 0; // данные тега не вмещаются в скан-файл
                    if ( be2le(tag_pointer[tag_idx].data_offset) >= 8 ) // если смещение данных тега < 8, значит у тега нет тела -> не имеет смысл добавлять его в бд, т.к. в бд уже есть ifd, содержащий этот тег
                    {
                        ifds_and_tags_db[be2le(tag_pointer[tag_idx].data_offset)] = result_tag_data_size;
                    }
                }
            }
        }
        next_ifd_offset = be2le(*((u32i*)&buffer[last_index + 2 + num_of_tags * 12])); // считываем смещение следующего IFD из конца предыдущего IFD
        //qInfo() << "   next_ifd_offset:" << next_ifd_offset;
        if ( next_ifd_offset == 0 ) break;
        last_index = base_index + next_ifd_offset;
    }
    // на выходе из цикла переменная last_index не будет иметь значения (она будет указывать на последний IFD), т.к. нужно проанализировать бд
    if ( total_tags_counter == 0 ) return 0; // файл без тегов не имеет смысла
    if ( ( ifd_exif_offset > 0 ) and ( total_tags_counter == 1 ) ) return 0; // файл с единственным тегом exif не имеет смысла (часто встречается встронным в jpg и png)
    if ( !accumulated_tag_ids.contains(273) and !accumulated_tag_ids.contains(279) ) return 0; // файл без изображения (strips) не имеет смысла
    if ( ifds_and_tags_db.count() == 0 ) return 0; // проверка на всякий случай
    last_index = base_index + ifds_and_tags_db.lastKey() + ifds_and_tags_db[ifds_and_tags_db.lastKey()];
    if ( last_index > file_size ) return 0;
    //qInfo() << "width:" << image_width << "; height:" << image_height << "; bps:" << image_bps << " compr:" << image_compression << "; spp:" << image_spp;
    if ( ( image_width == 0 ) or ( image_height == 0 ) ) return 0;
    if ( !TIFF_VALID_COMPRESSION.contains(image_compression)) image_compression = 0xFFFFFFFF;
    info.chop(2); // удаляем последние ", "
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound("tif", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_flc RECOGNIZE_FUNC_HEADER
{ // https://www.compuphase.com/flic.htm
#pragma pack(push,1)
    struct FLC_Header
    {
        u32i file_size;
        u16i file_id;
        u16i frames_num;
        u16i width;
        u16i height;
        u16i pix_depth;
        u16i flags;
        u32i frame_delay;
        u16i reserved;
    };
#pragma pack(pop)
    static const u64i min_room_need = sizeof(FLC_Header) - sizeof(FLC_Header::file_size);
    static const QHash<u16i, QString> EXTENSION { { 0xAF11, "fli" }, { 0xAF12, "flc" }, {0xAF44, "flx"} };
    if ( e->scanbuf_offset < sizeof(FLC_Header::file_size) ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset - sizeof(FLC_Header::file_size);
    auto buffer = e->mmf_scanbuf;
    auto info_header = (FLC_Header*)(&buffer[base_index]);
    if ( ( info_header->pix_depth != 8 ) and ( info_header->pix_depth != 16 )) return 0; // формат FLX может использовать 16bit pix depth
    if ( ( info_header->width > 2000 ) or ( info_header->height > 2000 ) ) return 0; // вряд ли хранятся большие изображения, т.к. это старый формат
    if ( ( info_header->width == 0 ) or ( info_header->height == 0 ) ) return 0;
    if ( (info_header->width % 2) != 0 ) return 0;   // и вряд ли нечётный размер картинки является корректным
    if ( (info_header->height % 2) != 0 ) return 0;  //
    if ( ( info_header->flags != 0x03 ) and ( info_header->flags != 0x00 )) return 0;
    if ( info_header->file_size <= sizeof(FLC_Header) ) return 0; // сверхкороткие файлы нам не нужны
    u64i last_index = base_index + info_header->file_size;
    if ( last_index > e->file_size ) return 0;
    u64i resource_size = last_index - base_index;
    auto info = QString("%1x%2 %3-bpp").arg(  QString::number(info_header->width),
                                                        QString::number(info_header->height),
                                                        QString::number(info_header->pix_depth));
    Q_EMIT e->txResourceFound("flc", EXTENSION[info_header->file_id], base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_669 RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct _669_Header
    {
        u16i marker;
        u8i  song_name[108];
        u8i  smpnum, patnum, loop_order;
        u8i  order_list[128];
        u8i  tempo_list[128];
        u8i  break_list[128];
    };
    struct SampleHeader
    {
        u8i  sample_name[13];
        u32i sample_size, loop_begin, loop_end;
    };
    using Note = u8i[3];
    using Row = Note[8]; // т.к. 8 каналов
    using Pattern = Row[64];
#pragma pack(pop)
    //qInfo() << " !!! 669 RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(_669_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (_669_Header*)(&buffer[base_index]);
    if ( ( info_header->smpnum == 0 ) or ( info_header->patnum == 0 ) ) return 0; // жесткая проверка (т.к. в теории могут быть модули и без сэмплов и без паттернов), но вынужденная, иначе много ложных срабатываний
    if ( info_header->smpnum > 64 ) return 0; // согласно описанию
    if ( info_header->patnum > 128 ) return 0; // согласно описанию
    // for (u8i idx = 0; idx < sizeof(_669_Header::song_name); ++idx) // проверка на неподпустимые символы в названии песни
    // {
    //     //if ( ( info_header->song_name[idx] != 0 ) and ( info_header->song_name[idx] < 32 )) return 0;
    //     if ( ( info_header->song_name[idx] != 0 ) and ( info_header->song_name[idx] > 126 )) return 0;
    // }
    s64i file_size = e->file_size;
    if ( base_index + sizeof(_669_Header) + sizeof(SampleHeader) * info_header->smpnum + info_header->patnum * 1536 > file_size) return 0; // где 1536 - размер одного паттерна
    u64i last_index = base_index + sizeof(_669_Header); // переставляем last_index на первый заголовок сэмпла
    SampleHeader *sample_header;
    u64i samples_block = 0; // суммарный размер тел сэмплов
    for (u8i idx = 0; idx < info_header->smpnum; ++idx)
    {
        if ( file_size - last_index < sizeof(SampleHeader) ) return 0; // не хватает места для заголовка сэмпла
        sample_header = (SampleHeader*)&buffer[last_index];
        // qInfo() << " sample_id:" << idx << " sample_size:" << sample_header->sample_size << " loop_begin:" << sample_header->loop_begin << " loop end:" << sample_header->loop_end;
        // сигнатура 'if' очень часто встречается, поэтому формат CAT_PERFRISK.
        // нужны тщательные проверки :
        if ( sample_header->loop_begin > sample_header->sample_size ) return 0;
        if ( ( sample_header->loop_end > sample_header->sample_size ) and ( ( sample_header->loop_end != 0xFFFFF ) and ( sample_header->loop_end != 0xFFFFFFFF ) ) ) return 0;
        if ( sample_header->loop_begin > sample_header->loop_end ) return 0;
        // for (u8i sub_idx = 0; sub_idx < sizeof(SampleHeader::sample_name); ++sub_idx) // проверка на неподпустимые символы в названиях сэмплов
        // {
        //     if ( ( sample_header->sample_name[sub_idx] != 0 ) and ( sample_header->sample_name[sub_idx] < 32 )) return 0;
        //     if ( ( sample_header->sample_name[sub_idx] != 0 ) and ( sample_header->sample_name[sub_idx] > 126 )) return 0;
        // }
        samples_block += sample_header->sample_size;
        last_index += sizeof(SampleHeader); // переставляем last_index на следующий заголовок сэмпла (или начало паттернов, если завершение цикла)
    }
    // здесь last_index стоит в начале паттернов
    // qInfo() << "begin of patterns offset:" << last_index;
    // далее валидация нотных команд
    auto patterns = (Pattern*)&buffer[last_index];
    for(u8i pat_idx = 0; pat_idx < info_header->patnum; ++ pat_idx) // идём по паттернам
    {
        // qInfo() << "pat_idx:" << pat_idx << "; pat_offset:" << (uchar*)&patterns[pat_idx] - e->mmf_scanbuf;
        auto rows = &patterns[pat_idx][0];
        for(u8i row_idx = 0; row_idx < 64; ++row_idx) // идём по строкам
        {
            // qInfo() << "row_idx:" << row_idx << "; row_offset:" << (uchar*)&rows[row_idx] - e->mmf_scanbuf;
            auto notes = &rows[row_idx][0];
            for(u8i note_idx = 0; note_idx < 8; ++note_idx) // идём по каналам (нотам из каналов 0 - 7)
            {
                u8i instr_id = (notes[note_idx][1] >> 4) | ((notes[note_idx][0] << 4) & 0x00111111);
                u8i command = notes[note_idx][2] >> 4;
                // qInfo() << "note_idx:" << note_idx << "; note_offset:" << (uchar*)&patterns[pat_idx][row_idx] - e->mmf_scanbuf << "; byte0:" << notes[note_idx][0]
                //         << "; byte1:" << notes[note_idx][1] << "; byte2:" << notes[note_idx][2] << "; instr_num:" << instr_id << "; command:" << command;
                if ( ( command > 7 ) and ( notes[note_idx][2] != 0xFF ) ) return 0; // некорректная команда
                if ( ( instr_id > info_header->smpnum ) and (instr_id != 31 ) ) return 0;
            }
        }
    }
    // qInfo() << " summary sample bodies size:" << samples_block;
    last_index += (info_header->patnum * 1536 + samples_block);
    // qInfo() << "last_index:" << last_index;
    if ( last_index > file_size ) return 0;
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 32; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [31]
    {
        if ( info_header->song_name[song_name_len] == 0 ) break;
    }
    auto info = QString("song '%1'").arg(QString(QByteArray((char*)(info_header->song_name), song_name_len)));
    Q_EMIT e->txResourceFound("669", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_au RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct AU_Header
    {
        u32i magic_number;
        u32i data_offset;
        u32i data_size;
        u32i encoding;
        u32i sample_rate;
        u32i channels;
    };
#pragma pack(pop)
    //qInfo() << " !!! AU RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(AU_Header);
    static const QSet <u32i> VALID_SAMPLE_RATE { 5500, 7333, 8000, 8012, 8013, 8192, 8363, 11025, 16000, 22050, 32000, 44056, 44100, 48000 };
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (AU_Header*)(&buffer[base_index]);
    if ( be2le(info_header->data_offset) < sizeof(AU_Header) ) return 0;
    //qInfo() << "encoding:" << be2le(info_header->encoding) << " sample_rate:" << be2le(info_header->sample_rate) << " channels:" << be2le(info_header->channels);
    if ( be2le(info_header->encoding) > 27 ) return 0;
    if ( !VALID_SAMPLE_RATE.contains(be2le(info_header->sample_rate)) ) return 0;
    if ( ( be2le(info_header->channels) < 1 ) or ( be2le(info_header->channels) > 2 ) ) return 0;
    if ( info_header->data_size == 0xFFFFFFFF ) return 0; // судя по докам, значение 0xFFFFFFFF в случае неопределённого размера данных - нам такое не подходит, т.к. невозможно определить размер ресурса
    u64i last_index = base_index + be2le(info_header->data_offset) + be2le(info_header->data_size);
    if ( last_index > e->file_size ) return 0;
    u64i resource_size = last_index - base_index;
    auto info = QString("%1Hz %2-ch").arg(QString::number(be2le(info_header->sample_rate)),
                                                    QString::number(be2le(info_header->channels)));
    Q_EMIT e->txResourceFound("au", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_voc RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct VOC_Header
    {
        u64i signature1; // 'Creative'
        u64i signature2; // ' Voice F'
        u32i signature3; // 'ile\x1A'
        u16i header_size;
        u16i version;
        u16i checksum;
    };
    struct DataBlock
    {
        u32i type_with_size;
    };
    struct SoundDataBlock1
    {
        u8i req_divisor;
        u8i compression_type;
    };
    struct SoundDataBlock9
    {
        u32i sample_rate;
        u8i  bits_per_sample;
        u8i  channels;
        u16i wformat;
        u8i  reserved[4];
    };
#pragma pack(pop)
    //qInfo() << " !!! VOC RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(VOC_Header);
    static const QSet<u16i> VALID_VERSION { 0x0100, 0x010A, 0x0114 };
    static const QMap<u8i, QString> VALID_COMPRESSION { {0x00,   "8-bits"},
                                                        {0x01,   "4-bits"},
                                                        {0x02,   "2.6-bits"},
                                                        {0x03,   "2-bits"},
                                                        {0x04,   "Multi DAC"} };
    static const QMap<u16i, QString> VALID_WFORMAT {{0x00,   "unsigned PCM"},
                                                    {0x01,   "Creative 8-bit to 4-bit ADPCM"},
                                                    {0x02,   "Creative 8-bit to 3-bit ADPCM"},
                                                    {0x03,   "Creative 8-bit to 2-bit ADPCM"},
                                                    {0x04,   "signed PCM"},
                                                    {0x06,   "CCITT a-Law"},
                                                    {0x07,   "CCITT u-Law"},
                                                    {0x2000, "Creative 16-bit to 4-bit ADPCM"} };
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (VOC_Header*)(&buffer[base_index]);
    if ( info_header->signature1 != 0x6576697461657243 ) return 0;
    if ( info_header->signature2 != 0x46206563696F5620) return 0;
    if ( info_header->signature3 != 0x1A656C69 ) return 0;
    if ( info_header->header_size < sizeof(VOC_Header) ) return 0;
    if ( !VALID_VERSION.contains(info_header->version) ) return 0;
    u64i last_index = base_index + info_header->header_size; // переставляемся на DataBlock
    s64i file_size = e->file_size;
    if ( last_index >= file_size ) return 0; // не осталось места на data block'и
    DataBlock *data_block;
    bool info_derived = false; // флаг "информация получена", по первому блоку типа 1 или 9
    QString info;
    while(true)
    {
        if ( last_index + 1 > file_size) return 0; // нет места на тип блока
        if ( buffer[last_index] == 0 ) break; // достигли терминатора
        if ( last_index + sizeof(DataBlock) > file_size ) break; // нет места на очередной data block -> вероятно достигли конца
        data_block = (DataBlock*)&buffer[last_index];
        if ( ( data_block->type_with_size & 0xFF ) > 9 ) return 0; // неизвестный тип блока
        //qInfo() << " data_block type:" << (data_block->type_with_size & 0xFF);
        if ( ( (data_block->type_with_size & 0xFF) == 1 ) and !info_derived ) // считываем инфо по первому блоку типа 1, в остальные такие же блоки больше не смотрим
        {
            if ( last_index + sizeof(DataBlock) + sizeof(SoundDataBlock1) <= file_size ) // хватает ли места на SoundDataBlock1?
            {
                auto sound_data = (SoundDataBlock1 *)&buffer[last_index + sizeof(DataBlock)];
                u32i sample_rate = 1'000'000  / (256 - sound_data->req_divisor);
                u8i channels = 1;
                u8i compression_type = sound_data->compression_type;
                if (compression_type > 0x03 )
                {
                    channels = compression_type - 3;
                    compression_type = 0x04;
                }
                info = QString("%1 codec : %2Hz %3-ch").arg(  VALID_COMPRESSION[compression_type],
                                                                        QString::number(sample_rate),
                                                                        QString::number(channels));
                info_derived = true;
            }
        }
        if ( ( (data_block->type_with_size & 0xFF) == 9 ) and !info_derived ) // считываем инфо по первому блоку типа 9, в остальные такие же блоки больше не смотрим
        {
            if ( last_index + sizeof(DataBlock) + sizeof(SoundDataBlock9) <= file_size ) // хватает ли места на SoundDataBlock9?
            {
                auto sound_data = (SoundDataBlock9 *)&buffer[last_index + sizeof(DataBlock)];
                QString codec;
                if ( !VALID_WFORMAT.contains(sound_data->wformat) )
                {
                    codec = "Unknown";
                }
                else
                {
                    codec = VALID_WFORMAT[sound_data->wformat];
                }
                info = QString("%1 codec : %2Hz %3-ch %4-bit").arg(   codec,
                                                                                QString::number(sound_data->sample_rate),
                                                                                QString::number(sound_data->channels),
                                                                                QString::number(sound_data->bits_per_sample));
                info_derived = true;
            }
        }
        last_index += ((data_block->type_with_size >> 8) + sizeof(DataBlock)); // сдвигаем вправо на 8, чтобы затереть type и оставить только size
    }
    last_index += 1; // на размер терминатора
    if ( last_index > file_size) return 0;
    if ( !info_derived ) return 0; // если информация о сэмпле не получнеа, значит не было ни одного блока типа 1 или 9 - без них ресурс вообще не имеет смысла
    u64i resource_size = last_index - base_index;
    if ( resource_size == (sizeof(VOC_Header) + 1) ) return 0; // короткий ресурс без аудио-данных не имеет смысла
    Q_EMIT e->txResourceFound("voc", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_mov_qt RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct AtomHeader
    {
        u32i size;
        u32i type;
    };
    struct MP4_FtypHeader // size 28
    {
        u64i signature1; // '....ftyp'
        u32i signature2; // 'mp42'
        u32i signature2_data;
        u32i signature3; // 'isom'
        u32i signature3_data;
        u32i signature4; // 'mp42'
    };
    struct MOV_FtypHeader_32 // size 32
    {
        u64i signature1;
        u64i signature2;
        u64i signature3;
        u32i signature4;
    };
    struct MOV_FtypHeader_40 // size 40
    {
        u64i signature1;
        u64i signature2;
        u64i signature3;
        u64i signature4;
        u64i signature5;
    };
#pragma pack(pop)
    //qInfo() << " !!! QT_MOV RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static u32i mov_qt_id {fformats["mov_qt"].index};
    static u32i mp4_qt_id {fformats["mp4_qt"].index};
    static const QSet <u32i> VALID_ATOM_TYPE { 0x7461646d /*mdat*/, 0x766F6F6D /*moov*/, 0x65646977 /*wide*/, 0x65657266 /*free*/, 0x70696B73 /*skip*/, 0x746F6E70 /*pnot*/};
    if ( !e->selected_formats[mov_qt_id] and !e->selected_formats[mp4_qt_id] ) return 0;
    u64i base_index = e->scanbuf_offset;
    if ( base_index < sizeof(AtomHeader::size) ) return 0; // нет места на поле size
    base_index -= sizeof(AtomHeader::size);
    u64i last_index = base_index; // ставим на самый первый atom
    auto buffer = e->mmf_scanbuf;
    bool mp4_flag = false;
    bool was_mdat = false; // встречался ли атом mdat?
    bool was_moov = false; // встречался ли атом moov?
    if ( e->selected_formats[mp4_qt_id] and ( base_index >= sizeof(MP4_FtypHeader) ) ) // может это mp4 с ftyp-заголовком, а не обычный mov ?
    {
        auto ftyp_header = (MP4_FtypHeader*)&buffer[base_index - sizeof(MP4_FtypHeader)];
        if (( ftyp_header->signature1 == 0x707974661C000000 ) and
            ( ftyp_header->signature2 == 0x3234706D ) and
            ( ftyp_header->signature3 == 0x6D6F7369 ) and
            ( ftyp_header->signature4 == 0x3234706D ) )
        {
            //qInfo() << " MP4 FTYP HEADER PRESENTS";
            base_index -= sizeof(MP4_FtypHeader);
            mp4_flag = true;
        }
    }
    if ( !mp4_flag and ( base_index >= sizeof(MOV_FtypHeader_32) ) ) // а может у mov тоже есть ftyp-заголовок?
    {
        auto ftyp_header = (MOV_FtypHeader_32*)&buffer[base_index - sizeof(MOV_FtypHeader_32)];
        if (( ftyp_header->signature1 == 0x70797466'14'000000 ) and
            ( ftyp_header->signature2 == 0x00'00'00'00'20207471 ) and
            ( ftyp_header->signature3 == 0x08'00000020207471 ) and
            ( ftyp_header->signature4 == 0x65646977 ) )
        {
            //qInfo() << " MOV FTYP_32 HEADER PRESENTS";
            base_index -= sizeof(MOV_FtypHeader_32);
        }
    }
    if ( !mp4_flag and ( base_index >= sizeof(MOV_FtypHeader_40) ) ) // а может у mov тоже есть ftyp-заголовок?
    {
        auto ftyp_header = (MOV_FtypHeader_40*)&buffer[base_index - sizeof(MOV_FtypHeader_40)];
        if (( ftyp_header->signature1 == 0x70797466'20'000000 ) and
            ( ftyp_header->signature2 == 0x00'03'05'20'20207471 ) and
            ( ftyp_header->signature3 == 0x00'00000020207471 ) and
            ( ftyp_header->signature4 == 0x0000000000000000 ) and
            ( ftyp_header->signature5 == 0x6564697708000000 ))
        {
            //qInfo() << " MOV FTYP_40 HEADER PRESENTS";
            base_index -= sizeof(MOV_FtypHeader_40);
        }
    }
    AtomHeader *atom_header;
    s64i file_size = e->file_size;
    while(true)
    {
        if ( last_index + sizeof(AtomHeader) >= file_size ) break; // нет места на заголовок очередного атома -> достигли конца ресурса
        atom_header = (AtomHeader*)&buffer[last_index];
        if ( !VALID_ATOM_TYPE.contains(atom_header->type) ) break; // неизвестный атом -> далее не сканируем
        if ( atom_header->type == 0x7461646d ) was_mdat = true;
        if ( atom_header->type == 0x766F6F6D ) was_moov = true;
        if ( ( be2le(atom_header->size) == 1 ) and ( atom_header->type == 0x7461646d /*mdat*/) ) // значит размер в виде 8 байт идёт после поля type, а не перед ним
        {
            //qInfo() << "\n :::::::::::;;;;;;;;;;;;; ------> mdat with size==1 !!!!\n";
            if ( last_index + sizeof(AtomHeader) + sizeof(u64i) <= file_size ) // а место-то есть для extended size?
            {
                last_index += be2le(*((u64i*)&buffer[last_index + sizeof(AtomHeader)]));
            }
            else
            {
                break;
            }
        }
        else
        {
            if ( be2le(atom_header->size) < sizeof(AtomHeader) ) break;
            last_index += be2le(atom_header->size);
        }
        // qInfo() << " last atom_type:" << QString::number(atom_header->type, 16);
        // qInfo() << " last_index:" << last_index;
        if ( last_index > file_size ) return 0;
    }
    // в этом месте last_index уже должен стоять в конце ресурса
    u64i resource_size = last_index - base_index;
    if ( resource_size < 48 ) return 0; // 48 - просто импирическая величина; вряд ли ресурс с таким размером является корректным
    //qInfo() << " was_moov:" << was_moov << " was_mdat:" << was_mdat << " last_index:" << last_index;
    if ( !was_moov or !was_mdat ) return 0; // ресурс имеет смысл только при наличии обоих атомов
    Q_EMIT e->txResourceFound(mp4_flag ? "mp4_qt" : "mov_qt", "", base_index, resource_size, "");
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_tga RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct TGA_Header
    {
        u8i  id_len, color_map_type, image_type;
        u16i cmap_start, cmap_len;
        u8i  cmap_depth;
        u16i x_offset, y_offset;
        u16i width, height;
        u8i  pix_depth;
        u8i  image_descriptor;
    };
#pragma pack(pop)
    //qInfo() << " !!! TGA RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static u32i tga_id {fformats["tga_tc"].index};
    static const u64i min_room_need = sizeof(TGA_Header);
    static const QSet<u8i> VALID_PIX_DEPTH { 16, 24, 32 };
    if ( !e->selected_formats[tga_id] ) return recognize_ico_cur(e); // если не tga, так может cur ?
    if ( !e->enough_room_to_continue(min_room_need) ) return recognize_ico_cur(e);
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (TGA_Header*)(&buffer[base_index]);
    if ( info_header->cmap_start != 0 ) return recognize_ico_cur(e);
    if ( info_header->cmap_len != 0 ) return recognize_ico_cur(e);
    if ( info_header->cmap_depth != 0 ) return recognize_ico_cur(e);
    if ( ( info_header->width < 1) or ( info_header->width >= 4096 ) ) return recognize_ico_cur(e);
    if ( info_header->height < 1) return recognize_ico_cur(e);
    if ( info_header->height / info_header->width > 5 ) return recognize_ico_cur(e); // предположение, что такие файлы маловероятны
    if ( info_header->width / info_header->height > 5 ) return recognize_ico_cur(e); //
    if ( info_header->height < 1 ) return recognize_ico_cur(e);
    if ( !VALID_PIX_DEPTH.contains(info_header->pix_depth) ) return recognize_ico_cur(e);
    if ( (info_header->image_descriptor >> 6) != 0 ) return recognize_ico_cur(e);
    u64i last_index = base_index + sizeof(TGA_Header) + info_header->width * info_header->height * (info_header->pix_depth / 8); // вычисление эмпирического размера
    last_index += 4096; // накидываем ещё 4K : вдруг это TGAv2, у которого есть области расширения и разработчика.
    if ( last_index > e->file_size ) last_index = e->file_size;
    u64i resource_size = last_index - base_index;
    // прошли все этапы проверки, осталось попытаться декодировать данные в надежде найти ошибку
    e->tga_decoder.init(&buffer[base_index], resource_size);
    auto last_err = e->tga_decoder.validate_header();
    if ( last_err == TgaErr::InvalidHeader ) {/*qInfo() << "invalid header";*/ return recognize_ico_cur(e);}
    auto tga_info = e->tga_decoder.info();
    if ( tga_info.total_size > 50 * 1024 * 1024 ) return recognize_ico_cur(e); // ограничение на размер декодированных данных 50 МиБайт
    last_err = e->tga_decoder.decode_checkup();
    if ( ( last_err == TgaErr::TooMuchPixAbort ) or ( last_err == TgaErr::TruncDataAbort ) )
    {
        //qInfo() << "invalid pixel data";
        return recognize_ico_cur(e);
    }
    auto info = QString("%1x%2 %3-bpp").arg(  QString::number(info_header->width),
                                                        QString::number(info_header->height),
                                                        QString::number(info_header->pix_depth));
    Q_EMIT e->txResourceFound("tga_tc", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_ico_cur RECOGNIZE_FUNC_HEADER
{ // https://docs.fileformat.com/image/ico/
#pragma pack(push,1)
    struct ICO_Header // одинаков для ico и cur
    {
        u32i signature;
        u16i count;
    };
    struct Directory // одинаков для ico и cur
    {
        u8i  width, height, color_count, reserved;
        u16i cpl_xhotspot, bpp_yhotspot;
        u32i bitmap_size, bitmap_offset;
    };
    struct BitmapInfoHeader
    {
        u32i bitmapheader_size; // 12, 40, 108
        s32i width;
        s32i height;
        u16i planes;
        u16i bits_per_pixel; // 1, 4, 8, 16, 24, 32
    };
#pragma pack(pop)
    //qInfo() << " !!! ICO/CUR RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static u32i ico_id {fformats["ico_win"].index};
    static u32i cur_id {fformats["cur_win"].index};
    static const u64i min_room_need = sizeof(ICO_Header);
    static const QSet <u32i> VALID_BMP_HEADER_SIZE { 12, 40, 108 };
    static const QSet <u16i> VALID_BITS_PER_PIXEL { 1, 4, 8, 16, 24, 32 };
    if ( !e->selected_formats[ico_id] and !e->selected_formats[cur_id] ) return 0;
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (ICO_Header*)(&buffer[base_index]);
    if ( info_header->count < 1 ) return 0;
    bool is_cur = (info_header->signature == 0x00020000 );
    //qInfo() << "is_cur:" << is_cur;
    if ( is_cur and !e->selected_formats[cur_id] ) return 0;
    if ( !is_cur and !e->selected_formats[ico_id] ) return 0;
    u64i last_index = base_index + sizeof(ICO_Header); // last_index на начало директорий изображений
    //qInfo() << "directory starts at:" << last_index;
    s64i file_size = e->file_size;
    Directory *directory;
    QMap<u32i, u32i> db; // бд тел битмапов: ключ - смещение, значение - размер
    //qInfo() << "count:" << info_header->count;
    QString info;
    for (u16i idx = 0; idx < info_header->count; ++idx)
    {
        if ( last_index + sizeof(Directory) > file_size) return 0; // не хватает места на Directory
        //qInfo() << "idx:" << idx;
        directory = (Directory*)&buffer[last_index];
        if ( directory->reserved != 0 ) return 0;
        if ( ( !is_cur ) and ( directory->cpl_xhotspot > 1 ) ) return 0;
        db[directory->bitmap_offset] = directory->bitmap_size;
        //qInfo() << "bitmap_offset:" << base_index + directory->bitmap_offset << " bitmap_size:" << directory->bitmap_size;
        if ( base_index + directory->bitmap_offset + sizeof(BitmapInfoHeader) > file_size ) return 0; // нет места на bitmap-заголовок
        auto bitmap_ih = (BitmapInfoHeader*)&buffer[base_index + directory->bitmap_offset];
        if ( !VALID_BMP_HEADER_SIZE.contains(bitmap_ih->bitmapheader_size) ) // тогда может быть есть встроенный PNG?
        {
            if ( bitmap_ih->bitmapheader_size != 0x474E5089 ) return 0;
            //qInfo() << " ----> embedded png here!!!!";
            if ( idx <= 4 ) // 5 изображений
            {
                info += QString("%1:png, ").arg(  QString::number(idx + 1));
            }
        }
        else
        {
            if ( !VALID_BITS_PER_PIXEL.contains(bitmap_ih->bits_per_pixel) ) return 0;
            if ( bitmap_ih->planes != 1 ) return 0;
            if ( idx <= 4 ) // 5 изображений
            {
                info += QString("%1:%2x%3, ").arg(  QString::number(idx + 1),
                                                    QString::number(bitmap_ih->width),
                                                    QString::number(bitmap_ih->height));
            }
        }
        if ( idx == 5 ) // на 6-м изображении добавляем только многоточие
        {
            info += "...  ";
        } // на изображениях более 6 ничего не делаем
        last_index += sizeof(Directory);
    }
    //qInfo() << "most far offset:" << db.lastKey() << " bitmap size:" << db[db.lastKey()];
    if ( db.lastKey() < sizeof(ICO_Header) + sizeof(Directory) ) return 0;
    last_index = base_index + db.lastKey() + db[db.lastKey()];
    if ( last_index > file_size ) return 0;
    u64i resource_size = last_index - base_index;
    info.chop(2); // удаляем последнюю запятую с пробелом
    Q_EMIT e->txResourceFound(is_cur ? "cur_win" : "ico_win", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_mp3 RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct FrameHeader
    {
        u8i AAAAAAAA, AAABBCCD, EEEEFFGH, IIJJKLMM;
    };
    // https://id3.org/ID3v1
    struct ID3v1 // вставляется в конце ресурса
    {
        u16i signature1; // 'TA'
        u8i  signature2; // 'G'
        u8i  title[30];
        u8i  artist[30];
        u8i  album[30];
        u8i  year[4];
        u8i  comment[30];
        u8i  genre[1];
    };
    // https://id3.org/d3v2.3.0
    struct ID3v2 // вставляется в начале ресурса
    {
        u16i signature1; // 'ID'
        u8i  signature2; // '3'
        u8i  ver_major; // 0x02 - 0x04
        u8i  ver_minor;
        u8i  flags;
        union
        {
            u8i  size_as_bytes[4];
            u32i size_as_dword;
        };
    };
    // https://id3.org/Lyrics3v2
    struct LyricsField // вставляется после фреймов, но перед ID3v1
    {
        union
        {
            struct
            {
                u8i id[3];
                u8i size[5];
            } as_bytes;
            u64i as_qword;
        };
    };
    struct LyricsEnding
    {
        u8i size[6];
        u64i lyrics200_sign1; // 'LYRICS20'
        u8i  lyrics200_sign2;  // '0'
    };
#pragma pack(pop)
    //qInfo() << " !!! MP3 RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const QMap <u8i, u64i> VALID_BIT_RATE {
                                                    {0b0001, 32000 }, {0b0010, 40000 }, {0b0011, 48000 }, {0b0100, 56000 },
                                                    {0b0101, 64000 }, {0b0110, 80000 }, {0b0111, 96000 }, {0b1000, 112000},
                                                    {0b1001, 128000}, {0b1010, 160000}, {0b1011, 192000}, {0b1100, 224000},
                                                    {0b1101, 256000}, {0b1110, 320000}
                                                 };
    static const QMap <u8i, u64i> VALID_SAMPLE_RATE { {0b00, 44100}, {0b01, 48000}, {0b10, 32000} };
    static const QSet <u64i> VALID_LYRICS_FIELD { 0x444E49 /*IND*/, 0x52594C /*LYR*/, 0x464E49 /*INF*/, 0x545541 /*AUT*/, 0x4C4145 /*EAL*/, 0x524145 /*EAR*/, 0x545445 /*ETT*/, 0x474D49 /*IMG*/ };
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    u64i last_index = base_index;
    s64i file_size = e->file_size;
    while(true)
    {
        if ( (*(u16i*)&buffer[last_index]) == 0x44'49 /*ID*/ ) // зашли сюда, потому что движок нашёл сигнатуру ID3v2 ?
        {
            if ( file_size - last_index < sizeof(ID3v2) ) return 0; // нет места для заголовка ID3v2
            //qInfo() << "has id3v2!!!";
            auto id3v2 = (ID3v2*)&buffer[last_index];
            if ( id3v2->ver_major > 4 ) return 0;
            if ( id3v2->ver_minor == 0xFF ) return 0;
            if ( (id3v2->flags & 0b00011111) != 0 ) return 0;
            if ( id3v2->size_as_bytes[0] > 0x80 ) return 0;
            if ( id3v2->size_as_bytes[1] > 0x80 ) return 0;
            if ( id3v2->size_as_bytes[2] > 0x80 ) return 0;
            if ( id3v2->size_as_bytes[3] > 0x80 ) return 0;
            u64i id3v2_size = u64i(id3v2->size_as_bytes[3]) + (u64i(id3v2->size_as_bytes[2]) << 7) + (u64i(id3v2->size_as_bytes[1]) << 14) + (u64i(id3v2->size_as_bytes[0]) << 21) + sizeof(ID3v2);
            //qInfo() << "id3v2_size:" << id3v2_size;
            if ( file_size - last_index < id3v2_size ) return 0; // нет места для всего ID3v2
            last_index += id3v2_size; // переставляемся сразу после ID3v2
        }
        else
        {
            break;
        }
        if ( file_size - last_index < 4 ) return 0; // нет места на следующий ID3v2 или заголовок-фрейма
    }
    // в этом месте last_index стоит на заголовке первого фрейма
    FrameHeader *frame_header;
    u64i bit_rate;
    u64i sample_rate;
    u8i  padding;
    u8i  crc_len;
    u64i frame_number = 0;
    u64i avg_bit_rate = 0;
    u64i avg_sample_rate = 0;
    while(true)
    {
        ++frame_number;
        if ( last_index + sizeof(FrameHeader) > file_size ) break; // достигли конца сканируемого файла
        frame_header = (FrameHeader*)(&buffer[last_index]);
        if ( (frame_header->AAABBCCD & 0b11111110) != 0xFA ) break; // достигли конца потока
        bit_rate = frame_header->EEEEFFGH >> 4;
        sample_rate = (frame_header->EEEEFFGH >> 2) & 0b00000011;
        if ( !VALID_BIT_RATE.contains(bit_rate) ) break; // достигли конца потока
        if ( !VALID_SAMPLE_RATE.contains(sample_rate) ) break; // достигли конца потока
        bit_rate = VALID_BIT_RATE[bit_rate]; // переиспользуем переменную bit_rate
        sample_rate = VALID_SAMPLE_RATE[sample_rate]; // переиспользуем переменную sample_rate
        avg_bit_rate += bit_rate;
        avg_sample_rate += sample_rate;
        padding = (frame_header->EEEEFFGH >> 1) & 0b00000001;
        crc_len = 2 * (frame_header->AAABBCCD & 0b00000001);
        //qInfo() << " sample_rate:" << sample_rate << " bit_rate:" << bit_rate << " padding:" << padding << " crc_len:" << crc_len << " last_index:" << last_index << " frame_size:" << ( (1152 * bit_rate / 8) / sample_rate + padding );
        last_index += ( (1152 * bit_rate / 8) / sample_rate + padding );
    }
    // здесь переменная frame_number на единицу больше реального количества фреймов
    //
    if ( frame_number > 1 ) // проверка, чтобы не поделить на нуль
    {
        avg_bit_rate /= (frame_number - 1);    // высчитываем среднее значения для info
        avg_sample_rate /= (frame_number - 1); //
        //qInfo() << "avg_bit_rate:" << avg_bit_rate << "; avg_sample_rate:" << avg_sample_rate;
    }
    if ( last_index > file_size ) last_index = file_size;
    u64i resource_size = last_index - base_index;
    if ( resource_size <= 4 ) return 0;
    if ( frame_number <= 4 ) return 0; // попытка ограничить ложные срабатывания количеством 3 фреймов
    if ( file_size - last_index >= 11 /*len('LYRICSBEGIN')*/ ) // может после звука и перед ID3v1 вставлен Lyrics?
    {
        if ( ( (*((u64i*)&buffer[last_index])) == 0x454253434952594C /*LYRICSBE*/) and ( (*((u16i*)&buffer[last_index+8])) == 0x4947 /*GI*/) and ( buffer[last_index+10] == 0x4E /*N*/))
        {
            //qInfo() << " Lyrics tag here:" << last_index;
            u64i last_index_ = last_index; // запоминаем last_index, чтобы его восстановить, если окажется, что Lyrics некорректный
            last_index += 11; // переставляемся на первое Lyrics-поле
            LyricsField *lyrics_field;
            while (true)
            {
                if ( file_size - last_index >= sizeof(LyricsField) )
                {
                    lyrics_field = (LyricsField*)&buffer[last_index];
                    if ( VALID_LYRICS_FIELD.contains(lyrics_field->as_qword & 0xFFFFFF) )
                    {
                        //qInfo() << "contains ok at:" << last_index;
                        u32i lyrics_size = 0;
                        for (u8i idx = 0; idx < sizeof(LyricsField::as_bytes.size); ++idx) // проверка на допустимость символов
                        {
                            if ( ( lyrics_field->as_bytes.size[idx] < 0x30 /*'0'*/) or ( lyrics_field->as_bytes.size[idx] > 0x39 /*'9'*/) ) // возврат из Lyrics, если неверные символы
                            {
                                last_index = last_index_;
                                goto mp3_id3v1;
                            }
                            lyrics_size += ( u32i(lyrics_field->as_bytes.size[idx] - 0x30) * qPow(10, sizeof(LyricsField::as_bytes.size) - idx - 1) );
                        }
                        //qInfo() << "symbols ok and lyrics_size=" << lyrics_size;
                        // символы размера допустимые,
                        // тогда переставим last_index на следующее lyrics-поле
                        last_index += (sizeof(LyricsField) + lyrics_size);
                    }
                    else // сюда, если поле id неверное -> оно либо реально неверное, либо это lyrics-концовка, уточняем :
                    {
                        //qInfo() << "contains not ok at:" << last_index;
                        auto lyrics_ending = (LyricsEnding*)lyrics_field;
                        // проверка на допустимые символы
                        for (u8i idx = 0; idx < sizeof(LyricsField::as_bytes.id); ++idx) // проверка на допустимость символов
                        {
                            if ( ( lyrics_field->as_bytes.id[idx] < 0x30 /*'0'*/) or ( lyrics_field->as_bytes.id[idx] > 0x39 /*'9'*/) ) // возврат из Lyrics, если неверные символы
                            {
                                last_index = last_index_;
                                goto mp3_id3v1;
                            }
                        }
                        if ( ( lyrics_ending->lyrics200_sign1 == 0x303253434952594C /*'LYRICS20'*/) and ( lyrics_ending->lyrics200_sign2 == '0' ) and ( file_size - last_index >= sizeof(LyricsEnding)))
                        {
                            last_index += sizeof(LyricsEnding);
                            break;
                        }
                        else // нет, это не концовка, это просто неверные данные, выходим из поиска Lyrics
                        {
                            last_index = last_index_;
                            break;
                        }
                    }
                }
                else // не хватает места на заголовок lyrics-поля -> выходим отсюда и возвращаем last_index на позицию сразу после звуковых фреймов
                {
                    last_index = last_index_;
                    break;
                }
            }
        }
    }
mp3_id3v1:
    QString info = QString("%1kbps %2Hz").arg(QString::number(avg_bit_rate / 1000), QString::number(avg_sample_rate));
    if ( file_size - last_index >= sizeof(ID3v1) ) // может осталось место под ID3v1 в конце ресурса?
    {
        auto id3v1 = (ID3v1*)&buffer[last_index];
        if ( ( id3v1->signature1 == 0x4154 ) and ( id3v1->signature2 == 0x47 ) )
        {
            info += ", song '";
            for (u8i song_name_len = 0; song_name_len < 30; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [29]
            {
                if ( id3v1->title[song_name_len] == 0 ) break;
                info.append(QChar(id3v1->title[song_name_len]));
            }
            info = info + "'";
            last_index += sizeof(ID3v1);
        }
    }
    resource_size = last_index - base_index; // коррекция resource_size с учётом Lyrics или ID3v1
    Q_EMIT e->txResourceFound("mp3", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_ogg RECOGNIZE_FUNC_HEADER
{ // https://xiph.org/ogg/doc/framing.html
  // https://www.xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-590004
#pragma pack(push,1)
    struct PageHeader
    {
        u32i signature; // 'OggS'
        u8i  version;
        u8i  flag;
        u64i granule_position;
        u32i serial_number;
        u32i page_counter;
        u32i page_checksum;
        u8i  segments_num;
    };
    struct VorbisHeader
    {
        u8i  packet_type;
        u32i signature1; // "vorb"
        u16i signature2; // "is"
        u32i vorbis_version;
        u8i  audio_channels;
        u32i sample_rate;
        u32i bitrate_max;
        u32i bitrate_nom;
        u32i bitrate_min;
    };
    struct VideoHeader
    {
        u8i  packet_type;
        u32i signature1; // "vide"
        u16i signature2; // "o\x00"
        u8i  unknown_1[2];
        u8i  codec_name[8];
        u8i  unknown_2[28];
        u32i width;
        u32i height;
        u8i  unknown[4];
    };
#pragma pack(pop)
    //qInfo() << " !!! OGG RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    enum class OggType { Unknown , Vorbis, Video };
    static u32i ogg_id {fformats["ogg"].index};
    static u32i ogm_id {fformats["ogm"].index};
    static const u64i min_room_need = sizeof(PageHeader);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    PageHeader *page_header;
    u64i last_index = base_index;
    s64i file_size = e->file_size;
    u64i pages = 0;
    bool info_derived = false;
    VorbisHeader *vorbis_header;
    VideoHeader *video_header;
    OggType ogg_type = OggType::Unknown;
    while(true)
    {
        if ( last_index + sizeof(PageHeader) > file_size ) break;// есть место для очередного PageHeader?+
        page_header = (PageHeader*)(&buffer[last_index]);
        if ( page_header->signature != 0x5367674F /*'OggS'*/) break;
        if ( last_index + sizeof(PageHeader) + page_header->segments_num > file_size ) break;// есть ли место на список размеров сегментов? он идёт сразу после PageHeader
        u32i segments_block = 0;
        for (u8i segm_idx = 0; segm_idx < page_header->segments_num; ++segm_idx) // высчитываем размер текущего блока тел сегментов
        {
            segments_block += buffer[last_index + sizeof(PageHeader) + segm_idx];
        }
        if ( last_index + sizeof(PageHeader) + page_header->segments_num + segments_block > file_size ) break;// есть ли место под блок тел сегментов?
        if ( !info_derived and ( page_header->segments_num > 0 ) and segments_block >= sizeof(VorbisHeader) )
        {
            vorbis_header = (VorbisHeader*)&buffer[last_index + sizeof(PageHeader) + page_header->segments_num];
            if ( ( vorbis_header->packet_type == 0x1 ) and ( vorbis_header->signature1 == 0x62726F76 /*vorb*/) and ( vorbis_header->signature2 == 0x7369 /*is*/) )
            {
                if ( !e->selected_formats[ogg_id] ) return 0;
                info_derived = true;
                ogg_type = OggType::Vorbis;
            }
        }
        if ( !info_derived and ( page_header->segments_num > 0 ) and segments_block >= sizeof(VideoHeader) )
        {
            video_header = (VideoHeader*)&buffer[last_index + sizeof(PageHeader) + page_header->segments_num];
            if ( ( video_header->packet_type == 0x1 ) and ( video_header->signature1 == 0x65646976 /*vide*/) and ( video_header->signature2 == 0x006F /*o */) )
            {
                if ( !e->selected_formats[ogm_id] ) return 0;
                info_derived = true;
                ogg_type = OggType::Video;
            }
        }
        last_index += (sizeof(PageHeader) + page_header->segments_num + segments_block);
        ++pages;
    }
    if ( pages < 2 ) return 0;
    u64i resource_size = last_index - base_index;
    QString info;
    QString fmt_extension;
    switch(ogg_type)
    {
    case OggType::Vorbis:
    {
        info = QString("%1Hz %2kbps").arg(QString::number(vorbis_header->sample_rate),
                                                    QString::number(vorbis_header->bitrate_nom / 1000));
        fmt_extension = "ogg";
        break;
    }
    case OggType::Video:
    {
        info = QString("%1x%2, codec_id: %3").arg(QString::number(video_header->width),
                                                            QString::number(video_header->height),
                                                            QString(QByteArray((char*)video_header->codec_name, 8)));
        fmt_extension = "ogm";
        break;
    }
    case OggType::Unknown:
        return 0;
    }
    Q_EMIT e->txResourceFound(fmt_extension, "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_med RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct MED_Header
    {
        u32i signature, mod_len, song_ptr, psecnum_and_pseq, blockarr_ptr;
        u8i  mmdflags;
        u8i  reserved[3];
        u32i smplarr_ptr, reserved2, expdata_ptr, reserved3, pstate_and_pblock, pline_and_pseqnum;
        u16i actplayline;
        u8i  counter, extra_songs;
    };
    struct ExpData
    {
        u32i nextmod_ptr, exp_smp_ptr;
        u16i s_ext_entries, s_ext_entrzs;
        u32i anno_txt_ptr, anno_len, iinfo_ptr;
        u16i i_ext_entries, i_ext_entrzs;
        u32i jump_mask, rgb_table_ptr;
        u8i  channel_split[4];
        u32i n_info_ptr, songname_ptr, songname_len;
        u32i dumps_ptr, mmdinfo_ptr, mmdrexx_ptr, mmdcmd3x_ptr;
        u32i reserved2[3];
        u32i tag_end;
    };
#pragma pack(pop)
    //qInfo() << " !!! MED RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(MED_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (MED_Header*)&buffer[base_index];
    u64i resource_size = be2le(info_header->mod_len);
    s64i file_size = e->file_size;
    if ( file_size - base_index < resource_size ) return 0; // не помещается
    // далее грубые проверки (без учёта размеров блоков по указателям)
    if ( ( ( be2le(info_header->song_ptr) >= resource_size ) or ( be2le(info_header->song_ptr) < sizeof(MED_Header) ) ) and ( be2le(info_header->song_ptr) != 0 ) ) return 0;
    if ( ( ( be2le(info_header->blockarr_ptr) >= resource_size ) or ( be2le(info_header->blockarr_ptr) < sizeof(MED_Header) ) ) and ( be2le(info_header->blockarr_ptr) != 0 ) ) return 0;
    if ( ( ( be2le(info_header->smplarr_ptr) >= resource_size ) or ( be2le(info_header->smplarr_ptr) < sizeof(MED_Header) ) ) and ( be2le(info_header->smplarr_ptr) != 0 ) ) return 0;
    if ( ( ( be2le(info_header->expdata_ptr) >= resource_size ) or ( be2le(info_header->expdata_ptr) < sizeof(MED_Header) ) ) and ( be2le(info_header->expdata_ptr) != 0 ) ) return 0;
    //
    if ( info_header->mmdflags > 1 ) return 0;
    if ( info_header->psecnum_and_pseq != 0 ) return 0;
    if ( info_header->pstate_and_pblock != 0 ) return 0;
    if ( info_header->pline_and_pseqnum != 0 ) return 0;
    if ( info_header->actplayline != 0xFFFF ) return 0;
    QString info;
    if ( be2le(info_header->expdata_ptr) != 0 ) // попробуем найти название модуля
    {
        if ( base_index + be2le(info_header->expdata_ptr) + sizeof(ExpData) <= file_size ) // есть ли место для ExpData?
        {
            auto exp_data = (ExpData*)&buffer[base_index + be2le(info_header->expdata_ptr)];
            if ( be2le(exp_data->songname_ptr) >= sizeof(MED_Header) )
            {
                if ( base_index + be2le(exp_data->songname_ptr) + be2le(exp_data->songname_len) <= file_size ) // есть ли место для строки?
                {
                    u32i songname_len = be2le(exp_data->songname_len);
                    u8i *songname_ptr = &buffer[base_index + be2le(exp_data->songname_ptr)];
                    info = "song '";
                    if (songname_len > 32) songname_len = 32;
                    for (u32i s_idx = 0; s_idx < (songname_len - 1); ++s_idx)
                    {
                        info.append(QChar(songname_ptr[s_idx]));
                    }
                    info += "'";
                }
            }
        }
    }
    Q_EMIT e->txResourceFound("med", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_dbm0 RECOGNIZE_FUNC_HEADER
{ // http://www.digibooster.de/en/format.php
#pragma pack(push,1)
    struct DBM0_Header
    {
        u32i identifier; // 'DBM0'
        u8i  version;
        u8i  revision;
        u16i reserved;
    };
    struct Chunk
    {
        u32i id;
        u32i data_len;
    };
#pragma pack(pop)
    //qInfo() << " !!! DBM0 RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(DBM0_Header);
    static const QSet <u32i> VALID_CHUNK_ID {   0x454D414E /*'NAME'*/, 0x4F464E49 /*'INFO'*/, 0x474E4F53 /*'SONG'*/, 0x54534E49 /*'INST'*/,
                                                0x54544150 /*'PATT'*/, 0x4C504D53 /*'SMPL'*/, 0x564E4556 /*'VENV'*/, 0x564E4550 /*'PENV'*/,
                                                0x45505344 /*'DSPE'*/, 0x4D414E50 /*'PNAM'*/};
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (DBM0_Header*)&buffer[base_index];
    if ( info_header->reserved != 0 ) return 0;
    s64i file_size = e->file_size;
    u64i last_index = base_index + sizeof(DBM0_Header); // ставимся на первый Chunk
    Chunk *chunk;
    QString info;
    bool at_least_one_chunk = false;
    while(true)
    {
        if ( last_index + sizeof(Chunk) > file_size ) break; // хватает ли места для очередного заголовка Chunk ?
        chunk = (Chunk*)&buffer[last_index];
        if ( !VALID_CHUNK_ID.contains(chunk->id) ) break;
        at_least_one_chunk = true;
        //qInfo() << " chunk_id:" << QString::number(chunk->id, 16) << " at:" << last_index;
        if ( last_index + sizeof(Chunk) + be2le(chunk->data_len) > file_size ) return 0; // данные чанка не помещаются
        if ( chunk->id == 0x454D414E /*'NAME'*/ )
        {
            u8i *songname_ptr = &buffer[last_index + sizeof(Chunk)];
            info = "song '";
            for (u32i s_idx = 0; s_idx < 44; ++s_idx)
            {
                if ( songname_ptr[s_idx] == 0 ) break;
                info.append(QChar(songname_ptr[s_idx]));
            }
            info += "'";
        }
        last_index += (sizeof(Chunk) + be2le(chunk->data_len));
    }
    if ( !at_least_one_chunk ) return 0; // модуль без единого чанка не имеет смысла
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound("dbm0", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
};

static const QSet<u32i> TTF_VALID_TABLE_TAG {   0x41534350 /*ASCP*/, 0x42415345 /*BASE*/, 0x43464620 /*CFF */, 0x434F4C52 /*COLR*/, 0x4350414C /*CPAL*/, 0x44534947 /*DSIG*/, 0x45424454 /*EBDT*/,
                                                0x45424C43 /*EBLC*/, 0x4646544D /*FFTM*/, 0x47444546 /*GDEF*/, 0x47504F53 /*GPOS*/, 0x47535542 /*GSUB*/, 0x48564152 /*HVAR*/, 0x4A535446 /*JSTF*/,
                                                0x4C545348 /*LTSH*/, 0x4D415448 /*MATH*/, 0x4D455247 /*MERG*/, 0x4D564152 /*MVAR*/, 0x4F532F32 /*OS/2*/, 0x50434C54 /*PCLT*/, 0x53544154 /*STAT*/,
                                                0x54544641 /*TTFA*/, 0x56444D58 /*VDMX*/, 0x564F5247 /*VORG*/, 0x61766172 /*avar*/, 0x62646174 /*bdat*/, 0x626C6F63 /*bloc*/, 0x636D6170 /*cmap*/,
                                                0x63767420 /*cvt */, 0x6670676D /*fpgm*/, 0x66766172 /*fvar*/, 0x67617370 /*gasp*/, 0x676C7966 /*glyf*/, 0x67766172 /*gvar*/, 0x68646D78 /*hdmx*/,
                                                0x68656164 /*head*/, 0x68686561 /*hhea*/, 0x686D7478 /*hmtx*/, 0x6B65726E /*kern*/, 0x6C6F6361 /*loca*/, 0x6D617870 /*maxp*/, 0x6D657461 /*meta*/,
                                                0x6E616D65 /*name*/, 0x706F7374 /*post*/, 0x70726570 /*prep*/, 0x76686561 /*vhea*/, 0x766D7478 /*vmtx*/ };

RECOGNIZE_FUNC_RETURN Engine::recognize_ttf RECOGNIZE_FUNC_HEADER
{
// https://github.com/corkami/pics/blob/master/binary/ttf.png
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6Tables.html
// https://learn.microsoft.com/en-us/typography/opentype/spec/avar
#pragma pack(push,1)
    struct TTF_Header
    {
        u32i sfnt_version; // 0x00000100 ttf or 0x4F54544F 'OTTO'
        u16i num_tables;
        u16i search_range;
        u16i entry_selector;
        u16i range_shift;
    };
    struct TableRecord
    {
        u32i table_tag;
        u32i checksum;
        u32i offset;
        u32i len;
    };
    struct NamingTableHeader
    {
        u16i format;
        u16i count;
        u16i strings_offset; // смещение от начала NamingTableHeader
    };
    struct NameRecord
    {
        u16i platf_id;
        u16i platf_specif_id;
        u16i lang_id;
        u16i name_id;
        u16i len;
        u16i offset; // смещение от начала таблицы строк
    };
#pragma pack(pop)
    //qInfo() << " !!! TTF RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(TTF_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (TTF_Header*)&buffer[base_index];
    bool is_otf = (info_header->sfnt_version == 0x4F54544F /*OTTO*/);
    u64i last_index = base_index + sizeof(TTF_Header); // переставляемся на директорию заголовков таблиц
    s64i file_size = e->file_size;
    QMap<u32i, u32i> tables_db; // бд тел таблиц
    TableRecord *table_record;
    u16i num_tables = be2le(info_header->num_tables);
    //qInfo() << "num_of_tables:" << num_tables;
    if ( num_tables == 0 ) return 0;
    if ( file_size - last_index < num_tables * sizeof(TableRecord) ) return 0; // не хватает места под директорию таблиц
    QString family_name;
    QString subfamily_name;
    for(u16i idx = 0; idx < num_tables; ++idx) // обход directory table
    {
        table_record = (TableRecord*)&buffer[last_index];
        if ( !TTF_VALID_TABLE_TAG.contains(be2le(table_record->table_tag)) ) return 0; // встретился неизвестный тег
        if ( base_index + be2le(table_record->offset) + be2le(table_record->len) > file_size ) return 0; // тело таблицы не вмещается в сканируемый файл
        tables_db[be2le(table_record->offset)] = be2le(table_record->len);
        //qInfo() << "idx:" << idx << QString("; tag_type: 0x%1").arg(QString::number(be2le(table_record->table_tag), 16)) << "; table_offset:" << be2le(table_record->offset) << "; table_len:" << be2le(table_record->len);
        if ( be2le(table_record->table_tag) == 0x6E616D65 /*name*/ ) // достаём имя и толщину шрифта
        {
            u64i ntbl_li = base_index + be2le(table_record->offset); // поставились на NamingTableHeader
            if ( ntbl_li >= file_size ) return 0; // смещение за пределами исходника
            if ( file_size - ntbl_li < sizeof(NamingTableHeader) ) return 0; // нет места на NamingTableHeader
            auto ntbl_header = (NamingTableHeader*)&buffer[ntbl_li];
            u64i strings_offset = ntbl_li + be2le(ntbl_header->strings_offset);
            if ( strings_offset >= file_size ) return 0; // неверное смещение списка строк за пределами исходного файла
            //qInfo() << "name_table header at offset:" << ntbl_li << "; name_strings at offset:" << strings_offset;
            ntbl_li += sizeof(NamingTableHeader); // ставимся на первую name-запись
            if ( file_size - ntbl_li < be2le(ntbl_header->count) * sizeof(NameRecord) ) return 0; // нет места под весь список name-записей
            auto name_rec = (NameRecord*)&buffer[ntbl_li];
            for(u16i n_idx = 0; n_idx < be2le(ntbl_header->count); ++n_idx)
            {
                // qInfo() << "name_record idx:" << n_idx << "; platf_id:" << be2le(name_rec[n_idx].platf_id) << "; platf_spec_id:" << be2le(name_rec[n_idx].platf_specif_id) << "; lang_id:" << be2le(name_rec[n_idx].lang_id)
                //         << "; name_id:" << be2le(name_rec[n_idx].name_id) << "; len:" << be2le(name_rec[n_idx].len)  << "; offset:" << strings_offset + be2le(name_rec[n_idx].offset) << "; relative_offset:" << be2le(name_rec[n_idx].offset);
                if ( ( be2le(name_rec[n_idx].platf_id) == 3 ) and ( ( be2le(name_rec[n_idx].platf_specif_id) == 0 ) or ( be2le(name_rec[n_idx].platf_specif_id) == 1 ) ) and
                     ( be2le(name_rec[n_idx].lang_id) == 1033 ) and ( be2le(name_rec[n_idx].name_id) == 1 ) and
                     ( be2le(name_rec[n_idx].len) >= 2 ) ) /// извлекаем имя шрифта
                {
                    if ( file_size - (strings_offset + be2le(name_rec[n_idx].offset)) < be2le(name_rec[n_idx].len) ) return 0;
                    // qInfo() << "possibly name is at:" << strings_offset + be2le(name_rec[n_idx].offset) << "; len:" << be2le(name_rec[n_idx].len);
                    auto name = (char*)&buffer[strings_offset + be2le(name_rec[n_idx].offset)];
                    name++; // сдвигаем на 1 байт, т.к. этот байт == \x00 и соответствует big-endian UTF-16, а нам нужен little-endian UTF-16
                    auto qba_font_name = QByteArray(name, be2le(name_rec[n_idx].len) - 1); // -1, т.к. указатель сдвинут
                    qba_font_name.append('\x00'); // добавляем недостающий ноль, т.к. в little-endian utf-16 нули для латинских букв идут после символов
                    family_name = std::move(QString::fromWCharArray((wchar_t*)qba_font_name.data(), be2le(name_rec[n_idx].len) / 2));
                }

                if ( ( be2le(name_rec[n_idx].platf_id) == 3 ) and ( ( be2le(name_rec[n_idx].platf_specif_id) == 0 ) or ( be2le(name_rec[n_idx].platf_specif_id) == 1 ) ) and
                     ( be2le(name_rec[n_idx].lang_id) == 1033 ) and ( be2le(name_rec[n_idx].name_id) == 2 ) and
                     ( be2le(name_rec[n_idx].len) >= 2 ) ) /// извлекаем subfamily (обычно это название веса шрифта (Bold/Italic/Regular/etc...))
                {
                    if ( file_size - (strings_offset + be2le(name_rec[n_idx].offset)) < be2le(name_rec[n_idx].len) ) return 0;
                    // qInfo() << "possibly weight is at:" << strings_offset + be2le(name_rec[n_idx].offset) << "; len:" << be2le(name_rec[n_idx].len);
                    auto name = (char*)&buffer[strings_offset + be2le(name_rec[n_idx].offset)];
                    name++; // сдвигаем на 1 байт, т.к. этот байт == \x00 и соответствует 'big-endian UTF-16', а нам нужен 'little-endian UTF-16'
                    auto qba_font_name = QByteArray(name, be2le(name_rec[n_idx].len) - 1); // -1, т.к. указатель сдвинут
                    qba_font_name.append('\x00'); // добавляем недостающий ноль, т.к. в 'little-endian utf-16' нули для латинских букв идут после символов
                    subfamily_name = std::move(QString::fromWCharArray((wchar_t*)qba_font_name.data(), be2le(name_rec[n_idx].len) / 2));
                }
            }
        }
        last_index += sizeof(TableRecord);
    }
    //qInfo() << tables_db;
    if ( tables_db.lastKey() < sizeof(TTF_Header) + num_tables * sizeof(TableRecord) ) return 0; // все таблицы должны быть строго после директории
    u64i resource_size = tables_db.lastKey() + tables_db[tables_db.lastKey()];
    QString info (QString("%1 %2").arg(family_name, subfamily_name));
    Q_EMIT e->txResourceFound("ttf", is_otf ? "otf" : "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_ttc RECOGNIZE_FUNC_HEADER
{ // https://docs.fileformat.com/font/ttc/
#pragma pack(push,1)
    struct TTC_Header
    {
        u32i ttc_tag;
        u16i maj_ver;
        u16i min_ver;
        u32i fonts_num;
    };
    struct DSIG_Tag
    {
        u32i tag;
        u32i len;
        u32i offset;
    };
    struct TTF_Header
    {
        u32i sfnt_version; // 0x00000100 ttf
        u16i num_tables;
        u16i search_range;
        u16i entry_selector;
        u16i range_shift;
    };
    struct TableRecord
    {
        u32i table_tag;
        u32i checksum;
        u32i offset;
        u32i len;
    };
#pragma pack(pop)
    //qInfo() << " !!! TTC RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static const u64i min_room_need = sizeof(TTC_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto ttc_header = (TTC_Header*)&buffer[base_index];
    if ( ( ttc_header->maj_ver != 0x0100 ) and ( ttc_header->maj_ver != 0x0200 ) ) return 0;
    if ( ttc_header->min_ver != 0x0000 ) return 0;
    s64i file_size = e->file_size;
    u64i last_index = base_index + sizeof(TTC_Header); // переставляемся на список смещений директорий
    if ( last_index >= file_size ) return 0; // список смещений за пределами исходника
    u32i max_fonts = be2le(ttc_header->fonts_num);
    if ( file_size - last_index < max_fonts * 4 ) return 0; // не хватает места под смещения директорий
    u32i* dir_offset_ptr = (u32i*)&buffer[last_index];
    QMap<u32i, u32i> tables_db; // бд тел таблиц всех фонтов в коллекции (а также digital signature, которая может быть в самом конце исходника)
    // анализ наличия DSIG'а
    if ( ( ttc_header->maj_ver == 0x0200 ) and ( file_size - base_index >= sizeof(TTC_Header) + max_fonts * 4 + sizeof(DSIG_Tag) ) )
    {
        auto dsig_tag = (DSIG_Tag*)&buffer[last_index + max_fonts * 4];
        if ( be2le(dsig_tag->offset) >= sizeof(TTC_Header) + max_fonts * 4 + sizeof(DSIG_Tag) )
        {
            if ( base_index + be2le(dsig_tag->offset) + be2le(dsig_tag->len) <= file_size )
            {
                tables_db[be2le(dsig_tag->offset)] = be2le(dsig_tag->len);
                //qInfo() << "digital signature present at offset:" << be2le(dsig_tag->offset) << "; len:" << be2le(dsig_tag->len);
            }
        }
    }
    for(u32i fnt_idx = 0; fnt_idx < max_fonts; ++fnt_idx) // проход по смещениям директорий
    {
        u64i font_base_offset = base_index + be2le(dir_offset_ptr[fnt_idx]); // смещение очередного фонта от начала исходника
        if ( font_base_offset >= file_size ) return 0; // смещение фонта за пределами исходника
        if ( file_size - font_base_offset < sizeof(TTF_Header) ) return 0; // не хватает места на заголовок фонта
        auto ttf_header = (TTF_Header*)&buffer[font_base_offset];
        if ( ttf_header->sfnt_version != 0x00000100 ) return 0;
        u16i num_tables = be2le(ttf_header->num_tables);
        if ( num_tables == 0 ) return 0;
        if ( file_size - font_base_offset < sizeof(TTF_Header) + sizeof(TableRecord) * num_tables ) return 0; // не хватает места под directory table
        auto table_record = (TableRecord*)&buffer[font_base_offset + sizeof(TTF_Header)]; // поставились на первую запись в directory table
        //qInfo() << "\n>>> fnt_idx:" << fnt_idx << "; font_base_offset: " << font_base_offset << "; max_tables:" << num_tables;
        for(u16i idx = 0; idx < num_tables; ++idx) // обход directory table
        {
            if ( !TTF_VALID_TABLE_TAG.contains(be2le(table_record[idx].table_tag)) ) return 0; // встретился неизвестный тег
            if ( base_index + be2le(table_record[idx].offset) + be2le(table_record[idx].len) > file_size ) return 0; // тело таблицы не вмещается в сканируемый файл
            tables_db[be2le(table_record[idx].offset)] = be2le(table_record[idx].len);
            //qInfo() << "    dir_idx:" << idx << QString("; tag_type: 0x%1").arg(QString::number(be2le(table_record[idx].table_tag), 16)) << "; table_offset:" << be2le(table_record[idx].offset) << "; table_len:" << be2le(table_record[idx].len);
        }
    }
    //qInfo() << tables_db;
    u64i resource_size = tables_db.lastKey() + tables_db[tables_db.lastKey()];
    QString info (QString("%1 font(s) in collection").arg(QString::number(max_fonts)));
    Q_EMIT e->txResourceFound("ttf", "ttc", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_asf RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct GUID
    {
        u64i data1_2_3;
        u64i data4_5;
    };
    struct ObjectHeader
    {
        GUID guid;
        u64i size;
    };
    struct ASF_Header
    {
        GUID guid;
        u64i size;
        u32i sub_objects_num;
        u8i reserved1, reserved2;
    };
    struct FileProps_Header
    {
        GUID guid;
        u64i size;
        GUID file_id;
        u64i file_size;
        u64i creation_date;
        u64i packets_cnt;
        u64i play_duration;
        u64i send_duration;
        u64i preroll;
        u32i flags;
        u32i min_pkt_size;
        u32i max_pkt_size;
        u32i max_bitrate;
    };
    struct StreamProps_Header
    {
        GUID guid;
        u64i size;
        GUID stream_type;
        GUID err_corr_type;
        u64i time_offset;
        u32i tsdl;
        u32i ecdl;
        u16i flags;
        u32i reserved1;
    };
#pragma pack(pop)
    // qInfo() << "!!! ASF RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    // qInfo() << "file:" << e->file.fileName();
    static const QMap<u64i, u64i> TOP_LEVEL_GUID {  { 0x11CF668E75B22630, 0x6CCE6200AA00D9A6 }, // header object
                                                    { 0x11CF668E75B22636, 0x6CCE6200AA00D9A6 }, // data object
                                                    { 0x11CFE5B133000890, 0xCB4903C9A000F489 }, // simple index object
                                                    { 0x11D135DAD6E229D3, 0xBE4903C9A0003490 }, // index object
                                                    { 0x4C6412ADFEB103F8, 0x8CD47A2F1D2A0F84 }, // media index object
                                                    { 0x48030C4A3CB73FD0, 0x0C8F22B6F7ED3D95 }  // timecode index object
                                                    };
    static const QMap<u64i, QString> STREAM_TYPE {  { 0x11CF5B4DF8699E40, "audio" },
                                                    { 0x11CF5B4DBC19EFC0, "video" }
                                                    };
    static const u64i min_room_need = sizeof(ObjectHeader);
    static u32i wma_id {fformats["wma"].index};
    static u32i wmv_id {fformats["wmv"].index};
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    u64i last_index = base_index;
    auto object_header = (ObjectHeader*)&buffer[base_index];
    if ( !TOP_LEVEL_GUID.contains(object_header->guid.data1_2_3) ) return 0; // неизвестный GUID
    else if ( TOP_LEVEL_GUID[object_header->guid.data1_2_3] != object_header->guid.data4_5 ) return 0; // неизвестный GUID
    u64i duration = 0;
    u32i max_bitrate = 0;
    QString format_type;
    while(true)
    {
        if ( last_index + sizeof(ObjectHeader) > file_size ) break; // нет места на заголовок очередного объекта -> возможно достигли конца сканируемого файла
        object_header = (ObjectHeader*)&buffer[last_index];
        if ( !TOP_LEVEL_GUID.contains(object_header->guid.data1_2_3) ) break; // неизвестный GUID -> достигли конца ресурса
        else if ( TOP_LEVEL_GUID[object_header->guid.data1_2_3] != object_header->guid.data4_5 ) break; // неизвестный GUID -> достигли конца ресурса
        //qInfo() << QString("Top-level object: 0x%1 at offset: 0x%2; size: %3").arg(QString::number(object_header->guid.data1_2_3, 16), QString::number(last_index, 16), QString::number(object_header->size));
        if ( file_size - last_index < object_header->size ) return 0; // неверный размер объекта (не помещается в результирующий файл)
        if ( ( object_header->guid.data1_2_3 == 0x11CF668E75B22630 ) and ( object_header->guid.data4_5 == 0x6CCE6200AA00D9A6 ) ) // попробуем достать info
        {
            if ( last_index + sizeof(ASF_Header) < file_size )
            {
                auto asf_header = (ASF_Header*)&buffer[last_index];
                //qInfo() << "sub objects num:" << asf_header->sub_objects_num;
                u64i ih_last_index = last_index + sizeof(ASF_Header); // встали на первый подобъект
                ObjectHeader* ih_header;
                for(int idx = 0; idx < asf_header->sub_objects_num; ++idx)
                {
                    if ( ih_last_index + sizeof(ObjectHeader) > file_size ) return 0; // нет места под заголовок очередного подобъекта
                    ih_header = (ObjectHeader*)&buffer[ih_last_index];
                    //qInfo() << QString("subobject:0x%1 at offset:0x%2; size: %3").arg(QString::number(ih_header->guid.data1_2_3, 16), QString::number(ih_last_index, 16), QString::number(ih_header->size));
                    if ( file_size - ih_last_index < ih_header->size ) return 0; // неверный размер подобъекта (не помещается в результирующий файл)
                    if ( ih_header->size <= sizeof(ObjectHeader) ) return 0; // неверный размер подобъекта (меньше самого заголовка или равен ему
                    u32i data_size = ih_header->size - sizeof(ObjectHeader);
                    if ( ( ih_header->guid.data1_2_3 == 0x11CFA9478CABDCA1 ) and ( ih_header->guid.data4_5 == 0x6553200CC000E48E ) and ( ih_header->size >= sizeof(FileProps_Header) ) ) // file properties subobject
                    {
                        auto properties = (FileProps_Header*)&buffer[ih_last_index];
                        duration = properties->play_duration / 10000; // перевод в миллисекунды
                        max_bitrate = properties->max_bitrate;
                    }
                    if ( ( ih_header->guid.data1_2_3 == 0x11CFA9B7B7DC0791 ) and ( ih_header->guid.data4_5 == 0x6553200CC000E68E ) and ( ih_header->size >= sizeof(StreamProps_Header) ) ) // stream properties subobject
                    {
                        auto properties = (StreamProps_Header*)&buffer[ih_last_index];
                        switch(properties->stream_type.data1_2_3)
                        {
                        case 0x11CF5B4DF8699E40: // audio
                        {
                            if ( !e->selected_formats[wma_id] ) return 0;
                            format_type = "wma";
                            break;
                        }
                        case 0x11CF5B4DBC19EFC0: // video
                        {
                            if ( !e->selected_formats[wmv_id] ) return 0;
                            format_type = "wmv";
                            break;
                        }
                        default:
                            return 0; // неизвестный тип (не видео и не аудио)
                        }
                    }
                    ih_last_index += (ih_header->size);
                }
            }
            else
            {
                return 0;  // нет места под главный ASF заголовок
            }
        }
        last_index += (object_header->size);
    }
    // здесь last_index стоит на позиции сразу за последним объектом
    //
    QTime duration_time(0, 0, 0);
    duration_time = duration_time.addMSecs(duration);
    QString info = QString("%1kbps %2").arg(  QString::number(max_bitrate / 1000),
                                                        duration_time.toString(Qt::ISODateWithMs));
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound(format_type, "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}


RECOGNIZE_FUNC_RETURN Engine::recognize_wmf RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct PlaceableHeader
    {
        u32i key;
        u16i hwmf;
        u16i bbox_left, bbox_top, bbox_right, bbox_bottom;
        u16i inch;
        u32i reserved;
        u16i checksum;
    };
    struct MetaHeader
    {
        u16i type;
        u16i header_size;
        u16i version;
        u32i size; // u16i size_low + u16i size_high
        u16i objs_num;
        u32i max_record;
        u16i members_num;
    };
    struct RecordHeader
    {
        u32i rec_size;
        u16i rec_func;
    };
#pragma pack(pop)/*
    qInfo() << "!!! WMF RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    qInfo() << "file:" << e->file.fileName();*/
    static const QSet<u16i> VALID_FUNC {0x0000, 0x0035, 0x0037, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106, 0x0107, 0x0108, 0x0127, 0x0139, 0x0142, 0x0149, 0x0201, 0x0209,
                                        0x0211, 0x0213, 0x0214, 0x0220, 0x0228, 0x0231, 0x0234, 0x0324, 0x0325, 0x020A, 0x020B, 0x020C, 0x020D, 0x020E, 0x020F, 0x0410,
                                        0x0412, 0x0415, 0x0416, 0x0418, 0x0419, 0x0429, 0x0436, 0x0521, 0x0538, 0x0548, 0x041B, 0x041F, 0x061C, 0x061D, 0x001E, 0x081A,
                                        0x0B23, 0x0626, 0x012A, 0x012B, 0x012C, 0x012D, 0x012E, 0x0817, 0x0830, 0x0922, 0x0a32, 0x0d33, 0x0940, 0x0b41, 0x0f43, 0x01f0,
                                        0x00f7, 0x01F9, 0x02FA, 0x02FB, 0x02FC, 0x06FF};
    bool placeable = ( *((u32i*)&e->mmf_scanbuf[e->scanbuf_offset]) == 0x9AC6CDD7 );
//    qInfo() << "placeable:" << placeable;
    u64i base_index = e->scanbuf_offset;
    e->resource_offset = base_index;
    if ( placeable)
    {
        if ( !e->enough_room_to_continue(sizeof(PlaceableHeader) + sizeof(MetaHeader)) ) return 0;
    }
    else
    {
        if ( !e->enough_room_to_continue(sizeof(MetaHeader)) ) return 0;
    }
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    u64i last_index = base_index;
    double image_width;
    double image_height;
    u16i image_inch;
    if ( placeable)
    {
        auto plcb_header = (PlaceableHeader*)&buffer[base_index];
        if ( plcb_header->reserved != 0x00000000 ) return 0;
        //qInfo() << "left:" << plcb_header->bbox_left << "; top:" << plcb_header->bbox_top << "; right:" << plcb_header->bbox_right << "; bottom:" << plcb_header->bbox_bottom << "; inch:" << plcb_header->inch;
        image_width = double(( plcb_header->bbox_right > plcb_header->bbox_left ) ? plcb_header->bbox_right - plcb_header->bbox_left : plcb_header->bbox_left - plcb_header->bbox_right) / plcb_header->inch;
        image_height = double(( plcb_header->bbox_bottom > plcb_header->bbox_top ) ? plcb_header->bbox_bottom - plcb_header->bbox_top : plcb_header->bbox_top - plcb_header->bbox_bottom) / plcb_header->inch;
        image_inch = plcb_header->inch;
        last_index += sizeof(PlaceableHeader); // переставляемся на MetaHeader
    }
    auto meta_header = (MetaHeader*)&buffer[last_index];
    if ( ( meta_header->type != 0x0001 ) and ( meta_header->type != 0x0002 ) ) return 0;
    if ( meta_header->header_size != 0x0009 ) return 0;
    if ( ( meta_header->version != 0x0300 ) and ( meta_header->version != 0x0100 ) ) return 0;
    if ( meta_header->members_num != 0 ) return 0;
    u64i record_li = last_index + sizeof(MetaHeader); // ставимся на первую запись типа Record
    last_index = base_index + meta_header->size * 2 + (placeable ? sizeof(PlaceableHeader) : 0); // сразу рассчитываем последнее смещение по данным из заголовка ресурса
    if ( last_index > file_size ) return 0;
    RecordHeader *record;
    while(true) // идём по записям в поисках EOF, попутно валидируя функции
    {
        if ( file_size - record_li < sizeof(RecordHeader) ) return 0; // нет места на очередной RecordHeader
        record = (RecordHeader*)&buffer[record_li];
        //qInfo() << "record_offset:" << record_li << "; rec_size:" << record->rec_size * 2 << QString("; rec_function: 0x%1").arg(QString::number(record->rec_func, 16).rightJustified(4, '0'));
        if ( !VALID_FUNC.contains(record->rec_func) ) return 0; // неизвестная функция
        if ( record->rec_size < 3 ) return 0; // неверный размер записи
        record_li += (record->rec_size * 2);
        if ( record->rec_func == 0 ) break; // нашли EOF
    }
    //qInfo() << "record_li:" << record_li << "; last_index:" << last_index;
    u64i resource_size = last_index - base_index;
    QString info;
    if ( placeable ) info = QString("%1 dpi : %2 x %3 inch (%4 x %5 cm)").arg(QString::number(image_inch),
                                                                                        QString::number(image_width, 'g', 4),
                                                                                        QString::number(image_height, 'g', 4),
                                                                                        QString::number(image_width * 25.4 / 10, 'g', 4),
                                                                                        QString::number(image_height * 25.4 / 10, 'g', 4));
    Q_EMIT e->txResourceFound("wmf", "", base_index, resource_size, info);
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_emf RECOGNIZE_FUNC_HEADER
{
#pragma pack(push,1)
    struct EMR_EMF_Header
    {
        u32i type;
        u32i header_size;
        u32i b_left, b_top, b_right, b_bottom;
        u32i f_left, f_top, f_right, f_bottom;
        u32i signature; // " EMF"
        u32i version;
        u32i file_size;
        u32i recs_num;
        u16i objs_num;
        u16i reserved;
        u32i descr_size;
        u32i descr_offset;
        u32i pall_num;
        u32i dev_cx_pix, dev_cy_pix;
        u32i dev_cx_mm, dev_cy_mm;
    };
#pragma pack(pop)
    //qInfo() << "!!! EMF RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    //qInfo() << "file:" << e->file.fileName();
    if ( e->scanbuf_offset < 40 ) return 0; // не хватает места под часть заголовка перед сигнатурой
    if ( !e->enough_room_to_continue(48) ) return 0;  // не хватает места под часть заголовка, начиная с сигнатуры
    u64i base_index = e->scanbuf_offset - 40;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (EMR_EMF_Header*)&buffer[base_index];
    if ( info_header->type != 0x00000001 ) return 0;
    if ( info_header->header_size < 88 ) return 0;
    s64i file_size = e->file_size;
    if ( file_size - base_index < info_header->file_size ) return 0; // неверное поле .->file_size в заголовке
    u64i last_index = base_index + info_header->file_size; // переставляемся на размер .->file_size
    // qInfo() << "file_size:" << info_header->file_size << "; descr_size:" << info_header->descr_size << "; descr_offset:" << info_header->descr_offset << "; pal_num:" << info_header->pall_num;
    // qInfo() << "b_left:" << info_header->b_left << "; b_top:" << info_header->b_top << "; b_right:" << info_header->b_right << "; b_bottom:" << info_header->b_bottom;
    // qInfo() << "f_left:" << info_header->f_left << "; f_top:" << info_header->f_top << "; f_right:" << info_header->f_right << "; f_bottom:" << info_header->f_bottom;
    // qInfo() << "dev_cx_pix:" << info_header->dev_cx_pix << "; dev_cy_pix:" << info_header->dev_cy_pix << "; dev_cx_mm:" << info_header->dev_cx_mm << "; dev_cy_mm:" << info_header->dev_cy_mm;
    double image_width = double(( info_header->b_right > info_header->b_left ) ? info_header->b_right - info_header->b_left : info_header->b_left - info_header->b_right) / 96;
    double image_height = double(( info_header->b_bottom > info_header->b_top ) ? info_header->b_bottom - info_header->b_top : info_header->b_top - info_header->b_bottom) / 96;
    QString info = QString("%1 dpi : %2 x %3 inch (%4 x %5 cm)").arg( QString::number(96),
                                                                                QString::number(image_width, 'g', 4),
                                                                                QString::number(image_height, 'g', 4),
                                                                                QString::number(image_width * 25.4 / 10, 'g', 4),
                                                                                QString::number(image_height * 25.4 / 10, 'g', 4));
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound("emf", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

// округление числа до кратности 4 в большую сторону
u32i align_bnd4(u32i value)
{
    return (value + 3) & 0xFFFFFFFC; // эквив. ((value + 3) >> 2) << 2 или ((value + 3) / 4) * 4
}

RECOGNIZE_FUNC_RETURN Engine::recognize_dds RECOGNIZE_FUNC_HEADER
{ // https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds
  // https://en.wikipedia.org/wiki/S3_Texture_Compression
  // https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression
#pragma pack(push,1)
    struct DDS_Header
    {
        u32i magic;
        u32i struct_size;
        u32i flags;
        u32i height;
        u32i width;
        u32i pitch_lsize;
        u32i depth;
        u32i mipmap_cnt;
        u32i reserved1[11];

        u32i pix_fmt_struct_size;
        u32i pix_fmt_flags;
        u32i pix_fmt_four_cc;
        u32i pix_fmt_rgb_bit_cnt;
        u32i pix_fmt_r_bitmask;
        u32i pix_fmt_g_bitmask;
        u32i pix_fmt_b_bitmask;
        u32i pix_fmt_a_bitmask;

        u32i caps;
        u32i caps2;
        u32i caps3;
        u32i caps4;
        u32i reserved2;
    };
    struct DX10_Header
    {
        u32i dxgi_fmt;
        u32i res_dim;
        u32i misc_flags1;
        u32i array_size;
        u32i misc_flags2;
    };
    struct Compression
    {
        QString name;
        u8i div_shift;
    };
#pragma pack(pop)
    // qInfo() << "!!! DDS RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    // qInfo() << "file:" << e->file.fileName();
    static const QSet<u32i> VALID_BITCOUNT { 8, 16, 24, 32 };
    static const QHash<u32i,Compression> VALID_COMPRESSION {{ 0x31545844, { "DXT1", 1 } }, { 0x32545844, { "DXT2", 0 } },
                                                            { 0x33545844, { "DXT3", 0 } }, { 0x34545844, { "DXT4", 0 } },
                                                            { 0x35545844, { "DXT5", 0 } }, { 0x30315844, { "DX10", 0 } }, // div_shift для DX10 может не использоваться в случае несжатой текстуры
                                                            { 0x31495441, { "ATI1", 1 } }, { 0x32495441, { "ATI2", 0 } },
                                                            { 0x42475852, { "RXGB", 0 } },
                                                            { 0x55344342, { "BC4U", 1 } }, { 0x55354342, { "BC5U" ,0 }},
                                                            { 0x53354342, { "BC5S", 0 } }
                                                            };
    static const QSet<u32i> VALID_DXGI { 29 /*R8G8B8A8_UNORM_SRGB*/, 91 /*B8G8R8A8_UNORM_SRGB*/, 95 /*BC6H_UF16*/, 98 /*BC7_UNORM*/, 99 /*BC7_UNORM_SRGB*/ };
    static const QHash<u32i,u8i> VALID_COMPRESSED_DXGI { {95, 0}, {98, 0}, {99, 0} }; // где u8i это div_shift
    static const QHash<u32i,u8i> VALID_UNCOMPRESSED_DXGI { {29, 2}, {91, 2} }; // где u8i это mult_shift
    static const QSet<u32i> VALID_PIXFLAGS {0x02,   /* only alpha uncompressed */
                                            0x04,   /* compressed rgb */
                                            0x40,   /* uncompressed rgb */
                                            0x20000 /* only luminance uncompressed */
                                           };
    static const u64i min_room_need = sizeof(DDS_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    u64i last_index = base_index;
    auto info_header = (DDS_Header*)&buffer[base_index];
    if ( !info_header->width or !info_header->height ) return 0;
    if ( ( info_header->struct_size != 124 ) or ( info_header->pix_fmt_struct_size != 32 ) ) return 0;
    if ( ( info_header->flags & 0x1007 ) != 0x1007 ) return 0; // заполнены ли обязательные поля caps, width, height, pix_fmt?
    if ( ( ( info_header->flags & 0x08 ) == 0x08 ) and ( ( info_header->flags & 0x080000 ) == 0x080000 ) ) return 0; // текстура не может быть одновременно и сжатой и несжатой
    if ( ( ( info_header->pix_fmt_flags & 0x04 ) == 0x04 ) and ( ( info_header->pix_fmt_flags & 0x40 ) == 0x40 ) ) return 0; // текстура не может быть одновременно и сжатой и несжатой
    // qInfo() << "width:" << info_header->width << "; height:" << info_header->height << "; pitch_lsize:" << info_header->pitch_lsize << "; depth:" << info_header->depth
    //         << "; mipmap_cnt:" << info_header->mipmap_cnt << "; flags:" << QString::number(info_header->flags, 16) << "; pix_flags:" << QString::number(info_header->pix_fmt_flags, 16)
    //         << "; pix_fmt_rgb_bit_cnt:" << info_header->pix_fmt_rgb_bit_cnt;
    bool valid_type = false;
    for(auto & one_type: VALID_PIXFLAGS)
    {
        if ( ( info_header->pix_fmt_flags & one_type ) == one_type ) valid_type = true;
    }
    if ( !valid_type ) return 0;

    QString info;
    u64i textures_array_size = 0; // суммарный размер текстур всех mipmap-уровней
    u32i mmp_cnt = qMax(1, int(info_header->mipmap_cnt)); // 0 и 1 - то же самое, т.е. всего 1 mipmap-уровень
    if ( (mmp_cnt - 1) > 31 ) return 0; // сдвиг (uint32)1 << 32 даст 0, а на ноль делить нельзя (в формулах вычисления scal_w и scal_h)
    u32i w = info_header->width;
    u32i h = info_header->height;
    u32i scal_w; // scaled width : divided by 2^mmp_idx
    u32i scal_h; // scaled height : divided by 2^mmp_idx

    if ( ( info_header->pix_fmt_flags & 0x04 ) == 0x04 ) // сжатая обычная текстура, либо DX10-текстура (сжатая или несжатая)
    {
        if ( !VALID_COMPRESSION.contains(info_header->pix_fmt_four_cc) ) return 0;
        u8i div_shift; // используется в формуле расчёта сжатых данных
        u8i mult_shift; // используется в формуле расчёта несжатых DX10-данных
        div_shift = VALID_COMPRESSION[info_header->pix_fmt_four_cc].div_shift;
        // div_shift = 1 эквив. 8/16=1/2, то-есть делению на 2;
        // 8/16, где 8 байт размер сжатого квадрата 4x4 из 16 пикселей
        if ( info_header->pix_fmt_four_cc == 0x30315844 /*DX10*/ )
        {
            if ( file_size - base_index < sizeof(DDS_Header) + sizeof(DX10_Header) ) return 0; // хватает ли места под DX10-заголовок?
            auto dx10_header = (DX10_Header*)&buffer[base_index + sizeof(DDS_Header)];
            // qInfo() << "dxgi_fmt:" << dx10_header->dxgi_fmt << "; res_dim:" << dx10_header->res_dim << "; misc_flags1:" << QString::number(dx10_header->misc_flags1, 16)
            //         << "; array_size:" << dx10_header->array_size << "; misc_flags2:" << QString::number(dx10_header->misc_flags2);
            if ( !VALID_DXGI.contains(dx10_header->dxgi_fmt) ) return 0;
            if ( VALID_COMPRESSED_DXGI.contains(dx10_header->dxgi_fmt) ) // сжатый DX10-формат
            {
                // qInfo() << "compressed DX10";
                div_shift = VALID_COMPRESSED_DXGI[dx10_header->dxgi_fmt]; // вычисление делителя для сжатой DX10-текстуры
                for(u32i mmp_idx = 0; mmp_idx < mmp_cnt; ++mmp_idx )
                {
                    scal_w = qMax(4, int(align_bnd4(w / (1 << mmp_idx)))); // 4 пикселя - минимальные ширина и высота
                    scal_h = qMax(4, int(align_bnd4(h / (1 << mmp_idx)))); //
                    // qInfo() << scal_w << "x" << scal_h << ":" << textures_array_size << "+" << ((scal_w * scal_h) >> div_shift);
                    textures_array_size += ((scal_w * scal_h) >> div_shift); // эквив. формуле (((w * h) / (2 ^ (mmp_idx * 2))) / 2), где div_shift == 1
                }
            }
            else // несжатый DX10-формат
            {
                // qInfo() << "uncompressed DX10";
                mult_shift = VALID_UNCOMPRESSED_DXGI[dx10_header->dxgi_fmt]; // вычисление множителя для несжатой DX10-текстуры
                for(u32i mmp_idx = 0; mmp_idx < mmp_cnt; ++mmp_idx )
                {
                    scal_w = qMax(1, int(w / (1 << mmp_idx)));
                    scal_h = qMax(1, int(h / (1 << mmp_idx)));
                    // qInfo() << scal_w << "x" << scal_h << ":" << textures_array_size << "+" << (scal_w * scal_h << mult_shift);
                    textures_array_size += (scal_w * scal_h << mult_shift);
                }
            }
        }
        else // обычная сжатая текстура (не DX10)
        {
            for(u32i mmp_idx = 0; mmp_idx < mmp_cnt; ++mmp_idx )
            {
                scal_w = qMax(4, int(align_bnd4(w / (1 << mmp_idx)))); // 4 пикселя - минимальные ширина и высота
                scal_h = qMax(4, int(align_bnd4(h / (1 << mmp_idx)))); //
                // qInfo() << scal_w << "x" << scal_h << ":" << textures_array_size << "+" << ((scal_w * scal_h) >> div_shift);
                textures_array_size += ((scal_w * scal_h) >> div_shift); // эквив. формуле (((w * h) / (2 ^ (mmp_idx * 2))) / 2), где div_shift == 1
            }
        }
        info = QString("%1x%2 (%3), %4 mip(s)").arg(QString::number(w),
                                                    QString::number(h),
                                                    VALID_COMPRESSION[info_header->pix_fmt_four_cc].name,
                                                    QString::number(mmp_cnt)
                                                    );
        // qInfo() << "compressed texture";
    }

    if ( ( ( info_header->pix_fmt_flags & 0x02 ) == 0x02 ) or
         ( ( info_header->pix_fmt_flags & 0x40 ) == 0x40 ) or
         ( ( info_header->pix_fmt_flags & 0x200 ) == 0x200 ) or
         ( ( info_header->pix_fmt_flags & 0x20000 ) == 0x20000 ) ) // несжатые текстуры типов : Alpha/RGB/YUV/Luminance
    {
        if ( !VALID_BITCOUNT.contains(info_header->pix_fmt_rgb_bit_cnt) ) return 0;
        for(u32i mmp_idx = 0; mmp_idx < mmp_cnt; ++mmp_idx )
        {
            scal_w = qMax(1, int(w / (1 << mmp_idx)));
            scal_h = qMax(1, int(h / (1 << mmp_idx)));
            // qInfo() << scal_w << "x" << scal_h << ":" << textures_array_size << "+" << (scal_w * scal_h * (info_header->pix_fmt_rgb_bit_cnt >> 3));
            textures_array_size += (scal_w * scal_h * (info_header->pix_fmt_rgb_bit_cnt >> 3)); // эквив. формула : (scal_w * scal_h) * (info_header->pix_fmt_rgb_bit_cnt / 8)
        }
        QString texture_type;
        if ( ( info_header->pix_fmt_flags & 0x02 ) == 0x02 ) texture_type = "alpha-ch.";
        if ( ( info_header->pix_fmt_flags & 0x40 ) == 0x40 ) texture_type = "RGB";
        if ( ( info_header->pix_fmt_flags & 0x200 ) == 0x200 ) texture_type = "YUV";
        if ( ( info_header->pix_fmt_flags & 0x20000 ) == 0x20000 ) texture_type = "lumin.";
        info = QString("%1x%2 %3-bpp (%4), %5 mip(s)").arg(  QString::number(info_header->width),
                                                            QString::number(info_header->height),
                                                            QString::number(info_header->pix_fmt_rgb_bit_cnt),
                                                            texture_type,
                                                            QString::number(mmp_cnt)
                                                        );
        // qInfo() << "non-compressed alpha/rgb/yuv/lumin. texture";
    }
    u64i resource_size = sizeof(DDS_Header) + sizeof(DX10_Header) * u32i(info_header->pix_fmt_four_cc == 0x30315844) + textures_array_size;
    if ( file_size - base_index < resource_size ) return 0;
    Q_EMIT e->txResourceFound("dds", "", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}
