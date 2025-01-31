// WALKER WINDOW
#ifndef SESSION_H
#define SESSION_H

#include "formats.h"
#include "walker.h"
#include "mw.h"
#include <QProgressBar>
#include <QMutex>
#include <QStackedWidget>
#include <QMovie>
#include <QCheckBox>

#define MAX_FILENAME_LEN 66

enum class TLV_Type: u8i { SrcFile = 0, FmtChange = 1, POD = 2, DstExtension = 3, Info = 4, Terminator = 5 };

struct ResourceRecord
{
    u64i    order_number; // назначенный порядковый номер (по сути счётчик находок)
    bool    is_selected; // выбран для последующего сохранения?
    u64i    src_fname_idx; // индекс имени файла в списке исходных файлов
    s64i    offset; // смещение в файле
    u64i    size; // размер ресурса
    QString info; // доп. информация о ресурсе
    QString dest_extension; // расширение файла для сохранения
};

//                     |--- наименование формата
//                     |            |--- данные о ресурсах
//                     V            V
using RR_Map = QMap<QString, QList<ResourceRecord>>;

#pragma pack(push,1)
struct POD_ResourceRecord
{
    u64i order_number;
    u64i src_fname_idx;
    s64i offset;
    u64i size;
};

struct TLV_Header
{
    TLV_Type type;
    u64i     length;
};
#pragma pack(pop)

struct TileCoordinates
{
    int row;
    int column;
};


class FormatTile: public QPushButton
{
    Q_OBJECT
    QLabel *counter;
    void update_counter(u64i value);
public:
    FormatTile(const QString &format_name);
    friend SessionWindow;
};


class ResultsByFormat: public QWidget
{
    QString my_fmt;
    QTableWidget *format_table;
    QCommonStyle *fmt_table_style;
    RR_Map *resources_db;
public:
    ResultsByFormat(const QString &your_fmt, RR_Map *db_ptr);
    ~ResultsByFormat();
    void rxInsertNewRecord(ResourceRecord *new_record, int qlist_idx);
};

class FormatCheckBox: public QCheckBox
{
    QString my_fmt;
    RR_Map *resources_db;
    int my_qlist_idx;
public:
    FormatCheckBox(const QString &your_fmt, RR_Map *db_ptr, int qlist_idx);
};

class SessionWindow: public QWidget
{
    Q_OBJECT
    QPointF prev_cursor_pos;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void closeEvent(QCloseEvent *event);
    u32i my_session_id;
    bool is_walker_dead {true}; // начальное true; перед запуском выставляем false; после получения сигнала finished от walker'а выставляется опять true
    bool is_walker_paused {false};
    QPushButton *stop_button;
    QPushButton *pause_resume_button;
    QPushButton *skip_button;
    QPushButton *save_all_button;
    QPushButton *report_button;
    QPushButton *back_button;
    QPushButton *select_all_button;
    QPushButton *unselect_all_button;
    QProgressBar *file_progress_bar;
    QProgressBar *general_progress_bar;
    QLabel *current_file_label;
    QLabel *paths_remaining;
    QLabel *current_status;
    QLabel *total_resources;
    QLabel *movie_zone;
    QMovie *scan_movie;
    QMutex *walker_mutex;
    WalkerThread *walker;
    void create_and_start_walker(); // создание walker-потока и запуск его в работу
    u64i total_resources_found {0}; // счётчик найдённых ресурсов
    u32i unique_formats_found {0}; // счётчик уникальных форматов среди найдённых ресурсов; по нему высчитываются строка/столбец следующего тайла
    QStackedWidget *pages;
    QTableWidget *results_table;
    QMap <QString, u64i> formats_counters;
    QMap <QString, FormatTile*> tiles_db; // ссылки на виджеты тайлов : ключ - формат
    QMap <QString, TileCoordinates> tile_coords; // координаты тайлов (строка, столбец) : ключ - формат
    QString current_file_name;
    u64i resources_in_current_file;
    QStringList src_files;
    RR_Map resources_db;
public:
    SessionWindow(u32i session_id);
    ~SessionWindow();
public Q_SLOTS:
    void rxGeneralProgress(QString remaining, u64i percentage_value);
    void rxFileChange(const QString &file_name);
    void rxFileProgress(s64i percentage_value);
    void rxResourceFound(const QString &format_name, s64i file_offset, u64i size, const QString &info);
    void rxSerializeAndStartSaveProcess();
    void rxChangePageTo(int page);
};


class SessionsPool
{
    SessionWindow **pool;
    u32i _size;
public:
    SessionsPool(u32i size);
    ~SessionsPool();
    u32i get_new_session_id();                                      // вернёт 0, если пул заполнен, в ином случае значение от 1 до MAX_SESSIONS
    u32i write_new_session(SessionWindow* session_window, u32i id); // вернёт 0, если id используется; в ином случае вернёт тот же id, что передавался в аргументах
    u32i remove_session(SessionWindow* session_window, u32i id);    // вернёт 0, если id от 1 до MAX_SESSIONS и по этому id лежит верный указатель
    u32i get_active_count();
    void free_session();
    friend MainWindow;
};


#endif // SESSION_H
