  //   lbl|sign| AHAL | BHBL
  //   ---|----|------|------
  //   0  |tga  : 0000 : 0200
  //   0  |tga  : 0000 : 0A00
  //   0  |ico  : 0000 : 0100
  //   0  |cur  : 0000 : 0200 <- crossing with tga
  //   1  |pcx  : 0A05
  //   2  |fli  : 11AF
  //   3  |flc  : 12AF
  //   25 |au   : 2E73 : 6E64
  //   4  |bik  : 4249
  //   4  |bmp  : 424D
  //   24 |ch   : 4348
  //   24 |voc  : 4372 : 6561
  //   6  |flx  : 44AF
  //   6  |dbm0 : 4442 : 4D30
  //   7  |xm   : 4578 : 7465
  //   8  |iff  : 464F : 524D
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
  //   27 |ogg  : 4F67 : 6753
  //   16 |rif  : 5249 : 4646
  //   17 |s3m  : 5343 : 524D
  //   17 |sm2  : 534D : 4B32
  //   17 |sm4  : 534D : 4B34
  //   22 |669  : 6966
  //   26 |mdat : 6D64 : 6174
  //   26 |moov : 6D6F : 6F76
  //   20 |png  : 8950 : 4E47
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
};

u16i be2le(u16i be)
{
    union {
        u16i as_le;
        u8i  bytes[2];
    };
    bytes[0] = be >> 8;
    bytes[1] = be;
    return as_le;
}

