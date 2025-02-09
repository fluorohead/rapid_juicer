#ifndef SAVING_H
#define SAVING_H

#include <QWidget>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QTextEdit>
#include "session.h"

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

    RR_Map resources_db; // основная БД найденных ресурсов
    QStringList src_files; // список исходных файлов
    QMap <QString, u64i> formats_counters;
    u64i total_resources_found {0}; // всего ресурсов найдено
    u64i resources_saved {0}; // счётчик кол-ва сохранённых ресурсов
    QString saving_path;

    QPointF prev_cursor_pos;
    int lang_id;
    bool debug_mode;

    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

    void load_data_from_shm(const QString &shm_key, const QString &shm_size, const QString &ssem_key);
public:
    SavingWindow(const QString &shm_key, const QString &shm_size, const QString &ssem_key, bool is_debug, const QString &language_id);
    ~SavingWindow();
    void start_saver(const QString &path);
public Q_SLOTS:
    void rxNextWasSaved();
    void rxDebugInfo(QString info);
    void rxSaverFinished();
};

#endif // SAVING_H
