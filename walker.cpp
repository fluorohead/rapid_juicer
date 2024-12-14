#include "walker.h"
#include "session.h"
#include <QDebug>
#include <QDir>

WalkerThread::WalkerThread(SessionWindow *receiver, QMutex *control_mtx, const Task &task, const Config &config, const QSet<QString> &formats_to_scan)
    : my_receiver(receiver)
    , my_control_mutex(control_mtx)
    , my_task(task)
    , my_config(config)
    , my_formats(formats_to_scan)
{
}

WalkerThread::~WalkerThread()
{
    delete engine;
    delete my_control_mutex;

    qInfo() << "WalkerThread object was destroyed in thread id:" << currentThreadId();
}

inline void WalkerThread::update_general_progress(int paths_total, int current_path_index)
{
    s64i current_msecs = QDateTime::currentMSecsSinceEpoch();
    int general_progress = ( current_path_index == paths_total ) ? 100 : ((current_path_index + 1) * 100) / paths_total; // там, где current_path_index+1 - т.к. это индекс и он начинается с 0
    if ( ( current_msecs < previous_msecs ) or ( current_msecs - previous_msecs >= MIN_SIGNAL_INTERVAL_MSECS ) )
    {
        previous_msecs = current_msecs;
        emit txGeneralProgress("", general_progress);
        return;
    }
}

void WalkerThread::sort_signatures(Signature **signs_to_scan, int amount)
{
    if ( amount > 1 )
    {
        Signature *tmp_sign;
        bool was_swap;
        do
        {
            was_swap = false;
            for (int idx = 0; idx < amount - 1; ++idx)
            {
                if ( signs_to_scan[idx]->as_u64i > signs_to_scan[idx+1]->as_u64i )
                {
                    tmp_sign = signs_to_scan[idx + 1];
                    signs_to_scan[idx + 1] = signs_to_scan[idx];
                    signs_to_scan[idx] = tmp_sign;
                    was_swap = true;
                }
            }
        } while (was_swap);
    }
}

void WalkerThread::prepare_avl_tree(TreeNode *tree, Signature **signs_to_scan, int first_index, int last_index, bool left, int *idx)
{
    int base_index = first_index + (last_index - first_index + 1) / 2;
    tree[*idx] = TreeNode{signs_to_scan[base_index], nullptr, nullptr}; // заполняем новый узел
    TreeNode *we_as_parent = &tree[*idx]; // указатель на нас самих, т.е. на TreeNode, записанный под индексом idx в tree
    (*idx)++; // инкремент ячейки в массиве tree
    if ( last_index != first_index ) // проверка на размер поддерева : если оно из 1 элемента, то это stop_condition для рекурсии
    {
        int new_first_index;
        int new_last_index;
        if ( base_index > first_index ) // значит слева ещё остались элементы
        {
            new_first_index = first_index;
            new_last_index  = base_index - 1;
            we_as_parent->left = &tree[*idx]; // записываем себе в левого потомка следующий TreeNode, который точно будет, потому что base_index > first_index
            prepare_avl_tree(tree, signs_to_scan, new_first_index, new_last_index, true /*left*/, idx);
        }
        if ( base_index < last_index ) // значит справа ещё остались элементы
        {
            new_first_index = base_index + 1;
            new_last_index  = last_index;
            we_as_parent->right = &tree[*idx]; // записываем себе в правого потомка следующий TreeNode, который точно будет, потому что base_index < last_index
            prepare_avl_tree(tree, signs_to_scan, new_first_index, new_last_index, false /*right*/, idx);
        }
    }
}

void WalkerThread::print_avl_tree(TreeNode *tree, int amount)
{
    for (int idx = 0; idx < amount; idx++)
    {
        qInfo() <<  "avl_tree index :" << idx << "; value :" << QString::number(tree[idx].signature_ptr->as_u64i, 16);
        if (tree[idx].left != nullptr)
        {
            qInfo() << "      : left child  :" << QString::number(tree[idx].left->signature_ptr->as_u64i, 16);
        } else {
            qInfo() << "      : left child  : {nullptr}";
        }
        if (tree[idx].right != nullptr)
        {
            qInfo() << "      : right child :" << QString::number(tree[idx].right->signature_ptr->as_u64i, 16);
        } else {
            qInfo() << "      : right child : {nullptr}";
        }
    }
}

