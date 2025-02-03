#include "walker.h"
#include "session.h"
#include <QDebug>
#include <QDir>

extern QMap <QString, FileFormat> fformats;

WalkerThread::WalkerThread(SessionWindow *receiver, QMutex *control_mtx, const Task &task, const Config &config, const QSet<QString> &formats_to_scan)
    : my_receiver(receiver)
    , walker_control_mutex(control_mtx)
    , walker_task(task)
    , walker_config(config)
    , my_formats(formats_to_scan)
{
    //qInfo() << " my_formats:" << my_formats;
}

WalkerThread::~WalkerThread()
{
    delete engine;
    delete walker_control_mutex;
//    qInfo() << "WalkerThread object was destroyed in thread id:" << currentThreadId();
}

// inline void WalkerThread::update_general_progress(int paths_total, int current_path_index)
// {
//     s64i current_msecs = QDateTime::currentMSecsSinceEpoch();
//     u64i general_progress = ( current_path_index == paths_total ) ? 100 : ((current_path_index + 1) * 100) / paths_total; // там, где current_path_index+1 - т.к. это индекс и он начинается с 0
//     if ( ( current_msecs < previous_msecs ) or ( current_msecs - previous_msecs >= MIN_SIGNAL_INTERVAL_MSECS ) )
//     {
//         previous_msecs = current_msecs;
//         Q_EMIT txGeneralProgress("", general_progress);
//         return;
//     }
// }

void WalkerThread::prepare_structures_before_engine()
{
    selected_formats_fast = new bool[fformats.size()];
    // заполнение массива selected_formats_fast (будет использоваться в recognizer'ах
    // для быстрого лукапа, вместо использования my_formats.contains(), который гораздо медленнее;
    for (const auto & [key, val] : fformats.asKeyValueRange())
    {
        selected_formats_fast[val.index] = this->my_formats.contains(key);
        //qInfo() << " inside 'for': key:" << key << " val:" << val.index << " fast bool:" << selected_formats_fast[val.index];
    }
}

void WalkerThread::clean_structures_after_engine()
{
    delete [] selected_formats_fast;
}

