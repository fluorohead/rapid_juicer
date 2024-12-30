#include "session.h"
#include "task.h"
#include <QMouseEvent>
#include <QEventLoop>
#include <QPushButton>
#include <QThread>
#include <QDir>

extern QString reduce_file_path(const QString&, int);

extern SessionsPool      sessions_pool;
extern Settings          settings;
extern Task              task;
extern const QList<u64i> permitted_buffers;
extern const QString     yes_no_txt[int(Langs::MAX)][2];
extern const QString     mebibytes_txt[int(Langs::MAX)];

const QString properties_header_txt[int(Langs::MAX)]
{
    "Session properties",
    "Свойства сессии"
};

const QString properties_txt[int(Langs::MAX)][4]
{
    { "session id   :", "scrupulous   :", "file mask    :", "read buffer  :" },
    { "номер сессии :", "тщательность :", "маска файлов :", "буфер чтения :" }
};

const QString file_txt[int(Langs::MAX)]
{
    "file  :",
    "файл  :"
};

const QString paths_txt[int(Langs::MAX)]
{
    "paths :",
    "путей :"
};

const QString stop_button_txt[int(Langs::MAX)]
{
    "STOP",
    "СТОП"
};

const QString pause_button_txt[int(Langs::MAX)]
{
    "PAUSE",
    "ПАУЗА"
};

const QString resume_button_txt[int(Langs::MAX)]
{
    "RESUME",
    "ВОЗОБН."
};

const QString skip_button_txt[int(Langs::MAX)]
{
    "SKIP",
    "ПРОП."
};

const QString save_all_button_txt[int(Langs::MAX)]
{
    "SAVE ALL",
    "СОХР.ВСЁ"
};

const QString report_button_txt[int(Langs::MAX)]
{
    "REPORT",
    "ОТЧЁТ"
};

const QString current_progress_header_txt[int(Langs::MAX)]
{
    "Current progress",
    "Текущий прогресс"
};