void WalkerThread::prepare_structures_before_engine()
{
    int global_signs_num = signatures.size(); // общее количество известных сигнатур

    Signature **signs_to_scan_dw = new Signature*[global_signs_num]; // массив dword-сигнатур для поиска (массив указателей на элементы структуры signatures)
    Signature **signs_to_scan_w  = new Signature*[global_signs_num]; // массив word-сигнатур для поиска  (массив указателей на элементы структуры signatures)
    // ^^^для простоты выделяем памяти на всё кол-во известных сигнатур; реально используемое кол-во будет хранится в amount_(d)w

    QSet <QString> uniq_signature_names_dw; // множества для отбора уникальных сигнатур по ключу сигнатур
    QSet <QString> uniq_signature_names_w; // ...

    selected_formats_fast = new bool[fformats.size()];

    uniq_signature_names_dw.reserve(global_signs_num);
    uniq_signature_names_dw.insert("special");
    uniq_signature_names_w.reserve(global_signs_num);

    tree_dw = new TreeNode[global_signs_num];
    tree_w  = new TreeNode[global_signs_num];
    // ^^^для простоты выделяем памяти на всё кол-во известных сигнатур; реально используемое кол-во будет хранится в amount_(d)w

    // заполнение массива selected_formats_fast (назван fast, потому что будет использоваться в recognizer'ах
    // для быстрого лукапа, вместо использования my_formats.contains(), который гораздо медленнее;
    // и одновременно
    // добавление сигнатур в множества uniq_signs_(d)w, чтобы исключить повторения, т.к.
    // разные форматы могут иметь одну и ту же сигнатуру, например WAV "RIFF" и AVI "RIFF";
    for (const auto & [key, val] : fformats.asKeyValueRange())
    {
        if (this->my_formats.contains(key))
        {
            selected_formats_fast[val.index] = true;
            for (const auto &name: val.signature_ids) // перербор всех сигнатур отдельного формата (у одного формата может быть несколько сигнатур, например у pcx)
            {
                switch(signatures[name].signature_size)
                {
                case 2:
                    uniq_signature_names_w.insert(name);
                    break;
                case 4:
                    uniq_signature_names_dw.insert(name);
                    break;
                }
            }
        }
        else
        {
            selected_formats_fast[val.index] = false;
        }
    }

    qInfo() << "uniq_signature_names_dw:"<< uniq_signature_names_dw;
    qInfo() << "uniq_signature_names_w :"<< uniq_signature_names_w;

    // добавление указателей на сигнатуры в таблицы signs_to_scan_(d)w
    amount_dw = 0;
    amount_w = 0;
    for (const auto &name : uniq_signature_names_dw)
    {
        signs_to_scan_dw[amount_dw] = &signatures[name];
        amount_dw++;
    }
    for (const auto &name : uniq_signature_names_w)
    {
        signs_to_scan_w[amount_w] = &signatures[name];
        amount_w++;
    }

    // сортировка signs_to_scan_(d)w, т.к. для построения авл-дерева массив сигнатур должен быть предварительно упорядочен
    sort_signatures(signs_to_scan_dw, amount_dw);
    sort_signatures(signs_to_scan_w, amount_w);

    // формируем АВЛ-деревья для сигнатур типа dword и word
    int avl_index = 0;
    prepare_avl_tree(tree_dw, signs_to_scan_dw, 0, amount_dw - 1, false, &avl_index); // рекурсивная ф-я, начальный диапазон от [#0 до #n], где #n = кол-во сигнатур - 1
    avl_index = 0;
    prepare_avl_tree(tree_w, signs_to_scan_w, 0, amount_w - 1, false, &avl_index); // рекурсивная ф-я, начальный диапазон от [#0 до #n], где #n = кол-во сигнатур - 1

    print_avl_tree(tree_dw, amount_dw);
    print_avl_tree(tree_w, amount_w);

    // освобождаем массивы signs_to_scan_(d)w, т.к. дерево построили и они больше не нужны
    delete [] signs_to_scan_dw;
    delete [] signs_to_scan_w;
}

void WalkerThread::clean_structures_after_engine()
{
    delete [] selected_formats_fast;
    delete [] tree_dw;
    delete [] tree_w;
}


