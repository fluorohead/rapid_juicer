#ifndef ENGINE_H
#define ENGINE_H

#include "formats.h"
#include <QObject>
#include <QMutex>

#define MIN_SIGNAL_INTERVAL_MSECS 20

class WalkerThread;

enum class WalkerCommand: int {Run = 0, Stop, Pause, Skip};

class Engine: public QObject
{
    Q_OBJECT
    WalkerCommand *command;
    QMutex *my_control_mutex; // тот же мьютекс, что использует WalkerThread и основной поток
    s64i previous_msecs; // должно быть инициализировано значением time.msecsSinceStartOfDay() до первого вызова update_file_progress()
    WalkerThread *my_walker_parent;
    void update_file_progress(const QString &file_name, int file_size, int total_readed_bytes);
public:
    Engine(WalkerThread *walker_parent, WalkerCommand *walker_command, QMutex *walker_mutex);
    ~Engine();
    void scan_file(const QString &file_name); // возвращает смещение в файле, где произошёл останов функции, чтобы потом можно было возобновить сканирование с этого места
signals:
    void txFileProgress(QString file_name, int percentage_value);
};

#endif // ENGINE_H
