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

    static const u32i special_signature;

    QFile file; // текущий файл
    int amount_dw;
    int amount_w;
    bool scrupulous;
    u64i read_buffer_size; // 2|10|50 MiB
    u64i total_buffer_size; // read_buffer_size + 4
    u8i *scanbuf_ptr {nullptr}; // главный буфер сканирования размером total_buffer_size
    u8i *auxbuf_ptr  {nullptr}; // вспомогательный буфер для recognizer'ов размером AUX_BUFFER_SIZE
    u8i *fillbuf_ptr {nullptr}; // указатель на главный буфер + 4; наполнение главного буфера всегда происходит с этого смещения, но сканирование с 0
    s64i signature_file_pos {0}; // позиция сигнатуры в файле

    void update_file_progress(const QString &file_name, u64i file_size, s64i total_readed_bytes);
public:
    Engine(WalkerThread *walker_parent);
    ~Engine();
    void scan_file_v1(const QString &file_name); // по dword-сигнатурам через авл-дерево в буфере
    void scan_file_v2(const QString &file_name); // по dword-сигнатурам через линейный поиск в буфере
    void scan_file_v3(const QString &file_name); // по dword-сигнатурам через пачку if'ов

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
    void txFileProgress(QString file_name, u64i percentage_value);
};

#endif // ENGINE_H