u32i be2le(u32i be)
{
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

u32i be2le(u64i be)
{
    union {
        u64i as_le;
        u8i  bytes[8];
    };
    bytes[0] = be >> 56;
    bytes[1] = be >> 48;
    bytes[2] = be >> 40 ;
    bytes[3] = be >> 32;
    bytes[4] = be >> 24;
    bytes[5] = be >> 16;
    bytes[6] = be >> 8;
    bytes[7] = be;
    return as_le;
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
    Label aj_signat_labels[30]; // 30 - с запасом
    Label aj_sub_labels[30]; // 30 - с запасом
    Label aj_scrup_mode_check_label = aj_asm.newLabel();
    Label aj_scrup_mode_off_label = aj_asm.newLabel();
    Label aj_loop_check_label = aj_asm.newLabel();
    Label aj_epilog_label = aj_asm.newLabel();

    int s_idx;
    for (s_idx = 0; s_idx < 30; ++s_idx) // готовим лейблы
    {
        aj_signat_labels[s_idx] = aj_asm.newLabel();
        aj_sub_labels[s_idx] = aj_asm.newLabel();
    }

    //x86::Mem rbp_plus_16 = x86::ptr(x86::rbp, 16); // указатель на наш "home rcx"
    x86::Mem rdi_edx_mul8 = x86::ptr(x86::rdi, x86::edx, 3); // shift = 3, равноценно *8 : [rdi + r15*8]

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
    if ( selected_formats[fformats["tga_tc"].index] or selected_formats[fformats["ico_win"].index] or selected_formats[fformats["cur_win"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[0])); // tag_tc, ico_win, cur_win
        aj_asm.mov(x86::ptr(x86::rdi, 0x00 * 8), x86::rax);
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

    if ( selected_formats[fformats["au"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[25])); // au
        aj_asm.mov(x86::ptr(x86::rdi, 0x2E * 8), x86::rax);
    }

    if ( selected_formats[fformats["bik"].index] or selected_formats[fformats["bmp"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[4])); // bik, bmp
        aj_asm.mov(x86::ptr(x86::rdi, 0x42 * 8), x86::rax);
    }

    if ( selected_formats[fformats["mod"].index] or selected_formats[fformats["voc"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[24])); // mod 'CH', voc
        aj_asm.mov(x86::ptr(x86::rdi, 0x43 * 8), x86::rax);
    }

    if ( selected_formats[fformats["flc"].index] or selected_formats[fformats["dbm0"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[6])); // flx, dbm0
        aj_asm.mov(x86::ptr(x86::rdi, 0x44 * 8), x86::rax);
    }

    if ( selected_formats[fformats["xm"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[7])); // xm
        aj_asm.mov(x86::ptr(x86::rdi, 0x45 * 8), x86::rax);
    }

    if ( selected_formats[fformats["xmi"].index] or selected_formats[fformats["lbm"].index] or selected_formats[fformats["aif"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[8])); // iff
        aj_asm.mov(x86::ptr(x86::rdi, 0x46 * 8), x86::rax);
    }

    if ( selected_formats[fformats["gif"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[9])); // gif
        aj_asm.mov(x86::ptr(x86::rdi, 0x47 * 8), x86::rax);
    }

    if ( selected_formats[fformats["tif_ii"].index] or selected_formats[fformats["it"].index] or selected_formats[fformats["mp3"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[10])); // tif_ii, it, mp3 id3v2
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

    if ( selected_formats[fformats["mod"].index] or selected_formats[fformats["tif_mm"].index] or selected_formats[fformats["mid"].index] or selected_formats[fformats["med"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[13])); // mod 'M.K.', tif_mm, mid, mmd0, mmd1, mmd2, mmd3
        aj_asm.mov(x86::ptr(x86::rdi, 0x4D * 8), x86::rax);
    }

    if ( selected_formats[fformats["ogg"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[27])); // ogg
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

    if ( selected_formats[fformats["png"].index] )
    {
        aj_asm.lea(x86::rax, x86::ptr(aj_signat_labels[20])); // png
        aj_asm.mov(x86::ptr(x86::rdi, 0x89 * 8), x86::rax);
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
    // ; tga : 0x00'00 : 0x02'00
    // ; tga : 0x00'00 : 0x0A'00
    // ; ico : 0x00'00 : 0x01'00
    // ; cur : 0x00'00 : 0x02'00
    aj_asm.cmp(x86::al, 0x00);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x02'00);
    aj_asm.je(aj_sub_labels[13]);
    aj_asm.cmp(x86::bx, 0x0A'00);
    aj_asm.je(aj_sub_labels[13]);
    aj_asm.cmp(x86::bx, 0x01'00);
    aj_asm.je(aj_sub_labels[14]);
    aj_asm.jmp(aj_loop_check_label);
    aj_asm.bind(aj_sub_labels[13]);
    if ( selected_formats[fformats["tga_tc"].index] )
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
    if ( selected_formats[fformats["ico_win"].index] or selected_formats[fformats["cur_win"].index] )
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

// ; 0x42
aj_asm.bind(aj_signat_labels[4]);
    // ; bik : 0x42'49
    // ; bmp : 0x42'4D
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
    if ( selected_formats[fformats["bmp"].index] )
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
    // ;  flx : 0x44'AF
aj_asm.bind(aj_sub_labels[18]); // dbm0?
    if ( selected_formats[fformats["dbm0"].index] )
    {
        aj_asm.cmp(x86::al, 0x42);
        aj_asm.jne(aj_sub_labels[19]);
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
    // ; iff : 0x46'4F : 0x52'4D
    aj_asm.cmp(x86::al, 0x4F);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x52'4D);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_xm
    aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
    aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
    aj_asm.call(imm((u64i)Engine::recognize_iff));
    aj_asm.cmp(x86::rax, 0);
    aj_asm.jne(aj_scrup_mode_check_label);
    //
    aj_asm.jmp(aj_loop_check_label);

// ; 0x47
aj_asm.bind(aj_signat_labels[9]);
    // ; gif : 0x47'49 : 0x46'38
    aj_asm.cmp(x86::al, 0x49);
    aj_asm.jne(aj_loop_check_label);
    aj_asm.cmp(x86::bx, 0x46'38);
    aj_asm.jne(aj_loop_check_label);
    // вызов recognize_xm
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
    if ( selected_formats[fformats["tif_ii"].index] )
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
        // вызов recognize_mod_mk
        aj_asm.mov(x86::qword_ptr(x86::r13), x86::r14); // пишем текущее смещение из r14 в this->scanbuf_offset, который по адресу [r13]
        aj_asm.mov(x86::rcx, imm(this)); // передача первого (и единственного) параметра в recognizer
        aj_asm.call(imm((u64i)Engine::recognize_mod));
        aj_asm.cmp(x86::rax, 0);
        aj_asm.jne(aj_scrup_mode_check_label);
        //
        aj_asm.jmp(aj_loop_check_label);
    }
aj_asm.bind(aj_sub_labels[5]); // tif_mm ?
    if ( selected_formats[fformats["tif_mm"].index] )
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
    // ; ogg  : 0x4F'67 : 0x67'53
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
    //qInfo() << " BMP recognizer called!" << e->scanbuf_offset;
    static constexpr u64i min_room_need = sizeof(FileHeader);
    static const QSet <u32i> VALID_BMP_HEADER_SIZE { 12, 40, 108 };
    static const QSet <u16i> VALID_BITS_PER_PIXEL { 1, 4, 8, 16, 24, 32 };
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    auto info_header = (FileHeader*)(&buffer[base_index]);
    if ( info_header->reserved != 0 ) return 0;
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
    Q_EMIT e->txResourceFound("bmp", base_index, resource_size, info);
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
    static const QSet <u8i>  VALID_BIT_DEPTH  {1, 2, 4, 8, 16};
    static const QSet <u8i>  VALID_COLOR_TYPE {0, 2, 3, 4, 6};
    static const QSet <u8i>  VALID_INTERLACE  {0, 1};
    static const QSet <u32i> VALID_CHUNK_TYPE {    0x54414449 /*IDAT*/, 0x4D524863 /*cHRM*/, 0x414D4167 /*gAMA*/, 0x54494273 /*sBIT*/, 0x45544C50 /*PLTE*/, 0x44474B62 /*bKGD*/,
                                                   0x54534968 /*hIST*/, 0x534E5274 /*tRNS*/, 0x7346466F /*oFFs*/, 0x73594870 /*pHYs*/, 0x4C414373 /*sCAL*/, 0x54414449 /*IDAT*/, 0x454D4974 /*tIME*/,
                                                   0x74584574 /*tEXt*/, 0x7458547A /*zTXt*/, 0x63415266 /*fRAc*/, 0x67464967 /*gIFg*/, 0x74464967 /*gIFt*/, 0x78464967 /*gIFx*/, 0x444E4549 /*IEND*/,
                                                   0x42475273 /*sRGB*/, 0x52455473 /*sTER*/, 0x4C414370 /*pCAL*/, 0x47495364 /*dSIG*/, 0x50434369 /*iCCP*/, 0x50434963 /*iICP*/, 0x7643446D /*mDCv*/,
                                                   0x694C4C63 /*cLLi*/, 0x74585469 /*iTXt*/, 0x744C5073 /*sPLt*/, 0x66495865 /*eXIf*/, 0x52444849 /*iHDR*/};
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
    Q_EMIT e->txResourceFound("png", base_index, resource_size, info);
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
    //qInfo() << " RIFF RECOGNIZER CALLED:";
    static u32i avi_id {fformats["avi"].index};
    static u32i wav_id {fformats["wav"].index};
    static u32i rmi_id {fformats["rmi"].index};
    static u32i ani_id {fformats["ani_riff"].index};
    static const u64i min_room_need = sizeof(ChunkHeader);
    static const QSet <u64i> VALID_SUBCHUNK_TYPE {  0x5453494C20495641 /*AVI LIST*/,
                                                    0x20746D6645564157 /*WAVEfmt */,
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
        Q_EMIT e->txResourceFound("avi", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x20746D6645564157: // wav, "WAVEfmt " subchunk
    {
        if ( ( !e->selected_formats[wav_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(WavInfoHeader)) ) return 0;
        auto wav_info_header = (WavInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        if ( ( wav_info_header->chans == 0 ) or ( wav_info_header->chans > 6 ) ) return 0;
        if ( !VALID_WAV_BPS.contains(wav_info_header->bits_per_sample) ) return 0;
        QString codec_name = ( wave_codecs.contains(wav_info_header->fmt_code) ) ? wave_codecs[wav_info_header->fmt_code] : "unknown";
        auto info = QString(R"(%1 codec : %2-bit %3Hz %4-ch)").arg( codec_name,
                                                                    QString::number(wav_info_header->bits_per_sample),
                                                                    QString::number(wav_info_header->sample_rate),
                                                                    QString::number(wav_info_header->chans));
        Q_EMIT e->txResourceFound("wav", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x6174616444494D52: // rmi - midi data encapsulated in riff, RMIDdata subchunk
    {
        if ( ( !e->selected_formats[rmi_id] ) ) return 0;
        if ( resource_size < (sizeof(ChunkHeader) + sizeof(MidiInfoHeader)) ) return 0;
        auto midi_info_header = (MidiInfoHeader*)(&buffer[base_index + sizeof(ChunkHeader)]);
        auto info = QString(R"(%1 track(s))").arg(be2le(midi_info_header->ntrks));
        Q_EMIT e->txResourceFound("rmi", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x5453494C4E4F4341: // ani - animated cursor with ACONLIST subchunk
    {
        if ( ( !e->selected_formats[ani_id] ) ) return 0;
        Q_EMIT e->txResourceFound("ani_riff", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x68696E614E4F4341: // ani - animated cursor with ACONanih subchunk
    {
        if ( ( !e->selected_formats[ani_id] ) ) return 0;
        Q_EMIT e->txResourceFound("ani_riff", base_index, resource_size, "");
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
    Q_EMIT e->txResourceFound("mid", base_index, resource_size, info);
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
        Q_EMIT e->txResourceFound("lbm", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x43464941: // "AIFC" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        Q_EMIT e->txResourceFound("aif", base_index, resource_size, "");
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
        Q_EMIT e->txResourceFound("aif", base_index, resource_size, info);
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
        Q_EMIT e->txResourceFound("lbm", base_index, resource_size, info);
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
        Q_EMIT e->txResourceFound("lbm", base_index, resource_size, info);
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
        Q_EMIT e->txResourceFound("lbm", base_index, resource_size, info);
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
        Q_EMIT e->txResourceFound("lbm", base_index, resource_size, info);
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x50454544: // "DEEP" picture
    {
        if ( ( !e->selected_formats[lbm_id] ) ) return 0;
        Q_EMIT e->txResourceFound("lbm", base_index, resource_size, "");
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
        Q_EMIT e->txResourceFound("xmi", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x58565338: // "8SVX" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        Q_EMIT e->txResourceFound("aif", base_index, resource_size, "");
        e->resource_offset = base_index;
        return resource_size;
    }
    case 0x56533631: // "16SV" sound
    {
        if ( ( !e->selected_formats[aif_id] ) ) return 0;
        Q_EMIT e->txResourceFound("aif", base_index, resource_size, "");
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
    Q_EMIT e->txResourceFound("pcx", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(FileHeader) + 1; // где +1 на u8i separator
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
    Q_EMIT e->txResourceFound("gif", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(JFIF_Header);
    static const QSet <u16i> VALID_VERSIONS  { 0x0100, 0x0101, 0x0102 };
    static const QSet <u8i> VALID_UNITS { 0x00, 0x01, 0x02 };
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset; // base offset (индекс в массиве)
    auto buffer = e->mmf_scanbuf;
    s64i file_size = e->file_size;
    auto info_header = (JFIF_Header*)(&buffer[base_index]);
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
    if ( last_index <= base_index ) return 0;
    u64i resource_size = last_index - base_index;

    // QImage image;
    // bool valid = image.loadFromData(e->mmf_scanbuf + base_index, resource_size);
    // qInfo() << "\njpeg is:" << valid;

    Q_EMIT e->txResourceFound("jpg", base_index, resource_size, "");
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
    // +0 - для 'M.K.'
    // +1 - для 'xCHN'
    // +2 - для 'xxCH'
    u64i offset_correction = 0;
    u64i channels = 4;             // это стандартное значение для M.K.-модуля (далее может меняться)
    u64i steps_in_pattern = 64;    // это стандартное значение для M.K.-модуля
    u64i one_note_size = 4;        // это стандартное значение для M.K.-модуля
    if ( *((u16i*)&buffer[base_index]) == 0x4843 )
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
    Q_EMIT e->txResourceFound("mod", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(XM_Header);
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
    Q_EMIT e->txResourceFound("xm", base_index, resource_size, info);
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
    Q_EMIT e->txResourceFound("s3m", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(IT_Header);
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
    Q_EMIT e->txResourceFound("it", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(BINK_Header);
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
            Q_EMIT e->txResourceFound("bik", base_index, resource_size, info);
        }
        else
        {
            return 0;
        }
        break;
    case '2':
        if ( e->selected_formats[bink2_id] )
        {
            Q_EMIT e->txResourceFound("bk2", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(SMK_Header);
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
    Q_EMIT e->txResourceFound("smk", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

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
    static constexpr u64i min_room_need = sizeof(TIF_Header);
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
                }
            }
            else // иначе, если данные помещаются в 4 байта, то поле data_offset хранит не смещение, а значение тега (например для тега 273 в data_offset хранится начало изображения размером из тега data_offset 279)
            {
                if ( tag_pointer[tag_idx].tag_id == 273 ) ifd_image_offset = tag_pointer[tag_idx].data_offset;
                if ( tag_pointer[tag_idx].tag_id == 279 ) ifd_image_size = tag_pointer[tag_idx].data_offset;
                if ( tag_pointer[tag_idx].tag_id == 34665 ) ifd_exif_offset = tag_pointer[tag_idx].data_offset; // бывает встречается тег "EXIF IFD" - это смещение на таблицу exif-данных, которая имеет структуру обычного IFD
            }
            // qInfo() << "   tag#"<< tag_idx << " tag_id:" << tag_pointer[tag_idx].tag_id << " data_type:" << tag_pointer[tag_idx].data_type << " data_count:" << tag_pointer[tag_idx].data_count << " multiplier:" << VALID_DATA_TYPE[tag_pointer[tag_idx].data_type] << " data_offset:" <<  tag_pointer[tag_idx].data_offset << " result_data_size:" << result_tag_data_size;
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
    if ( ifds_and_tags_db.count() == 0 ) return 0; // проверка на всякий случай
    last_index = base_index + ifds_and_tags_db.lastKey() + ifds_and_tags_db[ifds_and_tags_db.lastKey()];
    if ( last_index > file_size ) return 0;
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound("tif_ii", base_index, resource_size, "");
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
    static constexpr u64i min_room_need = sizeof(TIF_Header);
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
            //if ( e->scanbuf_offset == 437086454 ) qInfo() << "   result_tag_data_size:" << result_tag_data_size;
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
                }
            }
            else // иначе, если данные помещаются в 4 байта, то поле data_offset хранит не смещение, а значение тега (например для тега 273 в data_offset хранится начало изображения размером из тега data_offset 279)
            {
                if ( be2le(tag_pointer[tag_idx].tag_id) == 273 ) ifd_image_offset = be2le(tag_pointer[tag_idx].data_offset);
                if ( be2le(tag_pointer[tag_idx].tag_id) == 279 ) ifd_image_size = be2le(tag_pointer[tag_idx].data_offset);
                if ( be2le(tag_pointer[tag_idx].tag_id) == 34665 ) ifd_exif_offset = be2le(tag_pointer[tag_idx].data_offset); // бывает встречается тег "EXIF IFD" - это смещение на таблицу exif-данных, которая имеет структуру обычного IFD
            }
            // qInfo() << "   tag#"<< tag_idx << " tag_id:" << be2le(tag_pointer[tag_idx].tag_id) << " data_type:" << be2le(tag_pointer[tag_idx].data_type)
            //          << " data_count:" << be2le(tag_pointer[tag_idx].data_count) << " multiplier:" << VALID_DATA_TYPE[be2le(tag_pointer[tag_idx].data_type)]
            //          << " data_offset:" <<  be2le(tag_pointer[tag_idx].data_offset) << " result_data_size:" << result_tag_data_size;
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
    if ( ifds_and_tags_db.count() == 0 ) return 0; // проверка на всякий случай
    last_index = base_index + ifds_and_tags_db.lastKey() + ifds_and_tags_db[ifds_and_tags_db.lastKey()];
    if ( last_index > file_size ) return 0;
    u64i resource_size = last_index - base_index;
    //qInfo() << " last_offset:" << ifds_and_tags_db.lastKey() << " last_block:" << ifds_and_tags_db[ifds_and_tags_db.lastKey()] << "  resource_size:" << resource_size;
    //qInfo() << ifds_and_tags_db;
    Q_EMIT e->txResourceFound("tif_mm", base_index, resource_size, "");
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
    static constexpr u64i min_room_need = sizeof(FLC_Header) - sizeof(FLC_Header::file_size);
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
    auto info = QString("%1x%2 %3-bpp").arg(QString::number(info_header->width),
                                            QString::number(info_header->height),
                                            QString::number(info_header->pix_depth));
    Q_EMIT e->txResourceFound("flc", base_index, resource_size, info);
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
#pragma pack(pop)
    //qInfo() << " !!! 669 RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static constexpr u64i min_room_need = sizeof(_669_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (_669_Header*)(&buffer[base_index]);
    if ( ( info_header->smpnum == 0 ) or ( info_header->patnum == 0 ) ) return 0; // жесткая проверка (т.к. в теории могут быть модули и без сэмплов и без паттернов), но вынужденная, иначе много ложных срабатываний
    if ( info_header->smpnum > 64 ) return 0; // согласно описанию
    if ( info_header->patnum > 128 ) return 0; // согласно описанию
    for (u8i idx = 0; idx < sizeof(_669_Header::song_name); ++idx) // проверка на неподпустимые символы в названии песни
    {
        if ( ( info_header->song_name[idx] != 0 ) and ( info_header->song_name[idx] < 32 )) return 0;
        if ( ( info_header->song_name[idx] != 0 ) and ( info_header->song_name[idx] > 126 )) return 0;
    }
    s64i file_size = e->file_size;
    if ( base_index + sizeof(_669_Header) + sizeof(SampleHeader) * info_header->smpnum + info_header->patnum * 1536 > file_size) return 0; // где 1536 - размер одного паттерна
    u64i last_index = base_index + sizeof(_669_Header); // переставляем last_index на первый заголовок сэмпла
    SampleHeader *sample_header;
    u64i samples_block = 0; // суммарный размер тел сэмплов
    for (u8i idx = 0; idx < info_header->smpnum; ++idx)
    {
        if ( last_index + sizeof(SampleHeader) > file_size ) return 0; // не хватает места для заголовка сэмпла
        sample_header = (SampleHeader*)&buffer[last_index];
        //qInfo() << " sample_id:" << idx << " sample_size:" << sample_header->sample_size << " loop_begin:" << sample_header->loop_begin << " loop end:" << sample_header->loop_end;
        // сигнатура 'if' очень часто встречается, поэтому формат CAT_PERFRISK.
        // нужны тщательные проверки :
        if ( sample_header->loop_begin > sample_header->sample_size ) return 0;
        if ( ( sample_header->loop_end > sample_header->sample_size ) and ( sample_header->loop_end != 0xFFFFF) ) return 0;
        if ( sample_header->loop_begin > sample_header->loop_end ) return 0;
        for (u8i sub_idx = 0; idx < sizeof(SampleHeader::sample_name); ++idx) // проверка на неподпустимые символы в названиях сэмплов
        {
            if ( ( sample_header->sample_name[sub_idx] != 0 ) and ( sample_header->sample_name[sub_idx] < 32 )) return 0;
            if ( ( sample_header->sample_name[sub_idx] != 0 ) and ( sample_header->sample_name[sub_idx] > 126 )) return 0;
        }
        //
        samples_block += sample_header->sample_size;
        last_index += sizeof(SampleHeader); // переставляем last_index на следующий заголовок сэмпла (или начало паттернов, если завершение цикла)
    }
    //qInfo() << " summary sample bodies size:" << samples_block;
    last_index += (info_header->patnum * 1536 + samples_block);
    if ( last_index > file_size ) return 0;
    u64i resource_size = last_index - base_index;
    int song_name_len;
    for (song_name_len = 0; song_name_len < 32; ++song_name_len) // определение длины song name; не использую std::strlen, т.к не понятно всегда ли будет 0 на последнем индексе [31]
    {
        if ( info_header->song_name[song_name_len] == 0 ) break;
    }
    auto info = QString("song '%1'").arg(QString(QByteArray((char*)(info_header->song_name), song_name_len)));
    Q_EMIT e->txResourceFound("669", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(AU_Header);
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
    auto info = QString("%1Hz %2-ch").arg(  QString::number(be2le(info_header->sample_rate)),
                                            QString::number(be2le(info_header->channels)));
    Q_EMIT e->txResourceFound("au", base_index, resource_size, info);
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
#pragma pack(pop)
    //qInfo() << " !!! VOC RECOGNIZER CALLED !!!";
    static constexpr u64i min_room_need = sizeof(VOC_Header);
    static const QSet <u16i> VALID_VERSION { 0x0100, 0x010A, 0x0114 };
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
    while(true)
    {
        if ( last_index + 1 > file_size) return 0; // нет места на тип блока
        if ( buffer[last_index] == 0 ) break; // достигли терминатора
        if ( last_index + sizeof(DataBlock) > file_size ) break; // нет места на очередной data block -> вероятно достигли конца
        data_block = (DataBlock*)&buffer[last_index];
        if ( ( data_block->type_with_size & 0xFF ) > 9 ) return 0; // неизвестный тип блока
        //qInfo() << " data_block type:" << (data_block->type_with_size & 0xFF);
        last_index += ((data_block->type_with_size >> 8) + sizeof(DataBlock)); // сдвигаем вправо на 8, чтобы затереть type и оставить только size
    }
    last_index += 1; // на размер терминатора
    if ( last_index > file_size) return 0;
    u64i resource_size = last_index - base_index;
    if ( resource_size == (sizeof(VOC_Header) + 1) ) return 0; // короткий ресурс без аудио-данных не имеет смысла
    Q_EMIT e->txResourceFound("voc", base_index, resource_size, "");
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
    Q_EMIT e->txResourceFound(mp4_flag ? "mp4_qt" : "mov_qt", base_index, resource_size, "");
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
    static constexpr u64i min_room_need = sizeof(TGA_Header);
    static const QSet <u8i> VALID_PIX_DEPTH { 16, 24, 32 };
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
    last_index += 4096; // накидываем ещё 4K : вдруг это TGA v2? у которого есть области расширения и разработчика.
    if ( last_index > e->file_size ) last_index = e->file_size;
    u64i resource_size = last_index - base_index;
    auto info = QString("%1x%2 %3-bpp").arg(QString::number(info_header->width),
                                            QString::number(info_header->height),
                                            QString::number(info_header->pix_depth));
    Q_EMIT e->txResourceFound("tga_tc", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(ICO_Header);
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
    for (u16i idx = 0; idx < info_header->count; ++idx)
    {
        if ( last_index + sizeof(Directory) > file_size) return 0; // не хватает места на Directory
        //qInfo() << "idx:" << idx;
        directory = (Directory*)&buffer[last_index];
        if ( directory->reserved != 0 ) return 0;
        if ( ( !is_cur ) and ( directory->cpl_xhotspot > 1 ) ) return 0;
        db[directory->bitmap_offset] = directory->bitmap_size;
        //qInfo() << "bitmap_offset:" << base_index + directory->bitmap_offset << " bitmap_size:" << directory->bitmap_size;
        //qInfo() << "last_index:" << last_index;
        if ( base_index + directory->bitmap_offset + sizeof(BitmapInfoHeader) > file_size ) return 0; // нет места на bitmap-заголовок
        auto bitmap_ih = (BitmapInfoHeader*)&buffer[base_index + directory->bitmap_offset];
        if ( !VALID_BMP_HEADER_SIZE.contains(bitmap_ih->bitmapheader_size) ) // тогда может быть есть встроенный PNG?
        {
            if ( bitmap_ih->bitmapheader_size != 0x474E5089 ) return 0;
            //qInfo() << " ----> nested png here!!!!";
        }
        else
        {
            if ( !VALID_BITS_PER_PIXEL.contains(bitmap_ih->bits_per_pixel) ) return 0;
        }
        last_index += sizeof(Directory);
    }
    //qInfo() << "most far offset:" << db.lastKey() << " bitmap size:" << db[db.lastKey()];
    if ( db.lastKey() < sizeof(ICO_Header) + sizeof(Directory) ) return 0;
    last_index = base_index + db.lastKey() + db[db.lastKey()];
    if ( last_index > file_size ) return 0;
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound((is_cur) ? "cur_win" : "ico_win", base_index, resource_size, "");
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
            u64i id3v2_size = id3v2->size_as_bytes[3] | (u64i(id3v2->size_as_bytes[2]) << 7) | (u64i(id3v2->size_as_bytes[1]) << 14) | (u64i(id3v2->size_as_bytes[0]) << 21) + sizeof(ID3v2);
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
        padding = (frame_header->EEEEFFGH >> 1) & 0b00000001;
        crc_len = 2 * (frame_header->AAABBCCD & 0b00000001);
        //qInfo() << " sample_rate:" << sample_rate << " bit_rate:" << bit_rate << " padding:" << padding << " crc_len:" << crc_len << " last_index:" << last_index << " frame_size:" << ( (1152 * bit_rate / 8) / sample_rate + padding );
        last_index += ( (1152 * bit_rate / 8) / sample_rate + padding );
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
    QString info;
    if ( file_size - last_index >= sizeof(ID3v1) ) // может осталось место под ID3v1 в конце ресурса?
    {
        auto id3v1 = (ID3v1*)&buffer[last_index];
        if ( ( id3v1->signature1 == 0x4154 ) and ( id3v1->signature2 == 0x47 ) )
        {
            info = "song '";
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
    Q_EMIT e->txResourceFound("mp3", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
}

RECOGNIZE_FUNC_RETURN Engine::recognize_ogg RECOGNIZE_FUNC_HEADER
{ // https://xiph.org/ogg/doc/framing.html
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
#pragma pack(pop)
    //qInfo() << " !!! OGG RECOGNIZER CALLED !!!" << e->scanbuf_offset;
    static constexpr u64i min_room_need = sizeof(PageHeader);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    PageHeader *page_header;
    u64i last_index = base_index;
    s64i file_size = e->file_size;
    u64i pages = 0;
    while(true)
    {
        if ( last_index + sizeof(PageHeader) > file_size ) break;// есть место для очередного PageHeader?
        page_header = (PageHeader*)(&buffer[last_index]);
        if ( page_header->signature != 0x5367674F /*'OggS'*/) break;
        if ( last_index + sizeof(PageHeader) + page_header->segments_num > file_size ) break;// есть ли место на список размеров сегментов? он идёт сразу после PageHeader
        u32i segments_block = 0;
        for (u8i segm_idx = 0; segm_idx < page_header->segments_num; ++segm_idx) // высчитываем размер текущего блока тел сегментов
        {
            segments_block += buffer[last_index + sizeof(PageHeader) + segm_idx];
        }
        if ( last_index + sizeof(PageHeader) + page_header->segments_num + segments_block > file_size ) break;// есть ли место под блок тел сегментов?
        last_index += (sizeof(PageHeader) + page_header->segments_num + segments_block);
        ++pages;
    }
    if ( pages < 2 ) return 0;
    u64i resource_size = last_index - base_index;
    Q_EMIT e->txResourceFound("ogg", base_index, resource_size, "");
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
    static constexpr u64i min_room_need = sizeof(MED_Header);
    if ( !e->enough_room_to_continue(min_room_need) ) return 0;
    u64i base_index = e->scanbuf_offset;
    auto buffer = e->mmf_scanbuf;
    auto info_header = (MED_Header*)&buffer[base_index];
    u64i resource_size = be2le(info_header->mod_len);
    s64i file_size = e->file_size;
    if ( file_size - base_index < resource_size ) return 0; // не помещается
    // далее грубые проверки (без учёта размеров блоков по указателям)
    if ( (( be2le(info_header->song_ptr) >= resource_size ) or ( be2le(info_header->song_ptr) < sizeof(MED_Header) )) and ( be2le(info_header->song_ptr) != 0 ) ) return 0;
    if ( (( be2le(info_header->blockarr_ptr) >= resource_size ) or ( be2le(info_header->blockarr_ptr) < sizeof(MED_Header) )) and ( be2le(info_header->blockarr_ptr) != 0 ) ) return 0;
    if ( (( be2le(info_header->smplarr_ptr) >= resource_size ) or ( be2le(info_header->smplarr_ptr) < sizeof(MED_Header) )) and ( be2le(info_header->smplarr_ptr) != 0 ) ) return 0;
    if ( (( be2le(info_header->expdata_ptr) >= resource_size ) or ( be2le(info_header->expdata_ptr) < sizeof(MED_Header) )) and ( be2le(info_header->expdata_ptr) != 0 ) ) return 0;
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
    Q_EMIT e->txResourceFound("med", base_index, resource_size, info);
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
    static constexpr u64i min_room_need = sizeof(DBM0_Header);
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
    Q_EMIT e->txResourceFound("dbm0", base_index, resource_size, info);
    e->resource_offset = base_index;
    return resource_size;
};
