#include "saving.h"
#include "formats.h"
#include "session.h"
#include <QApplication>
#include <QFileDialog>
#include <QMouseEvent>
#include <QScrollBar>
#include "settings.h"
#include <QFont>
#include <QDesktopServices>
#include <QScreen>
#include <QWindow>

extern Settings *settings;
extern QMap <QString, FileFormat> fformats;

extern QString reduce_file_path(const QString&, int);

const QString saving_to_txt[int(Langs::MAX)]
{
    "Saving to directory",
    "Сохранение в каталог"
};

const QString saving_report_to_txt[int(Langs::MAX)]
{
    "Saving report to",
    "Сохранение отчёта в"
};

const QString saved_txt[int(Langs::MAX)]
{
    "resources saved    :",
    "сохранено ресурсов :"
};

const QString remains_txt[int(Langs::MAX)]
{
    "left to  :",
    "осталось :"
};

const QString minimize_button_txt[int(Langs::MAX)]
{
    "minimize",
    "свернуть"
};

const QString abort_button_txt[int(Langs::MAX)]
{
    "abort",
    "отмена"
};

const QString html_report_txt[int(Langs::MAX)]
{
    "Report",
    "Отчёт",
};

SaveDBGWindow::SaveDBGWindow()
    : QWidget(nullptr)
{
    this->setFixedSize(800, 600);

    info_text = new QTextEdit(this);
    info_text->move(8, 8);
    info_text->setFixedSize(this->width() - 16, this->height() - 16);
}