void WalkerThread::run()
{
    prepare_structures_before_engine();

    engine = new Engine(this, &command, my_control_mutex, my_config.scrupulous, 16);
    previous_msecs = QDateTime::currentMSecsSinceEpoch();

    connect(this, &WalkerThread::txGeneralProgress, my_receiver, &SessionWindow::rxGeneralProgress, Qt::QueuedConnection); // слот будет исполняться в основном потоке
    connect(engine, &Engine::txFileProgress, my_receiver, &SessionWindow::rxFileProgress, Qt::QueuedConnection); // слот будет исполняться в основном потоке

    int tp_count = my_task.task_paths.count();
    int general_progress;

    qInfo() << "formats selected" << settings.selected_formats;

    for (int tp_idx = 0; tp_idx < tp_count; ++tp_idx) // проход по спику путей
    {
        if ( my_task.task_paths[tp_idx].file_mask.isEmpty() )  // если .filter пуст, значит это файл
        {
            qInfo() << "thread" << currentThreadId() << "(" << QThread::currentThread() << ") : main \"for\" : scanning file \""<< my_task.task_paths[tp_idx].path;

            /////  запуск поискового движка
            engine->scan_file(my_task.task_paths[tp_idx].path);
            /////

            ///// анализ причин завершения функции scan_file()
            if ( command == WalkerCommand::Stop ) // функция scan_file завершилась по команде Stop => выходим из главного for
            {
                qInfo() << " >>>> WalkerThread : received Stop command, when Engine was running!";
                tp_idx = tp_count; // чтобы выйти из главного for
            }
            ////////////////////////////////////////////////
        }
        else // если .filter не пуст, значит это каталог
        {
            qInfo() << "thread" << currentThreadId() << "(" << QThread::currentThread() << ") : scanning folder \""<< my_task.task_paths[tp_idx].path << "; recursion:" << my_task.task_paths[tp_idx].recursion;

            std::function<void(const QString&, const QString&)> dir_walking = [&](const QString &dir, const QString &sub_dir) // так объявляется рекурсивная лямбда; обычные можно через auto
            {
                QDir current_dir {dir + QDir::separator() + sub_dir};
                if ( current_dir.exists() && current_dir.isReadable() )
                {
                    current_dir.setSorting(QDir::Name);
                    current_dir.setNameFilters({my_task.task_paths[tp_idx].file_mask}); // mask
                    current_dir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden | QDir::System);

                    QFileInfoList file_infolist = current_dir.entryInfoList(); // получение списка файлов в подкаталоге

                    qInfo() << " > Subfolder" << current_dir.absolutePath() << "has" << file_infolist.count() << "files";

                    int fil_count = file_infolist.count();
                    // проход по файлам внутри каталога
                    for (int idx = 0; idx < fil_count; ++idx)
                    {
                        // qInfo() << " --- ready to scan new file, you have 5 sec to press command button ----";
                        // QThread::msleep(5000);
                        qInfo() << "thread" << currentThreadId() << "(" << QThread::currentThread() << ") : \"for\" in lambda : scanning file \""<< file_infolist[idx].absoluteFilePath();

                        //// тут запуск поискового движка
                        engine->scan_file(file_infolist[idx].absoluteFilePath());
                        if ( command == WalkerCommand::Stop ) // функция scan_file завершилась по команде Stop => выходим из while (и потом сразу из lambda for, а потом сразу из главного for)
                        {
                            qInfo() << " >>>> WalkerThread : received Stop command, when Engine was running! (in lambda)";
                            tp_idx = tp_count; // а потому сразу и из главного for
                            return;
                        }
                        ////////////////////////////////////////////////
                    }

                    if ( my_task.task_paths[tp_idx].recursion )
                    {
                        current_dir.setNameFilters({"*"}); // для каталогов применяем маску * вместо маски файлов *.*
                        current_dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden | QDir::System);

                        QFileInfoList dir_infolist = current_dir.entryInfoList(); // получение списка подкаталогов в каталоге

                        int dil_count = dir_infolist.count();
                        qInfo() << " > Subfolder" << current_dir.absolutePath() << "has" << dir_infolist.count() << "subdirectories";
                        // обход подкаталогов
                        for (int idx = 0; idx < dil_count; ++idx)
                        {
                            switch (command)
                            {
                            case WalkerCommand::Run:
                                dir_walking(current_dir.absolutePath(), dir_infolist[idx].fileName());
                                break;
                            case WalkerCommand::Stop:
                                qInfo() << ">>>> WalkerThread : received Stop command in dir_walking \"for\", when recursing into subdirs";
                                tp_idx = tp_count; // чтобы выйти из главного for
                                return;
                            case WalkerCommand::Pause:
                                qInfo() << ">>>> WalkerThread : received Pause command in dir_walking \"for\", when resursing into subdirs, due to mutex lock";
                                emit txImPaused();
                                my_control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
                                my_control_mutex->unlock(); // сюда, если mutex разблокирован в вызывающем коде
                                if ( command == WalkerCommand::Stop) // вдруг пока мы стояли на паузе, была нажата кнопка Stop
                                {
                                    tp_idx = tp_count; // чтобы выйти из главного for
                                    return;
                                }
                                emit txImResumed();
                                qInfo() << ">>>> WalkerThread : received Resume(Run) command in dir_walking \"for\", when resursing into subdirs, due to mutex unlock";
                                dir_walking(current_dir.absolutePath(), dir_infolist[idx].fileName());
                                break;
                            default:
                                ;
                            }
                        }
                    }
                }
                else
                {
                    qInfo() << "Folder" << current_dir.absolutePath() << "does not exist or is not readable";
                }
            };
            ///// начальный вызов рекурсивной функции
            dir_walking(my_task.task_paths[tp_idx].path, "");
            /////
        }

        // обновляем графику для general progress
        update_general_progress(tp_count, tp_idx);
    }

    // финальный аккорд, обнуляем графику для file progress
    emit engine->txFileProgress("", 0);

    clean_structures_after_engine();
}