void WalkerThread::run()
{
    prepare_structures_before_engine();

    engine = new Engine(this);
    connect(engine, &Engine::txResourceFound, my_receiver, &SessionWindow::rxResourceFound, Qt::QueuedConnection); // исполнение слота в основном потоке
    //connect(this, &WalkerThread::txGeneralProgress, my_receiver, &SessionWindow::rxGeneralProgress, Qt::QueuedConnection); // слот будет исполняться в основном потоке
    connect(engine, &Engine::txFileChange, my_receiver, &SessionWindow::rxFileChange, Qt::QueuedConnection); // слот будет исполняться в основном потоке
    connect(engine, &Engine::txFileProgress, my_receiver, &SessionWindow::rxFileProgress, Qt::QueuedConnection); // слот будет исполняться в основном потоке

    previous_msecs = QDateTime::currentMSecsSinceEpoch();

    int tp_count = walker_task.task_paths.count();
    int general_progress;

    //qInfo() << "formats selected" << settings.selected_formats;

    for (int tp_idx = 0; tp_idx < tp_count; ++tp_idx) // проход по списку путей
    {
        if ( walker_task.task_paths[tp_idx].file_mask.isEmpty() )  // если .filter пуст, значит это файл
        {
            //qInfo() << "thread" << currentThreadId() << "(" << QThread::currentThread() << ") : main \"for\" : scanning file \""<< walker_task.task_paths[tp_idx].path;

            /////  запуск поискового движка
            //s64i fix_msecs = QDateTime::currentMSecsSinceEpoch();
            engine->scan_file_win64(walker_task.task_paths[tp_idx].path);
            //qInfo() << "scan_file worked for:" << (QDateTime::currentMSecsSinceEpoch() - fix_msecs) << "msecs";
            if ( engine->done_cause_skip ) Q_EMIT txFileWasSkipped(); // обновляем current_status
            /////

            ///// анализ причин завершения функции scan_file()
            if ( command == WalkerCommand::Stop ) // функция scan_file завершилась по команде Stop => выходим из главного for
            {
                //qInfo() << " >>>> WalkerThread : received Stop command, when Engine was running!";
                tp_idx = tp_count; // чтобы выйти из главного for
            }
            ////////////////////////////////////////////////
        }
        else // если .filter не пуст, значит это каталог
        {
            //qInfo() << "thread" << currentThreadId() << "(" << QThread::currentThread() << ") : scanning folder \""<< walker_task.task_paths[tp_idx].path << "; recursion:" << walker_task.task_paths[tp_idx].recursion;
            std::function<void(const QString&, const QString&)> dir_walking = [&](const QString &dir, const QString &sub_dir) // так объявляется рекурсивная лямбда; обычные можно через auto
            {
                QDir current_dir {dir + QDir::separator() + sub_dir};
                if ( current_dir.exists() && current_dir.isReadable() )
                {
                    current_dir.setSorting(QDir::Name);
                    current_dir.setNameFilters({walker_task.task_paths[tp_idx].file_mask}); // mask
                    current_dir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden | QDir::System);

                    QFileInfoList file_infolist = current_dir.entryInfoList(); // получение списка файлов в подкаталоге

                    //qInfo() << " > Subfolder" << current_dir.absolutePath() << "has" << file_infolist.count() << "files";

                    int fil_count = file_infolist.count();
                    // проход по файлам внутри каталога
                    for (int idx = 0; idx < fil_count; ++idx)
                    {
                        //qInfo() << "thread" << currentThreadId() << "(" << QThread::currentThread() << ") : \"for\" in lambda : scanning file \""<< file_infolist[idx].absoluteFilePath();

                        /////  запуск поискового движка
                        //  s64i fix_msecs = QDateTime::currentMSecsSinceEpoch();
                        engine->scan_file_win64(file_infolist[idx].absoluteFilePath());
                        //qInfo() << "scan_file worked for:" << (QDateTime::currentMSecsSinceEpoch() - fix_msecs) << "msecs";
                        if ( engine->done_cause_skip ) Q_EMIT txFileWasSkipped(); // обновляем current_status
                        /////

                        if ( command == WalkerCommand::Stop ) // функция scan_file завершилась по команде Stop => выходим из while (и потом сразу из lambda for, а потом сразу из главного for)
                        {
                            //qInfo() << " >>>> WalkerThread : received Stop command, when Engine was running! (in lambda)";
                            tp_idx = tp_count; // а потому сразу и из главного for
                            return;
                        }
                        ////////////////////////////////////////////////
                    }

                    if ( walker_task.task_paths[tp_idx].recursion )
                    {
                        current_dir.setNameFilters({"*"}); // для каталогов применяем маску * вместо маски файлов *.*
                        current_dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden | QDir::System);

                        QFileInfoList dir_infolist = current_dir.entryInfoList(); // получение списка подкаталогов в каталоге

                        int dil_count = dir_infolist.count();
                        //qInfo() << " > Subfolder" << current_dir.absolutePath() << "has" << dir_infolist.count() << "subdirectories";
                        // обход подкаталогов
                        for (int idx = 0; idx < dil_count; ++idx)
                        {
                            switch (command)
                            {
                            case WalkerCommand::Run:
                                dir_walking(current_dir.absolutePath(), dir_infolist[idx].fileName());
                                break;
                            case WalkerCommand::Stop:
                                //qInfo() << ">>>> WalkerThread : received Stop command in dir_walking \"for\", when recursing into subdirs";
                                tp_idx = tp_count; // чтобы выйти из главного for
                                return;
                            case WalkerCommand::Pause:
                                //qInfo() << ">>>> WalkerThread : received Pause command in dir_walking \"for\", when resursing into subdirs, due to mutex lock";
                                Q_EMIT txImPaused();
                                walker_control_mutex->lock(); // повисаем на этой строке (mutex должен быть предварительно заблокирован в вызывающем коде)
                                walker_control_mutex->unlock(); // сюда, если mutex разблокирован в вызывающем коде
                                if ( command == WalkerCommand::Stop) // вдруг пока мы стояли на паузе, была нажата кнопка Stop
                                {
                                    tp_idx = tp_count; // чтобы выйти из главного for
                                    return;
                                }
                                Q_EMIT txImResumed();
                                //qInfo() << ">>>> WalkerThread : received Resume(Run) command in dir_walking \"for\", when resursing into subdirs, due to mutex unlock";
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
            dir_walking(walker_task.task_paths[tp_idx].path, "");
            /////
        }
        // обновляем графику для general progress
        //update_general_progress(tp_count, tp_idx);
    }

    // финальный аккорд, обнуляем графику для file progress
    Q_EMIT engine->txFileChange("", 0);
    Q_EMIT engine->txFileProgress(0);

    clean_structures_after_engine();
}
