#ifndef ENGINE_H
#define ENGINE_H

#include "formats.h"
#include <QObject>
#include <QMutex>
#include <QFile>

#define MIN_SIGNAL_INTERVAL_MSECS 20

#define MIN_RESOURCE_SIZE 32 // должно быть кратно MAX_SIGN_SIZE
#define MAX_SIGNATURE_SIZE 4
#define AUX_BUFFER_SIZE 512

#define RECOGNIZE_FUNC_HEADER (Engine *e)
#define RECOGNIZE_FUNC_DECL_RETURN static u64i
#define RECOGNIZE_FUNC_RETURN u64i

class WalkerThread;
class Engine;

enum class WalkerCommand: int {Run = 0, Stop, Pause, Skip};

struct Signature {
    union {
        u64i as_u64i;
        u8i  as_u8i[8];
    };
    u64i signature_size;
    RECOGNIZE_FUNC_RETURN (*recognizer_ptr) RECOGNIZE_FUNC_HEADER; // указатель на функцию-обработчик
};

extern QMap <QString, Signature> signatures;

struct TreeNode {
    Signature signature;
    TreeNode  *left;
    TreeNode  *right;
};

class Engine: public QObject
{
    Q_OBJECT
    WalkerCommand *command;
    QMutex *control_mutex; // тот же мьютекс, что использует WalkerThread и основной поток
    s64i previous_msecs; // должно быть инициализировано значением QDateTime::currentMSecsSinceEpoch(); до первого вызова update_file_progress()
    u64i previous_file_progress;
    WalkerThread *my_walker_parent;
    bool *selected_formats;
    QFile file; // текущий файл
    bool scrupulous;
    s64i file_size; // заполняется в функции scan_file()
    u64i scanbuf_offset; // текущее смещение в буфере
    u64i resource_offset; // начало ресурса; заполняется recognizer'ом, когда ресурс найден (если размер не 0)
    void update_file_progress(const QString &file_name, u64i file_size, s64i total_readed_bytes);
    bool enough_room_to_continue(u64i min_size); // достаточно ли места min_size до конца файла, чтобы проверить заголовок сигнатуры
    uchar *mmf_scanbuf; // memry mapped file scanning buffer
    u64i hits {0};

public:
    Engine(WalkerThread *walker_parent);
    ~Engine();
    void scan_file(const QString &file_name); // asmjit : индексный вектор переходов

    RECOGNIZE_FUNC_DECL_RETURN recognize_special RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_bmp RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_png RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_riff RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_mid RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_iff RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_pcx RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_gif RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_jpg RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_mod RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_xm RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_s3m RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_it RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_bink RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_smk RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_tif_ii RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_tif_mm RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_flc RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_669 RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_au RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_voc RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_mov_qt RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_tga RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_ico_cur RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_mp3 RECOGNIZE_FUNC_HEADER;
    RECOGNIZE_FUNC_DECL_RETURN recognize_ogg RECOGNIZE_FUNC_HEADER;

Q_SIGNALS:
    void txFileProgress(QString file_name, s64i percentage_value);
    void txResourceFound(const QString &format_name, const QString &file_name, s64i file_offset, u64i size, const QString &info);
};

#endif // ENGINE_H
