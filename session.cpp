#include "session.h"
#include "task.h"
#include <QMouseEvent>
#include <QEventLoop>
#include <QPushButton>
#include <QThread>
#include <QDir>
#include <QHeaderView>
#include <QScrollBar>
#include <QProcess>
#include <QApplication>
#include <QSharedMemory>
#include <QBuffer>
#include <QSystemSemaphore>

#define RESULTS_TABLE_WIDTH 785
#define RESULTS_TABLE_HEIGHT 450
#define RESULTS_TABLE_COLUMNS 7
#define RESULTS_TABLE_ROWS 6

extern QString reduce_file_path(const QString&, int);

extern SessionsPool      sessions_pool;
extern Settings          *settings;
extern Task              task;
extern QMap <QString, FileFormat> fformats;
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

const QString scan_in_progress_txt[int(Langs::MAX)]
{
    "SCANNING IN PROGRESS",
    "ИДЁТ СКАНИРОВАНИЕ"
};

const QString scan_is_done_txt[int(Langs::MAX)]
{
    "SCANNING COMPLETED!",
    "СКАНИРОВАНИЕ ЗАВЕРШЕНО!"
};

const QString scan_is_paused_txt[int(Langs::MAX)]
{
    "SCANNING PAUSED",
    "СКАНИРОВАНИЕ НА ПАУЗЕ"
};

const QString scan_stop_request_txt[int(Langs::MAX)]
{
    "STOP REQUEST SENT...",
    "ЗАПРОС ОСТАНОВА..."
};

const QString scan_pause_request_txt[int(Langs::MAX)]
{
    "PAUSE REQUEST SENT...",
    "ЗАПРОС ПАУЗЫ..."
};

const QString scan_skip_request_txt[int(Langs::MAX)]
{
    "SKIP REQUEST SENT...",
    "ЗАПРОС НА ПРОПУСК ФАЙЛА..."
};

const QString total_txt[int(Langs::MAX)]
{
    "Total:",
    "Всего:"
};

const QMap <u64i, QString> first_categ_resources
    {
        { CAT_IMAGE, ":/gui/session/cat_image_big.png" },
        { CAT_VIDEO, ":/gui/session/cat_video_big.png" },
        { CAT_AUDIO, ":/gui/session/cat_audio_big.png" },
        { CAT_MUSIC, ":/gui/session/cat_music_big.png" },
        { CAT_3D,    ":/gui/session/cat_3d_big.png"    }
    };

void FormatTile::update_counter(u64i value)
{
    counter->setText(QString::number(value));
}