SavingWindow::SavingWindow(const QString &shm_key, const QString &shm_size, const QString &ssem_key, bool is_debug, const QString &language_id, const QString &screen, bool is_report)
    : QWidget(nullptr, Qt::FramelessWindowHint)
    , debug_mode(is_debug)
    , report_mode(is_report)
    , lang_id(language_id.toInt())
    , screen_name(screen)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setAttribute(Qt::WA_DeleteOnClose); // само удалится при закрытии
    this->setFixedSize(372, 164);

    auto central_widget = new QLabel(this);
    central_widget->move(0, 0);
    central_widget->setFixedSize(this->width(), this->height());
    central_widget->setObjectName("swcw");
    central_widget->setStyleSheet("QLabel#swcw {background-color: #929167; border-width: 4px; border-style: solid; border-radius: 36px; border-color: #665f75;}");

    auto background = new QLabel(central_widget);
    background->move(8, 8);
    background->setFixedSize(central_widget->width() - 16, central_widget->height() - 16);
    background->setObjectName("swbg");
    background->setStyleSheet("QLabel#swbg {background-color: #555763; border-width: 0px; border-radius: 30px;}");

    QFont common_font {*skin_font()};
    common_font.setItalic(false);
    common_font.setBold(true);
    common_font.setPixelSize(13);

    auto saving_to_label = new QLabel(background); // надпись "Сохранение в"
    saving_to_label->setFixedSize(164, 24);
    saving_to_label->move(background->width() / 2 - saving_to_label->width() / 2, 8);
    saving_to_label->setStyleSheet("color: #f1c596");
    saving_to_label->setFont(common_font);
    saving_to_label->setAlignment(Qt::AlignCenter);
    saving_to_label->setText(report_mode ? saving_report_to_txt[lang_id] : saving_to_txt[lang_id]);

    common_font.setItalic(true);
    common_font.setPixelSize(11);

    path_label = new QTextEdit(background); // путь для сохранения
    path_label->setFixedSize(340, 40);
    path_label->move(background->width() / 2 - path_label->width() / 2, 32);
    path_label->setReadOnly(true);
    path_label->setStyleSheet("background-color: #555763; color: #f9e885;");
    path_label->setFont(common_font);
    path_label->setFrameShape(QFrame::NoFrame); // background-color сработал только после отключения рамки
    path_label->verticalScrollBar()->hide();
    path_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    path_label->setLineWrapMode(QTextEdit::WidgetWidth);

    common_font.setItalic(false);
    common_font.setPixelSize(12);

    progress_bar = new QProgressBar(background);
    progress_bar->setFixedSize(332, 16);
    progress_bar->move(background->width() / 2 - progress_bar->width() / 2, 64);
    progress_bar->setFont(common_font);
    progress_bar->setMinimum(0);
    progress_bar->setMaximum(100);
    progress_bar->setAlignment(Qt::AlignCenter);
    progress_bar->setStyleSheet("QProgressBar {color: #2f2e29; background-color: #b6c7c7; border-width: 2px; border-style: solid; border-radius: 6px; border-color: #b6c7c7;}"
                                "QProgressBar:chunk {background-color: #42982f; border-width: 0px; border-style: solid; border-radius: 6px;}");
    progress_bar->setFormat("%v%");
    progress_bar->setValue(0);

    common_font.setBold(false);
    common_font.setPixelSize(12);

    auto saved_text_label = new QLabel(background); // надпись "сохранено ресурсов :"
    saved_text_label->setFixedSize(140, 16);
    saved_text_label->move(16, 88);
    saved_text_label->setStyleSheet("color: #d4e9e9");
    saved_text_label->setFont(common_font);
    saved_text_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    saved_text_label->setText(saved_txt[lang_id]);

    auto remains_text_label = new QLabel(background); // надпись "осталось :"
    remains_text_label->setFixedSize(72, 16);
    remains_text_label->move(16, 108);
    remains_text_label->setStyleSheet("color: #d4e9e9");
    remains_text_label->setFont(common_font);
    remains_text_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    remains_text_label->setText(remains_txt[lang_id]);

    common_font.setPixelSize(13);

    saved_label = new QLabel(background); // количество сохранённых ресурсов
    saved_label->move(164, 88);
    saved_label->setFixedSize(196, 16);
    saved_label->setStyleSheet("color: #d9b855");
    saved_label->setFont(common_font);
    saved_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    remains_label = new QLabel(background); // количество оставшихся ресурсов
    remains_label->move(96, 108);
    remains_label->setFixedSize(196, 16);
    remains_label->setStyleSheet("color: #d9b855");
    remains_label->setFont(common_font);
    remains_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    common_font.setPixelSize(12);

    auto minimize_button = new QPushButton(background);
    minimize_button->move(196, 116);
    minimize_button->setFixedSize(72, 24);
    minimize_button->setAttribute(Qt::WA_NoMousePropagation);
    minimize_button->setStyleSheet( "QPushButton:enabled  {color: #fffef9; background-color: #9ca14e; border-width: 2px; border-style: solid; border-radius: 12px; border-color: #b6c7c7;}"
                                    "QPushButton:hover    {color: #fffef9; background-color: #7c812e; border-width: 2px; border-style: solid; border-radius: 12px; border-color: #b6c7c7;}"
                                    "QPushButton:pressed  {color: #fffef9; background-color: #7c812e; border-width: 0px;}"
                                    "QPushButton:disabled {color: #a5a5a5; background-color: #828282; border-width: 0px; border-style: solid; border-radius: 12px;}");

    minimize_button->setFont(common_font);
    minimize_button->setText(minimize_button_txt[lang_id]);

    abort_button = new QPushButton(background);
    abort_button->move(276, 116);
    abort_button->setFixedSize(64, 24);
    abort_button->setAttribute(Qt::WA_NoMousePropagation);
    abort_button->setStyleSheet("QPushButton:enabled  {color: #fffef9; background-color: #ab6152; border-width: 2px; border-style: solid; border-radius: 12px; border-color: #b6c7c7;}"
                                "QPushButton:hover    {color: #fffef9; background-color: #8b4132; border-width: 2px; border-style: solid; border-radius: 12px; border-color: #b6c7c7;}"
                                "QPushButton:pressed  {color: #fffef9; background-color: #8b4132; border-width: 0px;}"
                                "QPushButton:disabled {color: #a5a5a5; background-color: #828282; border-width: 0px; border-style: solid; border-radius: 12px;}");

    abort_button->setFont(common_font);
    abort_button->setText(abort_button_txt[lang_id]);

    connect(minimize_button, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(abort_button, &QPushButton::clicked, this, &QWidget::close);

    if ( debug_mode) debug_window = new SaveDBGWindow;

    /// поиск экрана по переданному имени
    for(auto screen_ptr: QApplication::screens())
    {
        if ( screen_ptr->name() == screen_name )
        {
            /// перемещение относительно TopLeft-точки текущего экрана
            this->move(screen_ptr->availableGeometry().topLeft().x() + screen_ptr->availableSize().width() - this->width() - 16, screen_ptr->availableGeometry().topLeft().y() + screen_ptr->availableSize().height() - this->height() - 16);
            if ( debug_mode)
            {
                debug_window->setScreen(screen_ptr);
                debug_window->move(screen_ptr->availableGeometry().topLeft().x(), screen_ptr->availableGeometry().topLeft().y());
            }
            ///
            break;
        }
    }
    ///

    //load_data_from_file(); // для отладки, загрузка дампа shm из файла
    load_data_from_shm(shm_key, shm_size, ssem_key);
}

