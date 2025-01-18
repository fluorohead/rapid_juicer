#include "task.h"
#include <QDebug>

void Task::addTaskPath(const TaskPath &taskpath)
{
    task_paths.append(taskpath);
    ++paths_count;
}

void Task::delTaskPath(int index)
{
    task_paths.remove(index);
    task_paths.squeeze();
    --paths_count;
}

void Task::delAllTaskPaths()
{
    task_paths.clear();
    task_paths.squeeze();
    paths_count = 0;
}

bool Task::isTaskPathPresent(const QString &path)
{
    for (auto &&one: task_paths) {
        if (one.path == path) {
            //qInfo() << "path is already present";
            return true;
        }
    }
    return false;
}

bool Task::getTaskRecursion(int index)
{
    return task_paths[index].recursion;
}

void Task::setTaskRecursion(int index, bool value)
{
    task_paths[index].recursion = value;
}