FormatTile::FormatTile(const QString &format_name)
{
    this->setFixedSize(108, 149);
    this->setAutoFillBackground(true);
    this->setStyleSheet("background-color: #794642; border-width: 2px; border-style: solid; margin: 10px; border-radius: 14px; border-color: #b6c7c7;");
    auto big_icon = new QLabel(this);
    QPixmap big_icon_pixmap {first_categ_resources[fformats[format_name].base_categories[0]]};
    big_icon->setFixedSize(big_icon_pixmap.width(), big_icon_pixmap.height());
    big_icon->setPixmap(big_icon_pixmap);
    big_icon->setStyleSheet("border-width: 0px; margin: 0px");
    big_icon->move(34, 24);
    auto fmt_text = new QLabel(this);
    fmt_text->setFixedSize(80, 48);
    fmt_text->setStyleSheet("color: #e3b672; background-color: #794642; border-width: 2px; border-style: solid; border-radius: 8px; border-color: #b6c7c7;");
    fmt_text->move(14, 60);
    fmt_text->setAlignment(Qt::AlignCenter);
    QFont tmpFont {*skin_font()};
    tmpFont.setPixelSize(15);
    tmpFont.setBold(true);
    fmt_text->setFont(tmpFont);
    fmt_text->setText(fformats[format_name].extension);
    counter = new QLabel(this);
    counter->setFixedSize(80, 24);
    counter->setStyleSheet("color: #c6d7d7; background-color: #794642; border-width: 0px; margin: 0px;");
    counter->move(14, 104);
    counter->setAlignment(Qt::AlignCenter);
    tmpFont.setPixelSize(14);
    tmpFont.setBold(false);
    counter->setFont(tmpFont);
}

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
                                        yes_no_txt[curr_lang()][int(settings->config.scrupulous)],
                                        settings->config.file_mask,
                                        QString::number(permitted_buffers[settings->config.bfr_size_idx]) + mebibytes_txt[curr_lang()]
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
    paths_remaining = new QLabel; // количество оставшихся путей
    paths_remaining->setFixedSize(128, 16);
    paths_remaining->setStyleSheet("color: #d9b855");
    paths_remaining->setFont(tmpFont);
    paths_remaining->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    paths_remaining->setParent(central_widget);
    paths_remaining->move(400, y_offset + 48);

    connect(close_button, &OneStateButton::imReleased, this, &SessionWindow::close);
    connect(minimize_button, &OneStateButton::imReleased, this, &SessionWindow::showMinimized);

    tmpFont.setPixelSize(15);
    tmpFont.setItalic(false);
    current_status = new QLabel; // текущий статус
    current_status->setFixedSize(196, 16);
    current_status->setStyleSheet("color: #f9d875");
    current_status->setFont(tmpFont);
    current_status->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    current_status->setText(scan_in_progress_txt[curr_lang()]);
    current_status->setParent(central_widget);
    current_status->move(364, y_offset + 125);

    tmpFont.setPixelSize(14);
    tmpFont.setBold(false);
    tmp_label = new QLabel; // надпись "Total:"
    tmp_label->setFixedSize(64, 16);
    tmp_label->setStyleSheet("color: #d4e9e9");
    tmp_label->setFont(tmpFont);
    tmp_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tmp_label->setParent(central_widget);
    tmp_label->move(92, central_widget->height() - 36);
    tmp_label->setText(total_txt[curr_lang()]);

    tmpFont.setPixelSize(15);
    tmpFont.setBold(true);
    total_resources = new QLabel; // счётчик ресурсов
    total_resources->setFixedSize(196, 16);
    total_resources->setStyleSheet("color: #d4e9e9");
    total_resources->setFont(tmpFont);
    total_resources->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    total_resources->setText("0");
    total_resources->setParent(central_widget);
    total_resources->move(148, central_widget->height() - 36);

    movie_zone = new QLabel; // регион для отображения анимационных gif'ы в зависимости от статуса
    movie_zone->setFixedSize(32, 32);
    movie_zone->setParent(central_widget);
    movie_zone->move(316, 180);

    scan_movie = new QMovie(":/gui/session/scanning.gif");
    movie_zone->setMovie(scan_movie);
    scan_movie->start();

    pages = new QStackedWidget;
    pages->setParent(this);
    pages->move(72, 244);
    pages->setFixedSize(RESULTS_TABLE_WIDTH, RESULTS_TABLE_HEIGHT);

    results_table = new QTableWidget(RESULTS_TABLE_ROWS, RESULTS_TABLE_COLUMNS);
    results_table->setFixedSize(RESULTS_TABLE_WIDTH, RESULTS_TABLE_HEIGHT);
    pages->addWidget(results_table); // page 0
    results_table->setStyleSheet("gridline-color: #8d6858; background-color: #8d6858");
    results_table->verticalScrollBar()->setStyle(new QCommonStyle);
    results_table->verticalScrollBar()->setStyleSheet(  ":vertical {background-color: #8d6858; width: 10px}"
                                                        "::handle:vertical {background-color: #895652; border-radius: 5px;}"
                                                        "::sub-line:vertical {height: 0px}"
                                                        "::add-line:vertical {height: 0px}");
    results_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    results_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    results_table->setSelectionMode(QAbstractItemView::NoSelection);
    results_table->verticalHeader()->hide();
    results_table->horizontalHeader()->hide();
    results_table->setFrameShape(QFrame::NoFrame);
    results_table->setSortingEnabled(false);
    results_table->horizontalHeader()->setHighlightSections(false);

    for (int column = 0; column < RESULTS_TABLE_COLUMNS; ++column)
    {
        results_table->setColumnWidth(column, 770 / RESULTS_TABLE_COLUMNS);
    }
    for (int row = 0; row < RESULTS_TABLE_ROWS; ++row)
    {
        results_table->setRowHeight(row, 450 / 3);
    }

    // каждому формату заранее создаём свою страницу (начиная со страницы 1)
    for (auto it = fformats.cbegin(); it != fformats.cend(); ++it)
    {
        auto fformat_widget = new QWidget;
        fformat_widget->setFixedSize(RESULTS_TABLE_WIDTH, RESULTS_TABLE_HEIGHT);
        fformat_widget->setAutoFillBackground(true);
        pages->addWidget(fformat_widget);
    };
    //qInfo() << pages->count();

    // завершающей страницей будет информационный виджет (с анимацией и текстом)
    auto info_widget = new QWidget;
    info_widget->setFixedSize(RESULTS_TABLE_WIDTH, RESULTS_TABLE_HEIGHT);
    info_widget->setAutoFillBackground(true);
    pages->addWidget(info_widget);

    pages->setCurrentIndex(0);

    ///////////////////////// здесь готовим поток walker'а : создаём его, соединяем сигналы/слоты, затем запускаем поток
    create_and_start_walker();
    /////////////////////////////////////////////////////////
}

