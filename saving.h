#ifndef SAVING_H
#define SAVING_H

#include <QWidget>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QTextEdit>
#include "session.h"

struct FormatStatistics
{
    u64i total_number_of;
    u64i total_size_of;
};

class SavingWindow;

class SaveDBGWindow: public QWidget
{
    QTextEdit *info_text;
public:
    SaveDBGWindow();
    friend SavingWindow;
};

class SaverThread: public QThread
{
    Q_OBJECT
    RR_Map *resources_db;
    QStringList *src_files;
    bool debug_mode;
    QString saving_path;
    SavingWindow *my_parent;
public:
    SaverThread(SavingWindow *parent, RR_Map *resources_db_ptr, QStringList *src_files_ptr, bool is_debug, const QString &path);
    void run();
Q_SIGNALS:
    void txNextWasSaved(); // сигнал "очередной ресурс был сохранён в файл"
    void txDebugInfo(QString info);
};

class ReporterThread: public QThread
{
    Q_OBJECT
    RR_Map *resources_db;
    QStringList *src_files;
    QMap <QString, FormatStatistics> *formats_counters;
    bool debug_mode;
    QString report_path;
    POD_SessionSettings *session_settings;
    SavingWindow *my_parent;
    int lang_id;
public:
    ReporterThread(SavingWindow *parent, RR_Map *resources_db_ptr, QStringList *src_files_ptr, bool is_debug, const QString &path, int language, POD_SessionSettings *ss_ptr, QMap <QString, FormatStatistics> *fmt_counters_ptr);
    void run();
Q_SIGNALS:
    void txNextWasReported(); // сигнал "запись по очередному ресурсу была записана в файл отчёта"
};

class SavingWindow: public QWidget
{
    Q_OBJECT
    SaveDBGWindow *debug_window {nullptr};
    QTextEdit *path_label;
    QLabel *saved_label;
    QLabel *remains_label;
    QProgressBar *progress_bar;
    QPushButton *abort_button;

    QSharedMemory shared_memory;
    QSystemSemaphore *sys_semaphore;

    POD_SessionSettings session_settings;
    QMap <QString, FormatStatistics> formats_counters; // счётчик ресурсов по-форматно
    u64i total_resources_found {0}; // всего ресурсов найдено (подсчёт при декодировании)

    RR_Map resources_db; // основная БД найденных ресурсов
    QStringList src_files; // список исходных файлов (основная БД ссылается сюда по индексу)

    QString saving_path;
    u64i resources_saved {0}; // счётчик кол-ва сохранённых ресурсов
    u64i resources_reported {0}; // счётчик кол-ва отчётов

    QPointF prev_cursor_pos;
    int lang_id;
    bool debug_mode;
    bool report_mode;
    QString screen_name;

    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

    void decode_data(u8i *buffer);
    void load_data_from_shm(const QString &shm_key, const QString &shm_size, const QString &ssem_key);
    void load_data_from_file(); // для отладки, загрузка дампа shm из файла
public:
    SavingWindow(const QString &shm_key, const QString &shm_size, const QString &ssem_key, bool is_debug, const QString &language_id, const QString &screen, bool is_report);
    ~SavingWindow();
    void start_saver(const QString &path);
    void start_reporter(const QString &path);
public Q_SLOTS:
    void rxNextWasSaved();
    void rxNextWasReported();
    void rxDebugInfo(QString info);
    void rxSaverFinished();
    void rxReporterFinished();
};


#endif // SAVING_H
