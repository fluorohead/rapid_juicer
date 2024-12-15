#ifndef ENGINE_H
#define ENGINE_H

#include "formats.h"
#include <QObject>
#include <QMutex>

#define MIN_SIGNAL_INTERVAL_MSECS 20

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
    WalkerThread *my_walker_parent;

    bool scrupulous;
    u64i read_buffer_size; // 2|10|50 MiB

    void update_file_progress(const QString &file_name, int file_size, int total_readed_bytes);
public:
    //Engine(WalkerThread *walker_parent, WalkerCommand *walker_command, QMutex *walker_mutex, bool scrupulous_mode, u64i buffer_size);
    Engine(WalkerThread *walker_parent);
    ~Engine();
    void scan_file(const QString &file_name); // возвращает смещение в файле, где произошёл останов функции, чтобы потом можно было возобновить сканирование с этого места

    RECOGNIZE_FUNC_DECL_RETURN recognize_special RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_bmp RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_png RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_riff RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_iff RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_gif RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_tiff_ii RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_tiff_mm RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_tga_tc32 RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_jfif_soi RECOGNIZE_FUNC_HEADER;
    // RECOGNIZE_FUNC_DECL_RETURN recognize_jfif_eoi RECOGNIZE_FUNC_HEADER; // всегда должна возвращать 0 (или 2? обдумать)
signals:
    void txFileProgress(QString file_name, int percentage_value);
};

#endif // ENGINE_H