void SessionWindow::create_and_start_walker()
{
    task.delAllTaskPaths();
    //task.addTaskPath(TaskPath {R"(c:\Games\Borderlands 3 Directors Cut\OakGame\Content\Paks\pakchunk0-WindowsNoEditor.pak)", "", false});

    //task.addTaskPath(TaskPath {R"(c:\Games\Remnant2\Remnant2\Content)", "*.*", true});
    task.addTaskPath(TaskPath {R"(c:\Downloads\rj_research\battlefield\result_png_it.dat)", "", false});
    //task.addTaskPath(TaskPath {R"(c:\Downloads\rj_research\battlefield\mp3\00001.mp3)", "", false});

    if ( !task.task_paths.empty() and !settings->selected_formats.empty() ) // запускаем только в случае наличия путей и хотя бы одного выбранного формата
    {
        is_walker_dead = false;
        walker_mutex = new QMutex;  // задача по освободжению указателя возложена на WalkerThread, т.к. SessionWindow может быть удалено раньше,
                                    // поэтому walker_mutex должен оставаться в памяти до окончания работы WalkerThread.

        walker = new WalkerThread(this, walker_mutex, task, settings->config, settings->selected_formats);

        connect(walker, &WalkerThread::finished, this, [this](){ // блокируем кнопки управления, когда walker отчитался, что закончил работу полностью
            //qInfo() << " :::: finished() :::: signal received from WalkerThread and slot executed in thread id" << QThread::currentThreadId();
            stop_button->setDisabled(true);
            pause_resume_button->setDisabled(true);
            skip_button->setDisabled(true);
            current_status->setText(scan_is_done_txt[curr_lang()]);
            is_walker_dead = true;
            // разблокируем кнопки "Save All" и "Report"
            save_all_button->setEnabled(true);
            report_button->setEnabled(true);
            scan_movie->stop();
            movie_zone->setPixmap(QPixmap(":/gui/session/done.png"));
            }, Qt::QueuedConnection);

        connect(walker, &WalkerThread::finished, walker, &QObject::deleteLater); // WalkerThread будет уничтожен в главном потоке после завершения работы метода .run()

        connect(walker, &WalkerThread::txFileWasSkipped, this, [this](){ // Engine завершил работы по причине получения сигнала Skip -> нам надо обновить статус
            //qInfo() << " :::: txFileWasSkipped() :::: signal received from WalkerThread and slot executed in thread id" << QThread::currentThreadId();
            current_status->setText(scan_in_progress_txt[curr_lang()]);
        });

        connect(walker, &WalkerThread::txImPaused, this, [this]() { // walker отчитывается сюда, что он встал на паузу
            stop_button->setEnabled(true);
            pause_resume_button->setEnabled(true);
            skip_button->setDisabled(true);
            current_status->setText(scan_is_paused_txt[curr_lang()]);
            scan_movie->setPaused(true);
        }, Qt::QueuedConnection);

        connect(walker, &WalkerThread::txImResumed, this, [this]() { // walker отчитывается сюда, что он снялся с паузы и продолжил работу
            stop_button->setEnabled(true);
            skip_button->setEnabled(true);
            pause_resume_button->setEnabled(true);
            current_status->setText(scan_in_progress_txt[curr_lang()]);
            scan_movie->setPaused(false);
        }, Qt::QueuedConnection);

        connect(stop_button, &QPushButton::clicked, this, [this]() {
            stop_button->setDisabled(true);
            pause_resume_button->setDisabled(true);
            skip_button->setDisabled(true);
            walker->command = WalkerCommand::Stop;
            walker_mutex->unlock();
            current_status->setText(scan_stop_request_txt[curr_lang()]);
        }, Qt::QueuedConnection);

        connect(pause_resume_button, &QPushButton::clicked, [=](){
            stop_button->setDisabled(true);
            pause_resume_button->setDisabled(true);
            skip_button->setDisabled(true);
            // начальное значение is_walker_paused = false и выставляется при создании объекта SessionWindow
            if ( !is_walker_paused ) // нажали pause
            {
                current_status->setText(scan_pause_request_txt[curr_lang()]);
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
            current_status->setText(scan_skip_request_txt[curr_lang()]);
        });

        stop_button->setEnabled(true);
        pause_resume_button->setEnabled(true);
        skip_button->setEnabled(true);

        connect(save_all_button, &QPushButton::clicked, this, &SessionWindow::rxStartSaveAllProcess);

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
    //qInfo() << "Session window destroyed";
}

void SessionWindow::rxGeneralProgress(QString remaining, u64i percentage_value)
{
    // qInfo() << "thread" << QThread::currentThreadId() << ": general progress is" << percentage_value << "%";
    general_progress_bar->setValue(percentage_value);
}

void SessionWindow::rxFileChange(QString file_name)
{
    QString tmp_fn = reduce_file_path(QDir::toNativeSeparators(file_name), MAX_FILENAME_LEN);
    current_file_label->setText(reduce_file_path(QDir::toNativeSeparators(file_name), MAX_FILENAME_LEN));
}

void SessionWindow::rxFileProgress(s64i percentage_value)
{
    file_progress_bar->setValue(percentage_value);
}

void SessionWindow::rxResourceFound(const QString &format_name, const QString &file_name, s64i file_offset, u64i size, const QString &info)
{
    ++total_resources_found;
    if ( !resources_db.contains(format_name) ) // формат встретился первый раз?
    {
        ++unique_formats_found;
        int row = (unique_formats_found - 1) / 7;
        int column = (unique_formats_found - 1) % 7;
        auto tile = new FormatTile(format_name);
        results_table->setCellWidget(row, column, tile);
        tiles_db[format_name] = tile;
        formats_counters[format_name] = 0;
    }
    ++formats_counters[format_name];
    // занесение в БД
    resources_db[format_name][file_name].append( { total_resources_found, file_offset, size, info, fformats[format_name].extension } );
    tiles_db[format_name]->update_counter(formats_counters[format_name]);
    total_resources->setText(QString::number(total_resources_found));
}

void SessionWindow::rxStartSaveAllProcess()
{
    save_all_button->setDisabled(true);
    QByteArray data_buffer;
    char str_end = 0;
    // временные переменные, используемые ниже при обходе бд
    TLV_Header tlv_header;
    QByteArray qba_format_name;
    QByteArray qba_file_name;
    QByteArray qba_dst_extension;
    QByteArray qba_info;
    QString format_name_key;
    QString file_name_key;
    POD_ResourceRecord pod_rr;
    //
    // перенос данных из resources_db в QByteArray (с одновременным обнулением resources_db)
    while(!resources_db.isEmpty()) // обход ключей форматов
    {
        /// запись TLV "FmtChange" 'смена формата':
        format_name_key = resources_db.firstKey();
        tlv_header.type = TLV_Type::FmtChange;
        qba_format_name = format_name_key.toLatin1();
        tlv_header.length = qba_format_name.length();// + 1; // +1 на нулевой символ в конце
        data_buffer.append(QByteArray((char*)&tlv_header, sizeof(tlv_header)));
        //data_buffer.append(QByteArray(&str_end), 1);
        data_buffer.append(qba_format_name);
        qInfo() << "written 'FmtChange' for '" << format_name_key << "' current size of buffer:" << data_buffer.size();
        ///
        QMap<QString, QList<ResourceRecord>> &source_files = resources_db[format_name_key];
        while(!source_files.isEmpty()) // обход ключей имён файлов
        {
            /// запись TLV "SrcChange" 'смена файла':
            file_name_key = source_files.firstKey();
            tlv_header.type = TLV_Type::SrcChange;
            qba_file_name = file_name_key.toUtf8();
            tlv_header.length = qba_file_name.length();
            data_buffer.append(QByteArray((char*)&tlv_header, sizeof(tlv_header)));
            data_buffer.append(qba_file_name);
            qInfo() << "written 'SrcChange' for file:" << source_files.firstKey() << "' current size of buffer:" << data_buffer.size();;
            ///
            QList <ResourceRecord> &resource_records = resources_db[format_name_key][file_name_key];
            while(!resource_records.isEmpty())
            {
                ResourceRecord &one_resource = resource_records.first();
                /// запись TLV "POD":
                tlv_header.type = TLV_Type::POD;
                tlv_header.length = sizeof(POD_ResourceRecord);
                pod_rr.order_number = one_resource.order_number;
                pod_rr.offset = one_resource.offset;
                pod_rr.size = one_resource.size;
                data_buffer.append(QByteArray((char*)&tlv_header, sizeof(tlv_header)));
                data_buffer.append(QByteArray((char*)&pod_rr, sizeof(pod_rr)));
                qInfo() << "written 'POD': pod_rr.order_num:" << pod_rr.order_number << " pod_rr.offset:" << pod_rr.offset << "pod_rr.size:" << pod_rr.size << " current size of buffer:" << data_buffer.size();;
                ///
                /// запись TLV "DstExtension":
                tlv_header.type = TLV_Type::DstExtension;
                qba_dst_extension = one_resource.dest_extension.toLatin1();
                tlv_header.length = qba_dst_extension.length();
                data_buffer.append(QByteArray((char*)&tlv_header, sizeof(tlv_header)));
                data_buffer.append(qba_dst_extension);
                qInfo() << "written 'DstExtension':" << one_resource.dest_extension << " current size of buffer:" << data_buffer.size();
                ///
                /// запись TLV "Info":
                tlv_header.type = TLV_Type::Info;
                qba_info = one_resource.info.toUtf8();
                tlv_header.length = qba_info.length();
                data_buffer.append(QByteArray((char*)&tlv_header, sizeof(tlv_header)));
                data_buffer.append(qba_info);
                qInfo() << "written 'Info':" << one_resource.info << " current size of buffer:" << data_buffer.size();
                ///
                resource_records.removeFirst();
                resource_records.squeeze();
            }
            source_files.remove(source_files.firstKey()); // удаление ключа имени файла
        }
        resources_db.remove(resources_db.firstKey()); // удаление ключа формата
    }
    /// запись TLV "Terminator" 'Завершитель'
    tlv_header.type = TLV_Type::Terminator;
    tlv_header.length = 0;
    data_buffer.append(QByteArray((char*)&tlv_header, sizeof(tlv_header)));
    qInfo() << "written 'Terminator' : current size of buffer:" << data_buffer.size();
    ///
    resources_db.clear();  // на всякий случай ещё раз обнуляем (хотя после обхода, бд уже должна быть пустой).

    QSharedMemory shared_memory {QString("/shm/rj/%1/%2/%3").arg(  //создаём максимально уникальный key для shared memory
                                                                    QString::number(QApplication::applicationPid()),
                                                                    QString::number(u64i(QThread::currentThreadId())),
                                                                    QString::number(QDateTime::currentMSecsSinceEpoch())
                                                                ) };
    if ( !shared_memory.create(data_buffer.size(), QSharedMemory::ReadWrite) )
    {
        qInfo() << "shm error:" << shared_memory.error();
        return;
    }

    QSystemSemaphore sys_semaphore {QString("/ssem/rj/%1/%2/%3").arg(  //создаём максимально уникальный key для system semaphore
                                                                        QString::number(QApplication::applicationPid()),
                                                                        QString::number(u64i(QThread::currentThreadId())),
                                                                        QString::number(QDateTime::currentMSecsSinceEpoch())),
                                    1,
                                    QSystemSemaphore::Create
                                    };
    if ( sys_semaphore.error() != QSystemSemaphore::NoError )
    {
        qInfo() << "ssem error:" << sys_semaphore.error();
        shared_memory.detach();
        return;
    }

    // копирование из буфера в shared_memory
    char *from_buffer = (char*)data_buffer.data();
    char *to_shm = (char*)shared_memory.data();
    memcpy(to_shm, from_buffer, data_buffer.size());
    qInfo() << "||| data_buffer.size:" << data_buffer.size() << " shared_memory.size:" << shared_memory.size();

    QProcess saving_process;
    saving_process.startDetached(QCoreApplication::arguments()[0], QStringList{ "-save", shared_memory.key(), QString::number(data_buffer.size()), sys_semaphore.key() }, "");

    data_buffer.clear();   // обнуляем и буфер, т.к. теперь все данные в shared_memory
    data_buffer.squeeze(); //

    sys_semaphore.acquire();
    sys_semaphore.acquire(); // <- здесь ждём, когда saving_process отдаст семафор
    sys_semaphore.release(); // и тогда уже можно будет оторваться (detach) от shared memory

    shared_memory.detach();
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