SavingWindow::~SavingWindow()
{
    if ( debug_window != nullptr ) debug_window->close();
    QDesktopServices::openUrl(QUrl("file:///" + saving_path, QUrl::TolerantMode)); // открываем в проводнике каталог с ресурсами
}

void SavingWindow::start_saver(const QString &path)
{
    saving_path = path;
    if ( debug_mode) debug_window->info_text->append("Save to: " + path);
    path_label->setPlainText(reduce_file_path(QDir::toNativeSeparators(path + "a.a"), 86).chopped(3)); // временно прибавляем фиктивное имя файла 'a.a', чтобы reduce_file_path правильно отработала
    auto saver = new SaverThread(this, &resources_db, &src_files, debug_mode, path);
    connect(saver, &SaverThread::txDebugInfo, this, &SavingWindow::rxDebugInfo, Qt::QueuedConnection);
    connect(saver, &SaverThread::txNextWasSaved, this, &SavingWindow::rxNextWasSaved, Qt::QueuedConnection);
    connect(saver, &SaverThread::finished, this, &SavingWindow::rxSaverFinished, Qt::QueuedConnection);
    saver->start();
}

void SavingWindow::start_reporter(const QString &path)
{
    saving_path = path;
    QString report_file_path = saving_path + "report.html";
    path_label->setPlainText(reduce_file_path(QDir::toNativeSeparators(report_file_path), 86));
    //qInfo() << "report_file_path:" << report_file_path;
    auto reporter = new ReporterThread(this, &resources_db, &src_files, debug_mode, report_file_path, lang_id, &session_settings, &formats_counters);
    connect(reporter, &ReporterThread::txNextWasReported, this, &SavingWindow::rxNextWasReported, Qt::QueuedConnection);
    connect(reporter, &ReporterThread::finished, this, &SavingWindow::rxReporterFinished, Qt::QueuedConnection);
    reporter->start();
}

void SavingWindow::rxNextWasSaved()
{
    ++resources_saved;
    saved_label->setText(QString::number(resources_saved));
    remains_label->setText(QString::number(total_resources_found - resources_saved));
    progress_bar->setValue(resources_saved * 100 / total_resources_found);
}

void SavingWindow::rxNextWasReported()
{
    ++resources_reported;
    saved_label->setText(QString::number(resources_reported));
    remains_label->setText(QString::number(total_resources_found - resources_reported));
    progress_bar->setValue(resources_reported * 100 / total_resources_found);
}


void SavingWindow::rxDebugInfo(QString info)
{
    debug_window->info_text->append(info);
}

void SavingWindow::rxSaverFinished()
{
    //this->close();
}

void SavingWindow::rxReporterFinished()
{
    //qInfo() << "reporter finished";
    //this->close();
}

