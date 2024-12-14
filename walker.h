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
    Task my_task;     // собственная копия, не зависящая от глобальной;
    Config my_config;   // собственная копия, не зависящая от глобальной;
    SessionWindow *my_receiver;
    QMutex *my_control_mutex;
    Engine *engine;
    WalkerCommand command {WalkerCommand::Run};
    s64i previous_msecs;
    void update_general_progress(int paths_total, int current_path_index);
public:
    WalkerThread(SessionWindow* window_receiver, QMutex* control_mtx, const Task &task, const Config &config);
    ~WalkerThread();
    void run();
signals:
    void txGeneralProgress(QString remaining, int percents);
    void txImPaused();
    void txImResumed();

    friend SessionWindow;
};

#endif // WALKER_H
