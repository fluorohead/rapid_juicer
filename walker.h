#ifndef WALKER_H
#define WALKER_H

#include "task.h"
#include "engine.h"
#include "settings.h"
#include <QThread>
#include <QMutex>

class SessionWindow;

class WalkerThread: public QThread
{
    Q_OBJECT
    Task walker_task;              // собственные копии, не зависящие от глобальных
    Config walker_config;          // ..
    QSet<QString> my_formats;      // ..
    SessionWindow *my_receiver;
    QMutex *walker_control_mutex;
    Engine *engine;
    WalkerCommand command {WalkerCommand::Run};
    s64i previous_msecs; // должно быть инициализировано значением QDateTime::currentMSecsSinceEpoch(); до первого вызова update_general_progress()
    bool *selected_formats_fast; // массив будет построен в prepare_struct..(), для последующего быстрого lookup'а внутри recognizer'ов

    void prepare_structures_before_engine(); // подготавливает selected_formats_fast
    void clean_structures_after_engine();    // освобождает selected_formats_fast
    bool is_excluded_extension(const QString &path); // расширение файла в списке исключаемых?
public:
    WalkerThread(SessionWindow* window_receiver, QMutex* control_mtx, Task *task, const Config &config, const QSet<QString> &formats_to_scan);
    ~WalkerThread();
    void run();
Q_SIGNALS:
    void txImPaused();
    void txImResumed();
    void txFileWasSkipped();

    friend SessionWindow;
    friend Engine;
};

#endif // WALKER_H
