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

    Task my_task;              // собственные копии, не зависящие от глобальных
    Config my_config;          // ..
    QSet<QString> my_formats;  // ..
    SessionWindow *my_receiver;
    QMutex *my_control_mutex;
    Engine *engine;
    WalkerCommand command {WalkerCommand::Run};
    s64i previous_msecs; // должно быть инициализировано значением QDateTime::currentMSecsSinceEpoch(); до первого вызова update_general_progress()
    void update_general_progress(int paths_total, int current_path_index);

    TreeNode *tree_dw {nullptr}; // авл-дерево для dw-сигнатур, указатель на массив TreeNode-элементов
    TreeNode *tree_w  {nullptr}; // авл-дерево для w-сигнатур , указатель на массив TreeNode-элементов
    bool *selected_formats_fast; // массив будет построен в prepare_struct..(), для последующего быстрого lookup'а внутри recognizer'ов
    int amount_dw; // количество сигнатур в дереве
    int amount_w; // количество сигнатур добавленных в массив signs_to_scan_d

    void sort_signatures(Signature **signs_to_scan, int amount); // пузырьковая сортировка сигнатур (по полю .as_u64i, а не по ключу)
    void prepare_avl_tree(TreeNode *tree, Signature **signs_to_scan, s32i low_index, s32i base_index, s32i hi_index, bool left, u32i *idx);
    //void prepare_avl_tree(TreeNode *tree, Signature **signs_to_scan, int low_index, int base_index, int hi_index, bool left, int *idx);
    void print_avl_tree(TreeNode *tree, int amount);
    void prepare_structures_before_engine(); // подготавливает selected_formats_fast, tree_dw, tree_w;
    void clean_structures_after_engine();    // освобождает selected_formats_fast, tree_dw, tree_w;

public:
    WalkerThread(SessionWindow* window_receiver, QMutex* control_mtx, const Task &task, const Config &config, const QSet<QString> &formats_to_scan);
    ~WalkerThread();
    void run();
signals:
    void txGeneralProgress(QString remaining, int percents);
    void txImPaused();
    void txImResumed();

    friend SessionWindow;
};

#endif // WALKER_H
