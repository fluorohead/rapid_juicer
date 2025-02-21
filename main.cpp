#include "mw.h"
#include "settings.h"
#include "task.h"
#include "formats.h"
#include "session.h"
#include "saving.h"
#include <QApplication>
#include <QDir>
#include <QTextEdit>
#include <QFileDialog>
#include <QFontDatabase>
#include <QDesktopServices>

Settings *settings;
Task *task;
SessionsPool *sessions_pool;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(":: Rapid Juicer :: resource extractor ::");
    app.setWindowIcon(QIcon(":/gui/logo_tiny.png"));
    QFontDatabase::addApplicationFont(":/gui/RobotoMono-Regular.ttf");

    //qInfo() << "Main process id (uint64):" << app.applicationPid();
    // qInfo() << "Main thread id:" << QThread::currentThreadId();

    index_file_formats();

    auto args = app.arguments();
    // [0] - имя файла процесса
    // [1] - команды : "-save" или "-save_dbg"
    // [2] - ключ shm
    // [3] - размер блока shm в байтах
    // [4] - ключ system semaphore
    // [5] - индекс языка (0 - eng, 1 - rus)
    // [6] - имя экрана для отображения
    // [7] - путь сохранения (без подкаталога даты)
    if ( args.count() >= 8 )
    {
        if ( args[1].first(5) == "-save" )
        {
            settings = new Settings;
            settings->initSkin();
            bool is_report = ( args[1] == "-save_report" );
            auto saving_window = new SavingWindow(args[2], args[3], args[4], ( args[1] == "-save_dbg" ) /*is_debug*/, args[5], args[6], is_report);
            QString save_path = args[7];
            if ( save_path[save_path.length() - 1] != '/' ) save_path.append('/');
            if ( !is_report)
            {
                save_path += QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss/");
                QDir save_dir;
                save_dir.mkpath(save_path); // создаём подкаталог
                QDesktopServices::openUrl(QUrl("file:///" + save_path, QUrl::TolerantMode)); // и сразу открываем его в Проводнике
            }
            else
            {
                save_path += ("report_" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".html");
            }
            is_report ? saving_window->start_reporter(save_path) : saving_window->start_saver(save_path);
            saving_window->show();
            app.exec();
            delete settings;
            return 0;
        }
    }

    // auto sim = QImageReader::supportedImageFormats();
    // qInfo() << sim;

    task = new Task;
    sessions_pool = new SessionsPool(MAX_SESSIONS);

    //task->delAllTaskPaths();
    //task.addTaskPath(TaskPath {R"(c:\Games\Borderlands 3 Directors Cut\OakGame\Content\Paks\pakchunk0-WindowsNoEditor.pak)", "", false});
    //task->addTaskPath(TaskPath {R"(c:\Downloads\rj_research\battlefield\fonts\AdobeArabic-Bold.otf)", "", false});
    //task.addTaskPath(TaskPath {R"(C:/Program Files/ASCON/KOMPAS-3D v22/Manual/Exercises)", "*", true});

    settings = new Settings;
    settings->initSkin();

    auto mw = new MainWindow;
    mw->show();
    app.exec();
    delete mw;

    settings->dump_to_file();
    delete settings;
    delete sessions_pool;
    delete task;

    return 0;
}

QString reduce_file_path(const QString &path, int max_len) // передаваемый path должен быть с нативными сепараторами и с именем файла, не просто путь каталога
{
    static const QChar separator = QDir::separator();
    static const QString filler_part = QString(separator) + "..."; //  filler_part = "\..." or "/..."
    static const QString filler_file_name = "[...]";
    static const int min_file_name_len = 10; // если придётся усекать имя файла, то это мин. длина, иначе просто усечём до пустой строки ""; значение должно быть больше длины filler_file_name

    if ( max_len == 0 )
    {
        return "";
    }

    int source_len = (int)path.length();

    if ( ( source_len > max_len ) and ( source_len > 0 ) )
    {
        QStringList parts = path.split(QDir::separator(), Qt::SkipEmptyParts); // side-effect от .split() - удаление символов сепаратора, поэтому их надо обратно восстановить
        for (auto && one_part: parts)
        {
            one_part = separator + one_part; // восстанавливаем символ сепаратора в начале каждой части
        }
#ifdef _WIN64
        parts[0].removeFirst(); // на платформе WIN удаляем сепаратор по индексу 0
#endif
        int parts_count = parts.count();
        auto calc_total_len = [&parts]() // подсчитывает общую длину всех частей вместе
        {
            int total_len = 0;
            for (auto && one_part: parts)
            {
                total_len += one_part.length();
            }
            return total_len;
        };
        auto radical_reducing = [&parts, &parts_count, &calc_total_len, &max_len]() // применяется, когда приходится удалить корень и уменьшить само имя файла
        {
            // пробуем отказаться от корневой части и вставленного filler_part, заменив их на пустые строки ""
            if ( parts_count > 1 )
            {
                parts[0].clear();
            }
            if ( parts_count > 2 )
            {
                parts[1].clear();
            }
            parts.last().removeFirst(); // и сразу удаляем сепаратор в начале имени файла
            if ( calc_total_len() > max_len ) // всё ещё длинный путь? тогда оставляем только имя файла (копируем его в 0 индекс и усекаем лист до единичной длины)
            {
                parts.first() = parts.last();
                parts.resize(1);
                int part0_len = parts[0].length();
                if ( part0_len > max_len )
                {
                    // осталось только имя и оно тоже слишком длинное, тогда усекаем имя файла
                    if ( part0_len >= min_file_name_len )
                    {
                        int number_to_exclude = (part0_len - max_len) + filler_file_name.length(); // = 24
                        int start_idx = part0_len / 2 - number_to_exclude / 2; // 23 / 2 - 24 / 2 = 11 - 12 = - 1
                        if ( start_idx > 0 )
                        {
                            parts[0].remove(start_idx, number_to_exclude);
                            parts[0].insert(start_idx, filler_file_name);
                        }
                        else
                        {
                            parts[0].clear();
                        }
                    }
                    else
                    {
                        parts[0].clear();
                    }
                }
            }
        };

        if ( parts_count > 2 )
        {
            parts[1] = filler_part;
            for (int idx = 2; idx < (parts_count - 1); ++idx) // проходим по частям, начиная с индекса 2, и обнуляем, если не укладываемся в max_len
            {
                if ( calc_total_len() > max_len )
                {
                    parts[idx].clear();
                }
            }
            if ( calc_total_len() > max_len )
            {
                // результат всё ещё длиннее max_len, а иногда он может стать даже длиннее исходной строки,
                // например когда одновременно соблюдаются два условия : удаляемая часть меньше, чем filler_part, и количество частей = 3.
                radical_reducing();
            }
        }
        else
        {
            // если только 2 части, то видимо очень длинное имя файла или корневого каталога, либо и то и другое.
            // тогда придётся сократить имя файла или корневого каталога :(
            radical_reducing();
        }
        return parts.join("");
    }
    return path;
}