SessionWindow::SessionWindow(u32i session_id)
    : my_session_id(session_id)
    , QWidget(nullptr, Qt::FramelessWindowHint)  // окно это QWidget без родителя
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setAttribute(Qt::WA_NoSystemBackground);

    QPixmap background {":/gui/session/ssnw.png"};
    auto central_widget = new QLabel(this);
    central_widget->setFixedSize(background.size());
    central_widget->move(0, 0);
    central_widget->setPixmap(background);

    this->resize(central_widget->size()); // размер окна по размеру центрального QLabel

    auto close_button = new OneStateButton(central_widget, ":/gui/session/close.png", ":/gui/session/close_h.png");
    auto minimize_button = new OneStateButton(central_widget, ":/gui/session/min.png", ":gui/session/min_h.png");
    close_button->move(10, 8);
    minimize_button->move(10, 56);

    QFont tmpFont {*skin_font()};
    tmpFont.setPixelSize(13);
    tmpFont.setBold(true);

    auto properties_label = new QLabel; // "Session properties" label
    properties_label->setFixedSize(144, 24);
    properties_label->setStyleSheet("color: #c18556");
    properties_label->setFont(tmpFont);
    properties_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    properties_label->setParent(central_widget);
    properties_label->move(70, 36);
    properties_label->setText(properties_header_txt[curr_lang()]);

    QLabel *tmp_label;
    tmpFont.setBold(false);
    const int y_offset {64};
    for (int idx = 0; idx < 4; ++idx) // рисуем лейблы с наименованиями свойств
    {
        tmp_label = new QLabel;
        tmp_label->setFixedSize(144, 16);
        tmp_label->setStyleSheet("color: #d4e9e9");
        tmp_label->setFont(tmpFont);
        tmp_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tmp_label->setParent(central_widget);
        tmp_label->move(88, y_offset + 22 * idx);
        tmp_label->setText(properties_txt[curr_lang()][idx]);
    }

    QString session_label_texts [4] {   QString("#%1").arg(QString::number(my_session_id), 2, u'0'),
                                        yes_no_txt[curr_lang()][int(settings.config.scrupulous)],
                                        settings.config.file_mask,
                                        QString::number(permitted_buffers[settings.config.bfr_size_idx]) + mebibytes_txt[curr_lang()]
                                    };

    tmpFont.setBold(true);
    tmpFont.setItalic(true);
    tmpFont.setPixelSize(14);
    for (int idx = 0; idx < 4; ++idx) // рисуем лейблы значений свойств
    {
        tmp_label = new QLabel;
        tmp_label->setFixedSize(72, 16);
        tmp_label->setStyleSheet("color: #d9b855");
        tmp_label->setFont(tmpFont);
        tmp_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tmp_label->setParent(central_widget);
        tmp_label->move(208, y_offset + 22 * idx);
        tmp_label->setText(session_label_texts[idx]);
    }

    tmpFont.setItalic(false);
    stop_button = new QPushButton;
    stop_button->setAttribute(Qt::WA_NoMousePropagation);
    stop_button->setFixedSize(64, 32);
    stop_button->setStyleSheet( "QPushButton:enabled  {color: #fffef9; background-color: #ab6152; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:hover    {color: #fffef9; background-color: #8b4132; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:pressed  {color: #fffef9; background-color: #8b4132; border-width: 0px;}"
                                "QPushButton:disabled {color: #a5a5a5; background-color: #828282; border-width: 0px; border-style: solid; border-radius: 14px;}");

    stop_button->setFont(tmpFont);
    stop_button->setDisabled(true);
    stop_button->setText(stop_button_txt[curr_lang()]);
    stop_button->setParent(central_widget);
    stop_button->move(64, 182);

    pause_resume_button = new QPushButton;
    pause_resume_button->setAttribute(Qt::WA_NoMousePropagation);
    pause_resume_button->setFixedSize(64, 32);
    pause_resume_button->setStyleSheet( "QPushButton:enabled  {color: #fffef9; background-color: #7fab6d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                        "QPushButton:hover    {color: #fffef9; background-color: #5f8b4d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                        "QPushButton:pressed  {color: #fffef9; background-color: #5f8b4d; border-width: 0px;}"
                                        "QPushButton:disabled {color: #a5a5a5; background-color: #828282; border-width: 0px; border-style: solid; border-radius: 14px;}");
    pause_resume_button->setFont(tmpFont);
    pause_resume_button->setDisabled(true);
    pause_resume_button->setText(pause_button_txt[curr_lang()]);
    pause_resume_button->setParent(central_widget);
    pause_resume_button->move(139, 182);

    skip_button = new QPushButton;
    skip_button->setAttribute(Qt::WA_NoMousePropagation);
    skip_button->setFixedSize(64, 32);
    skip_button->setStyleSheet( "QPushButton:enabled  {color: #fffef9; background-color: #9ca14e; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:hover    {color: #fffef9; background-color: #7c812e; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:pressed  {color: #fffef9; background-color: #7c812e; border-width: 0px;}"
                                "QPushButton:disabled {color: #a5a5a5; background-color: #828282; border-width: 0px; border-style: solid; border-radius: 14px;}");
    skip_button->setFont(tmpFont);
    skip_button->setDisabled(true);
    skip_button->setText(skip_button_txt[curr_lang()]);
    skip_button->setParent(central_widget);
    skip_button->move(214, 182);

    save_all_button = new QPushButton;
    save_all_button->setAttribute(Qt::WA_NoMousePropagation);
    save_all_button->setFixedSize(96, 32);
    save_all_button->setStyleSheet( "QPushButton:enabled {color: #fffef9; background-color: #7fab6d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:hover   {color: #fffef9; background-color: #5f8b4d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:pressed {color: #fffef9; background-color: #5f8b4d; border-width: 0px;}"
                                    "QPushButton:disabled {color: #a5a5a5; background-color: #828282; border-width: 0px; border-style: solid; border-radius: 14px;}");
    save_all_button->setFont(tmpFont);
    save_all_button->setDisabled(true);
    save_all_button->setText(save_all_button_txt[curr_lang()]);
    save_all_button->setDisabled(true);
    save_all_button->setParent(central_widget);
    save_all_button->move(663, 182);

    report_button = new QPushButton;
    report_button->setAttribute(Qt::WA_NoMousePropagation);
    report_button->setFixedSize(96, 32);
    report_button->setStyleSheet(   "QPushButton:enabled {color: #fffef9; background-color: #7fab6d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:hover   {color: #fffef9; background-color: #5f8b4d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:pressed {color: #fffef9; background-color: #5f8b4d; border-width: 0px;}"
                                    "QPushButton:disabled {color: #a5a5a5; background-color: #828282; border-width: 0px; border-style: solid; border-radius: 14px;}");
    report_button->setFont(tmpFont);
    report_button->setDisabled(true);
    report_button->setText(report_button_txt[curr_lang()]);
    report_button->setDisabled(true);
    report_button->setParent(central_widget);
    report_button->move(771, 182);

    static const QString progress_bar_style_sheet {  "QProgressBar {color: #2f2e29; background-color: #b6c7c7; border-width: 2px; border-style: solid; border-radius: 6px; border-color: #b6c7c7;}"
                                                     "QProgressBar:chunk {background-color: #42982f; border-width: 0px; border-style: solid; border-radius: 6px;}"};

    tmpFont.setPixelSize(12);
    auto progress_label = new QLabel; // "Current progress" label
    progress_label->setFixedSize(144, 24);
    progress_label->setStyleSheet("color: #e1a576");
    progress_label->setFont(tmpFont);
    progress_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    progress_label->setParent(central_widget);
    progress_label->move(320, 36);
    progress_label->setText(current_progress_header_txt[curr_lang()]);

    file_progress_bar = new QProgressBar;
    file_progress_bar->setFixedSize(488, 16);
    file_progress_bar->setFont(tmpFont);
    file_progress_bar->setMinimum(0);
    file_progress_bar->setMaximum(100);
    file_progress_bar->setAlignment(Qt::AlignCenter);
    file_progress_bar->setStyleSheet(progress_bar_style_sheet);
    file_progress_bar->setFormat("%v%");
    file_progress_bar->setParent(central_widget);
    file_progress_bar->move(372, 86);
    file_progress_bar->setValue(0);

    general_progress_bar = new QProgressBar;
    general_progress_bar->setFixedSize(488, 16);
    general_progress_bar->setFont(tmpFont);
    general_progress_bar->setMinimum(0);
    general_progress_bar->setMaximum(100);
    general_progress_bar->setAlignment(Qt::AlignCenter);
    general_progress_bar->setStyleSheet(progress_bar_style_sheet);
    general_progress_bar->setFormat("%v%");
    general_progress_bar->setParent(central_widget);
    general_progress_bar->move(372, 132);
    general_progress_bar->setValue(0);

    tmpFont.setBold(false);
    tmpFont.setPixelSize(13);
    tmp_label = new QLabel; // надпись "file"
    tmp_label->setFixedSize(144, 16);
    tmp_label->setStyleSheet("color: #d4e9e9");
    tmp_label->setFont(tmpFont);
    tmp_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tmp_label->setParent(central_widget);
    tmp_label->move(336, y_offset);
    tmp_label->setText(file_txt[curr_lang()]);

    tmp_label = new QLabel; // надпись "paths"
    tmp_label->setFixedSize(144, 16);
    tmp_label->setStyleSheet("color: #d4e9e9");
    tmp_label->setFont(tmpFont);
    tmp_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tmp_label->setParent(central_widget);
    tmp_label->move(336, y_offset + 48);
    tmp_label->setText(paths_txt[curr_lang()]);

    tmpFont.setBold(true);
    tmpFont.setItalic(true);
    tmpFont.setPixelSize(11);
    current_file_label = new QLabel; // имя текущего файла
    current_file_label->setFixedSize(464, 16);
    current_file_label->setStyleSheet("color: #d9b855");
    current_file_label->setFont(tmpFont);
    current_file_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    current_file_label->setParent(central_widget);
    current_file_label->move(400, y_offset);

    tmpFont.setPixelSize(13);
    paths_remaining_label = new QLabel; // количество оставшихся путей
    paths_remaining_label->setFixedSize(128, 16);
    paths_remaining_label->setStyleSheet("color: #d9b855");
    paths_remaining_label->setFont(tmpFont);
    paths_remaining_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    paths_remaining_label->setParent(central_widget);
    paths_remaining_label->move(400, y_offset + 48);
    //paths_remaining_label->setText("");

    connect(close_button, &OneStateButton::imReleased, this, &SessionWindow::close);
    connect(minimize_button, &OneStateButton::imReleased, this, &SessionWindow::showMinimized);

    ///////////////////////// здесь готовим поток walker'а : создаём его, соединяем сигналы/слоты, затем запускаем поток
    create_and_start_walker();
    /////////////////////////////////////////////////////////
}