void SavingWindow::decode_data(u8i *buffer)
{
    TLV_Header *tlv_header;
    QString current_format;
    QString src_file;
    POD_ResourceRecord *current_pod_ptr;
    POD_SessionSettings *ss_pod_ptr;
    ResourceRecord current_rr;
    QString current_dest_ext;
    QString current_info;
    u64i next_header_offset = 0; // =0 - важно!
    do
    {
        tlv_header = (TLV_Header*)&buffer[next_header_offset];
        if ( debug_mode) debug_window->info_text->append("\ntype:" + QString::number(int(tlv_header->type)) + " length:" + QString::number(int(tlv_header->length)));
        switch(tlv_header->type)
        {
        case TLV_Type::SessionSettings: // различные свойства сессии
        {
            ss_pod_ptr = (POD_SessionSettings*)((char*)tlv_header + sizeof(TLV_Header));
            session_settings = *ss_pod_ptr;
            // session_settings.start_msecs = ss_pod_ptr->start_msecs;
            // session_settings.end_msecs = ss_pod_ptr->end_msecs;
            if ( debug_mode) debug_window->info_text->append("-> ss_start_msecs:" + QString::number(session_settings.start_msecs) + "; ss_end_msecs:" + QString::number(session_settings.end_msecs));
            break;
        }

        case TLV_Type::SrcFile: // эти TLV всегда идут перед POD_ResourceRecord
        {
            src_file = QString::fromUtf8((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            src_files.append(src_file);
            if ( debug_mode) debug_window->info_text->append("-> SrcFile: '" + src_file + "'");
            break;
        }
        case TLV_Type::FmtChange:
        {
            current_format = QString::fromLatin1((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            if ( debug_mode) debug_window->info_text->append("-> FmtChange: '" + current_format + "'");
            if ( !resources_db.contains(current_format) ) // формат встретился первый раз?
            {
                formats_counters[current_format].total_number_of = 0;
                formats_counters[current_format].total_size_of = 0;
            }
            break;
        }
        case TLV_Type::POD: // POD_ResourceRecord
        {
            current_pod_ptr = (POD_ResourceRecord*)((char*)tlv_header + sizeof(TLV_Header));
            current_rr.order_number = current_pod_ptr->order_number;
            current_rr.src_fname_idx = current_pod_ptr->src_fname_idx;
            current_rr.offset = current_pod_ptr->offset;
            current_rr.size = current_pod_ptr->size;
            if ( debug_mode) debug_window->info_text->append("-> POD : order_number: " + QString::number(current_rr.order_number) + "; src_fname_idx: " + QString::number(current_rr.src_fname_idx) + "; offset: " + QString::number(current_rr.offset) + "; size: " + QString::number(current_rr.size));
            if ( debug_mode) debug_window->info_text->append("-> POD : src_file by idx: " + src_files[current_rr.src_fname_idx]);
            break;
        }
        case TLV_Type::DstExtension:
        {
            current_dest_ext = QString::fromLatin1((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            if ( debug_mode) debug_window->info_text->append("-> DstExtension: '" + current_dest_ext + "'");
            current_rr.dest_extension = current_dest_ext;
            break;
        }
        case TLV_Type::Info:
        {
            current_info = QString::fromUtf8((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            if ( debug_mode) debug_window->info_text->append("-> Info: '" + current_info + "'");
            current_rr.info = current_info;
            /// TLV "Info" всегда идёт завершающей, значит можно сделать запись в БД
            resources_db[current_format].append(current_rr);
            ++total_resources_found;
            ++formats_counters[current_format].total_number_of;
            formats_counters[current_format].total_size_of += current_rr.size;
            ///
            break;
        }
        case TLV_Type::Terminator:
        {
            if ( debug_mode) debug_window->info_text->append("-> Terminator.");
            break;
        }
        }
        next_header_offset += (sizeof(TLV_Header) + tlv_header->length);
    } while(tlv_header->type != TLV_Type::Terminator);

    if ( debug_mode) debug_window->info_text->append("\nTOTAL RESOURCES FOUND: " + QString::number(total_resources_found));

}

void SavingWindow::load_data_from_shm(const QString &shm_key, const QString &shm_size, const QString &ssem_key)
{
    if ( debug_mode )
    {
        // debug_window = new SaveDBGWindow;
        debug_window->setWindowTitle("debug");
        debug_window->show();

        debug_window->info_text->append("Screen name (from args): " + screen_name);
        debug_window->info_text->append("Current screen name: " + this->screen()->name());
        debug_window->info_text->append("My process Id: " + QString::number(QApplication::applicationPid()));
        debug_window->info_text->append("Shared memory key: " + shm_key);
        debug_window->info_text->append("Shared memory size: " + shm_size);
        debug_window->info_text->append("System semaphore key: " + ssem_key);
    }

    sys_semaphore = new QSystemSemaphore(ssem_key, 1, QSystemSemaphore::Open);
    if ( sys_semaphore->error() != QSystemSemaphore::NoError )
    {
        if ( debug_mode) debug_window->info_text->append("System semaphore opening error? -> : " + sys_semaphore->errorString());
        delete sys_semaphore;
        return;
    }

    shared_memory.setKey(shm_key);
    shared_memory.attach(QSharedMemory::ReadOnly);
    sys_semaphore->release(); // говорим родительскому процессу, что он может отключиться от shm
    if ( shared_memory.error() != QSharedMemory::NoError )
    {
        if ( debug_mode) debug_window->info_text->append("Shared memory attach error? -> : " + shared_memory.errorString());
        delete sys_semaphore;
        return;
    }
    if ( debug_mode) debug_window->info_text->append("Shared memory real size: " + QString::number(shared_memory.size()));

    /// запись дампа shm на диск
    // QFile shm_dump_file("./" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".dmp"); // в каталог запуска
    // shm_dump_file.open(QIODeviceBase::ReadWrite);
    // int dump_size = shm_size.toUInt();
    // shm_dump_file.resize(dump_size);
    // auto mmf_shm_dump = shm_dump_file.map(0, dump_size);
    // memcpy(mmf_shm_dump, shared_memory.data(), dump_size);
    // shm_dump_file.flush();
    // shm_dump_file.unmap(mmf_shm_dump);
    // shm_dump_file.close();
    ///

    decode_data((u8i*)shared_memory.data());

    shared_memory.detach(); // отсоединяемся от shm, чтобы ОС могла освободить занимаемую память
    delete sys_semaphore;
}

void SavingWindow::load_data_from_file()
{
    QFile dump_file("c:/Downloads/2025-02-12-17-46-54.dmp");
    if ( !dump_file.open(QIODeviceBase::ReadWrite) )
    {
        qInfo() << "error opening dump file";
        return;
    }
    auto mmf = dump_file.map(0, dump_file.size());
    if ( mmf == nullptr )
    {
        qInfo() << "error memory mapping dump file";
        return;
    }

    decode_data(mmf);
}

void SavingWindow::mouseMoveEvent(QMouseEvent *event)
{
    if ( event->buttons() == Qt::LeftButton )
    {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void SavingWindow::mousePressEvent(QMouseEvent *event)
{
    if ( event->buttons() == Qt::LeftButton )
    {
        prev_cursor_pos = event->globalPosition();
    }
}

SaverThread::SaverThread(SavingWindow *parent, RR_Map *resources_db_ptr, QStringList *src_files_ptr, bool is_debug, const QString &path)
    : my_parent(parent)
    , resources_db(resources_db_ptr)
    , src_files(src_files_ptr)
    , debug_mode(is_debug)
    , saving_path(path)
{

}

void SaverThread::run()
{
    if ( debug_mode ) Q_EMIT txDebugInfo("SAVING STARTS...");
    for(auto it = (*resources_db).cbegin(); it != (*resources_db).cend(); ++it)
    {
        int resources_count = it.value().count();
        if ( debug_mode ) Q_EMIT txDebugInfo(QString("\n> format: %1 (count: %2)").arg(it.key(), QString::number(resources_count)));
        for(int idx = 0; idx < resources_count; ++idx)
        {
            QFile src_file((*src_files)[it.value()[idx].src_fname_idx]);
            if ( !src_file.open(QIODeviceBase::ReadOnly) )
            {
                if ( debug_mode ) Q_EMIT txDebugInfo("error opening source file");
                continue;
            }
            QFile dst_file(saving_path + QString::number(it.value()[idx].order_number) + "." + it.value()[idx].dest_extension.toLower());
            if ( !dst_file.open(QIODeviceBase::ReadWrite) )
            {
                if ( debug_mode ) Q_EMIT txDebugInfo("error creating destination file");
                src_file.close();
                continue;
            }
            if ( !dst_file.resize(it.value()[idx].size) )
            {
                if ( debug_mode ) Q_EMIT txDebugInfo("error resizing destination file");
                src_file.close();
                dst_file.close();
                continue;
            }
            auto mmf_src = src_file.map(it.value()[idx].offset, it.value()[idx].size);
            if ( ( mmf_src == nullptr ) and debug_mode)
            {
                Q_EMIT txDebugInfo("error mapping source file to memory");
                src_file.close();
                dst_file.close();
                continue;
            }
            auto mmf_dst = dst_file.map(0, it.value()[idx].size);
            if ( ( mmf_dst == nullptr ) and debug_mode)
            {
                Q_EMIT txDebugInfo("error mapping destination file to memory");
                src_file.close();
                dst_file.close();
                continue;
            }

            memcpy(mmf_dst, mmf_src, it.value()[idx].size); // непосредственно копирование

            dst_file.unmap(mmf_dst);
            src_file.unmap(mmf_src);

            dst_file.close();
            src_file.close();

            Q_EMIT txNextWasSaved();
            if ( debug_mode) QThread::msleep(1000);
        }
    }
}

ReporterThread::ReporterThread(SavingWindow *parent, RR_Map *resources_db_ptr, QStringList *src_files_ptr, bool is_debug, const QString &path, int language, POD_SessionSettings *ss_ptr, QMap <QString, FormatStatistics> *fmt_counters_ptr)
    : my_parent(parent)
    , resources_db(resources_db_ptr)
    , src_files(src_files_ptr)
    , debug_mode(is_debug)
    , report_path(path)
    , lang_id(language)
    , session_settings(ss_ptr)
    , formats_counters(fmt_counters_ptr)
{

}

const QString html_report_created_txt[int(Langs::MAX)]
{
    "Report created : ",
    "Отчёт создан : "
};

const QString html_scanning_period_txt[int(Langs::MAX)]
{
    "Scanning period : %1 - %2",
    "Период сканирования : %1 - %2"
};

const QString date_time_w_msecs_template
{
    "dd.MM.yyyy hh:mm:ss.zzz"
};

const QString html_duration_txt[int(Langs::MAX)]
{
    "; duration : ",
    "; продолжительность : "
};

const QString html_msecs_txt[int(Langs::MAX)]
{
    " msec",
    " мсек"
};

const QString html_quick_links_txt[int(Langs::MAX)]
{
    "Quick links : ",
    "Быстрые ссылки : "
};

const QString html_total_found_txt[int(Langs::MAX)]
{
    "Total found ",
    "Всего найдено "
};

const QString html_total_size_txt[int(Langs::MAX)]
{
    " resource(s), overall size ",
    " ресурс(а,ов), общим размером "
};

const QString html_bytes_txt[int(Langs::MAX)]
{
    " byte(s)",
    " байт(а)"
};

const QString html_resource_number_txt[int(Langs::MAX)]
{
    "Resource number",
    "Номер ресурса"
};

const QString html_name_to_save_txt[int(Langs::MAX)]
{
    "Name to save",
    "Имя для сохранения"
};

const QString html_size_in_bytes_txt[int(Langs::MAX)]
{
        "Size, bytes",
        "Размер, байт"
};

const QString html_source_file_txt[int(Langs::MAX)]
{
    "Source file",
    "Исходный файл"
};

const QString html_file_offset_txt[int(Langs::MAX)]
{
    "Offset in file",
    "Смещение в файле"
};

const QString html_additional_info_txt[int(Langs::MAX)]
{
    "Additional info",
    "Дополнительно"
};

const QString html_resources_found_txt[int(Langs::MAX)]
{
    "Resources found : ",
    "Найдено ресурсов : "
};

const QString html_unique_fmts_found_txt[int(Langs::MAX)]
{
    "; unique formats : ",
    "; уникальных форматов : "
};

const QString html_source_files_txt[int(Langs::MAX)]
{
    "; source files : ",
    "; исходных файлов : "
};


void ReporterThread::run()
{
    QFile report_file(report_path);
    if ( !report_file.open(QIODeviceBase::WriteOnly) )
    {
        qInfo() << "error creating report file";
        report_file.close();
        return;
    }
    QTextStream text_stream(&report_file);
    text_stream <<
R"(<!DOCTYPE html>
<html>
<head>
<meta charset=\"utf-8\">
<title>Rapid Juicer :: )" << html_report_txt[lang_id] << "</title>\n";
    text_stream <<
R"(<style>
body {
    background-color: #6d8186;
    color: #ffffff;
    font-family: 'Courier New', 'monospace';
}

table {
    width: 1472px;
    table-layout: fixed;
    border-collapse: collapse;
}

table tbody tr:nth-child(odd){
    background: #8d6858;
    color: #d6e7e7;
    height: 32px;
}

table tbody tr:nth-child(even){
    background: #7d5848;
    color: #d6e7e7;
    height: 32px;
}

td {
    border: 3px solid #6d8186;
    padding-left: 8px;
    padding-right: 8px;
}

td:nth-child(4) {
    font-size: 11px;
    word-wrap: break-word;
}

th {
    border: 3px solid #6d8186;
    position: sticky;
    top: 0;
    z-index: 1;
    background-color: #555763
}

th:nth-child(1) {
    width: 160px;
}
th:nth-child(2) {
    width: 196px;
}
th:nth-child(3) {
    width: 160px;
}
th:nth-child(4) {
    width: 300px;
}
th:nth-child(5) {
    width: 256px;
}
th:nth-child(6) {
    width: 400px;
}

p {
    margin: 32px;
}
</style>
</head>
<body>
<center>
<header>)";
    text_stream << "<p><h2>" << VERSION_TEXT << "</h3>\n";
    text_stream << "<h3>" << html_report_created_txt[lang_id] << QDateTime::currentDateTime().toString(date_time_w_msecs_template) << "</h3>\n";
    QDateTime start_date_time;
    start_date_time.setMSecsSinceEpoch(session_settings->start_msecs);
    QDateTime end_date_time;
    end_date_time.setMSecsSinceEpoch(session_settings->end_msecs);
    text_stream << "<h4>" << QString(html_scanning_period_txt[lang_id]).arg(start_date_time.toString(date_time_w_msecs_template), end_date_time.toString(date_time_w_msecs_template));
    s64i duration = session_settings->end_msecs - session_settings->start_msecs;
    QTime duration_time(0, 0, 0);
    duration_time = duration_time.addMSecs(duration);
    text_stream << html_duration_txt[lang_id];
    if ( duration < 1000 ) // если менее 1 секунды, то отображаем в мсек
    {
        text_stream << duration << html_msecs_txt[lang_id];
    }
    else
    {
        text_stream << duration_time.toString(Qt::ISODateWithMs);
    }
    text_stream << ".</h4>\n<h4>";
    text_stream << html_resources_found_txt[lang_id] << session_settings->total_resources_found;
    text_stream << html_unique_fmts_found_txt[lang_id] << resources_db->count();
    text_stream << html_source_files_txt[lang_id] << src_files->count();
    text_stream << ".</h4>\n</p>\n</header>\n<hr>\n";

    /// раздел быстрых ссылок
    text_stream << "<nav>" << html_quick_links_txt[lang_id] << "<strong>";
    for(auto it = resources_db->cbegin(); it != resources_db->cend(); ++it) // формирование ссылок быстрой навигации
    {
        text_stream << "<a href=\"#" << it.key() << "\">" << it.key() << "</a>\n";
    }
    text_stream <<
R"(</strong>
</nav>
<hr>
<article>
)";
   ////

    for(auto it = resources_db->cbegin(); it != resources_db->cend(); ++it)
    {
        int resources_count = it.value().count();

        text_stream << "<section id=\"" << it.key() << "\">\n<p>\n<table>\n";
        text_stream << "<caption><h2>" << fformats[it.key()].extension << " (" << fformats[it.key()].description << ")";
        text_stream << "</h2><h4>" << html_total_found_txt[lang_id] << (*formats_counters)[it.key()].total_number_of;
        text_stream <<  html_total_size_txt[lang_id] << (*formats_counters)[it.key()].total_size_of << html_bytes_txt[lang_id] << " (" << human_readable_bytes((*formats_counters)[it.key()].total_size_of, lang_id) <<").</h4></caption>\n";
        text_stream << "<tr><th>" << html_resource_number_txt[lang_id] <<"</th>";
        text_stream << "<th>" << html_name_to_save_txt[lang_id] << "</th>";
        text_stream << "<th>" << html_size_in_bytes_txt[lang_id] << "</th>";
        text_stream << "<th>" << html_source_file_txt[lang_id] << "</th>";
        text_stream << "<th>" << html_file_offset_txt[lang_id] << "</th>";
        text_stream << "<th>" << html_additional_info_txt[lang_id] << "</th></tr>\n";

        for(int idx = 0; idx < resources_count; ++idx)
        {
            text_stream << "<tr>";
            text_stream << "<td>" << it.value()[idx].order_number << "</td>";
            text_stream << "<td>" << it.value()[idx].order_number << "." << it.value()[idx].dest_extension.toLower() << "</td>";
            text_stream << "<td>" << it.value()[idx].size << "</td>";
            text_stream << "<td>" << QDir::toNativeSeparators((*src_files)[it.value()[idx].src_fname_idx]) << "</td>";
            text_stream << "<td>" << QString("0x%1 (%2)").arg(QString::number(it.value()[idx].offset, 16), 8, '0').arg(QString::number(it.value()[idx].offset)) << "</td>";
            text_stream << "<td>" << it.value()[idx].info << "</td></tr>\n";
            Q_EMIT txNextWasReported();
        }
        text_stream <<"</table>\n</p>\n</section>\n";
    }
    text_stream <<
R"(<hr>
</article>
<footer>
<p>

</p>
</footer>
</center>
</body>
</html>
)";
    report_file.flush();
    report_file.close();
}
