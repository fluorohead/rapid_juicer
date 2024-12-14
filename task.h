#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QSet>

struct TaskPath
{
    QString  path;
    QString  file_mask; // в случае файла здесь пустая строка ""
    bool     recursion; // для отдельных файлов всегда false
};

class Task
{
public:
    QList <TaskPath> task_paths;

    int paths_count; // поддерживается, чтобы не вызывать каждый раз task_paths.count() из GUI-виджетов
    bool scrupulous;  // заполняется только непосредственно перед запуском сканирования в SessionWindow::create_and_start_walker()

    void addTaskPath(const TaskPath &taskpath);
    void delTaskPath(int index);
    void delAllTaskPaths();
    bool isTaskPathPresent(const QString &path);
    bool getTaskRecursion(int index);
    void setTaskRecursion(int index, bool value);
};

#endif // TASK_H