void SessionWindow::create_and_start_walker()
{
    //task.delAllTaskPaths();
    //task.addTaskPath(TaskPath {R"(c:\Games\Remnant2\Remnant2\Content)", "*.*", true});
    //task.addTaskPath(TaskPath {R"(c:\Games\Borderlands 3 Directors Cut\OakGame\Content\Paks\pakchunk0-WindowsNoEditor.pak)", "", false});

    //task.addTaskPath(TaskPath {R"(c:\Downloads\rj_research\battlefield\title2.gif)", "", false});
    //task.addTaskPath(TaskPath {R"(c:\Downloads\rj_research\battlefield\1671625086303.jpg)", "", false});
    //task.addTaskPath(TaskPath {R"(c:\Downloads\rj_research\battlefield\CURE1.MOD)", "", false});
    //task.addTaskPath(TaskPath {R"(c:\Downloads\rj_research\battlefield\tiff\L1A.ZZZ.tif)", "", false});

    if ( !task.task_paths.empty() and !settings.selected_formats.empty() ) // запускаем только в случае наличия путей и хотя бы одного выбранного формата
    {
        is_walker_dead = false;
        walker_mutex = new QMutex;  // задача по освободжению указателя возложена на WalkerThread, т.к. SessionWindow может быть удалено раньше,
                                    // поэтому walker_mutex должен оставаться в памяти до окончания работы WalkerThread.

        walker = new WalkerThread(this, walker_mutex, task, settings.config, settings.selected_formats);

        connect(walker, &WalkerThread::finished, this, [this](){ // блокируем кнопки управления, когда walker отчитался, что закончил работу полностью
            qInfo() << " :::: finished() :::: signal received from WalkerThread and slot executed in thread id" << QThread::currentThreadId();
            stop_button->setDisabled(true);
            pause_resume_button->setDisabled(true);
            skip_button->setDisabled(true);
            is_walker_dead = true;
        });

        connect(walker, &WalkerThread::finished, walker, &QObject::deleteLater); // WalkerThread будет уничтожен в главном потоке после завершения работы метода .run()

        connect(walker, &WalkerThread::txImPaused, [=]() { // walker отчитывается сюда, что он встал на паузу
            stop_button->setEnabled(true);
            pause_resume_button->setEnabled(true);
            skip_button->setDisabled(true);
        });

        connect(walker, &WalkerThread::txImResumed, [=]() { // walker отчитывается сюда, что он снялся с паузы и продолжил работу
            stop_button->setEnabled(true);
            skip_button->setEnabled(true);
            pause_resume_button->setEnabled(true);
        });

        connect(stop_button, &QPushButton::clicked, [=]() {
            stop_button->setDisabled(true);
            pause_resume_button->setDisabled(true);
            skip_button->setDisabled(true);
            walker->command = WalkerCommand::Stop;
            walker_mutex->unlock();
        });

        connect(pause_resume_button, &QPushButton::clicked, [=](){
            stop_button->setDisabled(true);
            pause_resume_button->setDisabled(true);
            skip_button->setDisabled(true);
            // начальное значение is_walker_paused = false и выставляется при создании объекта SessionWindow
            if ( !(is_walker_paused) ) // нажали pause
            {
                walker_mutex->lock();
                walker->command = WalkerCommand::Pause;
                pause_resume_button->setText(resume_button_txt[curr_lang()]);
                is_walker_paused = true;
                // и дальше ждём, когда new_walker отчитается, что он встал на паузу (через его сигнал txImPaused);
                // только после этого можно активировать кнопки через setEnable(true) в слоте основного потока
            }
            else // нажали resume
            {
                walker->command = WalkerCommand::Run;
                walker_mutex->unlock();
                pause_resume_button->setText(pause_button_txt[curr_lang()]);
                is_walker_paused = false;
                // и дальше ждём, когда new_walker отчитается, что он снялся с паузы (через его сигнал txImResumed);
                // только после этого можно активировать кнопки через setEnable(true) в слоте основного потока
            }
        });

        connect(skip_button, &QPushButton::clicked, [=](){
            walker->command = WalkerCommand::Skip;
        });


        stop_button->setEnabled(true);
        pause_resume_button->setEnabled(true);
        skip_button->setEnabled(true);

        walker->start(QThread::InheritPriority);
    }
    else
    {
        qInfo() << " --- Nothing to do : no paths added or no formats selected";
    }
}

