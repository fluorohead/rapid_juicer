#ifndef SAVING_H
#define SAVING_H

#include <QWidget>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include "session.h"

class SavingWindow: public QWidget
{
    QSharedMemory shared_memory;
    QSystemSemaphore *sys_semaphore;
    RR_Map resources_db; // основная БД найденных ресурсов
    QMap <QString, u64i> formats_counters;
    QStringList src_files;
public:
    SavingWindow(const QString &shm_key, const QString &shm_size, const QString &ssem_key);
};

#endif // SAVING_H
