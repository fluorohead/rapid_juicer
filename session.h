// WALKER WINDOW
#ifndef SESSION_H
#define SESSION_H

#include "formats.h"
#include "walker.h"
#include "mw.h"
#include <QProgressBar>
#include <QMutex>
#include <QStackedWidget>

#define MAX_FILENAME_LEN 66

struct ResourceRecord
{
    u64i    number; // назначенный порядковый номер
    QString format; // наименование формата
    QString source; // путь исходного файла
    s64i    offset; // смещение в файле
    u64i    size;   // размер ресурса
    QString info;   // доп. информация о ресурсе
    QString dest_extension; // расширение файла для сохранения
};

struct TileCoordinates
{
    int row;
    int column;
};

class FormatTile: public QLabel
{
    QLabel *counter;
    void update_counter(u64i value);
public:
    FormatTile(const QString &format_name);
    friend SessionWindow;
};

class SessionWindow: public QWidget
{
    Q_OBJECT
    QPointF prev_cursor_pos;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void closeEvent(QCloseEvent *event);
    u32i my_session_id;
    bool is_walker_dead   {true}; // начальное true; перед запуском выставляем false; после получения сигнала finished от walker'а выставляется опять true
    bool is_walker_paused {false};
    QPushButton *stop_button;
    QPushButton *pause_resume_button;
    QPushButton *skip_button;
    QPushButton *save_all_button;
    QPushButton *report_button;
    QProgressBar *file_progress_bar;
    QProgressBar *general_progress_bar;
    QLabel *current_file_label;
    QLabel *paths_remaining_label;
    QMutex *walker_mutex;
    WalkerThread *walker;
    void create_and_start_walker(); // создание walker-потока и запуск его в работу
    u64i total_resources_found {0}; // счётчик найдённых ресурсов
    u32i unique_formats_found {0}; // счётчик уникальных форматов среди найдённых ресурсов; из него высчитываются координаты (строка/столбец) следующего тайла
    QStackedWidget *pages;
    QTableWidget *results_table;
    QMap <QString, QList<ResourceRecord>> resources_db; // сюда будет накапливаться информация по найдённым ресурсам : ключ - формат, значение - список ресурсов
    QMap <QString, FormatTile*> tiles_db; // ссылки на виджеты тайлов : ключ - формат
    QMap <QString, TileCoordinates> tile_coords; // координаты тайлов (строка, столбец) : ключ - формат
public:
    SessionWindow(u32i session_id);
    ~SessionWindow();
public Q_SLOTS:
    void rxGeneralProgress(QString remaining, u64i percentage_value);
    void rxFileProgress(QString file_name, s64i percentage_value);
    void rxResourceFound(const QString &format_name, const QString &file_name, s64i file_offset, u64i size, const QString &info);
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