SessionWindow::~SessionWindow()
{
    sessions_pool.remove_session(this, my_session_id);
    qInfo() << "Session window destroyed";
}

void SessionWindow::rxGeneralProgress(QString remaining, u64i percentage_value)
{
    // qInfo() << "thread" << QThread::currentThreadId() << ": general progress is" << percentage_value << "%";
    general_progress_bar->setValue(percentage_value);
}

void SessionWindow::rxFileProgress(QString file_name, s64i percentage_value)
{
    //qInfo() << "thread" << QThread::currentThreadId() << ": file progress is" << percentage_value << "%";
    QString tmp_fn = reduce_file_path(QDir::toNativeSeparators(file_name), MAX_FILENAME_LEN);
    current_file_label->setText(reduce_file_path(QDir::toNativeSeparators(file_name), MAX_FILENAME_LEN));
    file_progress_bar->setValue(percentage_value);
}

void SessionWindow::rxResourceFound(const QString &format_name, const QString &file_name, s64i file_offset, u64i size, const QString &info)
{
    qInfo() << "---\n| Thread:" << QThread::currentThreadId();
    qInfo() << "|    resource" << format_name.toUpper() << "found at pos:" << file_offset << "; size:"<< size << "bytes";
    qInfo() << "|    in file:" << file_name;
    qInfo() << "|    additional info:" << info << "\n---";
}

void SessionWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void SessionWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        prev_cursor_pos = event->globalPosition();
    }
}

void SessionWindow::closeEvent(QCloseEvent *event)
{
    if ( !is_walker_dead )
    {
        walker->command = WalkerCommand::Stop;
    }
}

SessionsPool::SessionsPool(u32i size)
    : _size(size)
{
    pool = new SessionWindow*[_size];
    for (u32i idx = 0; idx < _size; ++idx)
    {
        pool[idx] = nullptr;
    }
}

SessionsPool::~SessionsPool()
{
    delete [] pool;
}

u32i SessionsPool::get_new_session_id()
{
    for (u32i id = 0; id < _size; ++id)
    {
        if ( pool[id] == nullptr )
        {
            ++id; // всегда возвращаем число от 1 до MAX_SESSIONS в случае успеха
            return id;
        }
    }
    return 0; // возвращаем 0 в случае неудачи
}

u32i SessionsPool::write_new_session(SessionWindow* session_window, u32i id)
{
    if ( id == 0 ) // проверка на дурака
    {
        return 0;
    }
    --id; // т.к. индекс массива начинается с 0
    if ( pool[id] == nullptr )
    {
        pool[id] = session_window;
        ++id; // т.к. нумераци сессий начинается с 1
        return id;
    }
    return 0;
}

u32i SessionsPool::remove_session(SessionWindow* session_window, u32i id)
{
    if ( id == 0 ) // проверка на дурака
    {
        return 0;
    }
    --id; // т.к. индекс массива начинается с 0
    if ( pool[id] == session_window )
    {
        pool[id] = nullptr;
        ++id; // т.к. нумераци сессий начинается с 1
        return id;
    }
    return 0;
}

u32i SessionsPool::get_active_count()
{
    u32i cnt {0};
    for (u32i id = 0; id < _size; ++id)
    {
        if ( pool[id] != nullptr )
        {
            ++cnt;
        }
    }
    return cnt;
}
