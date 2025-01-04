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
    void update_general_progress(int paths_total, int current_path_index);

    TreeNode *tree_dw {nullptr}; // авл-дерево для dw-сигнатур, массив TreeNode-элементов
    TreeNode *tree_w  {nullptr}; // авл-дерево для w-сигнатур , массив TreeNode-элементов
    bool *selected_formats_fast; // массив будет построен в prepare_struct..(), для последующего быстрого lookup'а внутри recognizer'ов
    int amount_dw; // количество dw-сигнатур в dw-дереве
    int amount_w; // количество w-сигнатур в w-дереве

    void sort_signatures_array(Signature **signs_to_scan, int amount); // пузырьковая сортировка сигнатур (по полю .as_u64i, а не по ключу)
    void prepare_avl_tree(TreeNode *tree, Signature **signs_to_scan, int first_index, int last_index, int *idx);
    void print_avl_tree(TreeNode *tree, int amount);
    void prepare_structures_before_engine(); // подготавливает selected_formats_fast, tree_dw, tree_w;
    void clean_structures_after_engine();    // освобождает selected_formats_fast, tree_dw, tree_w;

    QSet <QString> uniq_signature_names_dw; // множества для отбора уникальных сигнатур по ключу сигнатур
    QSet <QString> uniq_signature_names_w; // ...

public:
    WalkerThread(SessionWindow* window_receiver, QMutex* control_mtx, const Task &task, const Config &config, const QSet<QString> &formats_to_scan);
    ~WalkerThread();
    void run();
Q_SIGNALS:
    void txGeneralProgress(QString remaining, u64i percents);
    void txImPaused();
    void txImResumed();

    friend SessionWindow;
    friend Engine;
};

#endif // WALKER_H
