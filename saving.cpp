#include "saving.h"
#include "formats.h"
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

extern QString reduce_file_path(const QString&, int);

const QString saving_to_txt[int(Langs::MAX)]
{
    "Saving to directory",
    "Сохранение в каталог"
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

SaveDBGWindow::SaveDBGWindow()
    : QWidget(nullptr)
{
   //this->setAttribute(Qt::WA_DeleteOnClose); // само удалится при закрытии
    this->setFixedSize(800, 600);

    info_text = new QTextEdit(this);
    info_text->move(8, 8);
    info_text->setFixedSize(this->width() - 16, this->height() - 16);
}

SavingWindow::SavingWindow(const QString &shm_key, const QString &shm_size, const QString &ssem_key, bool is_debug, const QString &language_id, const QString &screen)
    : QWidget(nullptr, Qt::FramelessWindowHint)
    , debug_mode(is_debug)
    , lang_id(language_id.toInt())
    , screen_name(screen)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setAttribute(Qt::WA_DeleteOnClose); // само удалится при закрытии
    this->setFixedSize(372, 164);

    //this->move(this->screen()->availableSize().width() - this->width() - 16, this->screen()->availableSize().height() - this->height() - 16); // двигаем в угол экрана

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
    saving_to_label->setText(saving_to_txt[lang_id]);

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
    //path_label->setPlainText(reduce_file_path(QDir::toNativeSeparators(saving_dir + "/a.a"), 86).chopped(3));

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
    //saved_label->setText("4296123456");

    remains_label = new QLabel(background); // количество оставшихся ресурсов
    remains_label->move(96, 108);
    remains_label->setFixedSize(196, 16);
    remains_label->setStyleSheet("color: #d9b855");
    remains_label->setFont(common_font);
    remains_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    //remains_label->setText("4296123456");

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
            break;
        }
    }
    ///

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
    saver->start(QThread::InheritPriority);
}

void SavingWindow::rxNextWasSaved()
{
    ++resources_saved;
    saved_label->setText(QString::number(resources_saved));
    remains_label->setText(QString::number(total_resources_found - resources_saved));
    progress_bar->setValue(resources_saved * 100 / total_resources_found);
}

void SavingWindow::rxDebugInfo(QString info)
{
    debug_window->info_text->append(info);
}

void SavingWindow::rxSaverFinished()
{
    //this->close();
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

    auto shm_buffer = (u8i*)shared_memory.data();
    TLV_Header *tlv_header;
    QString current_format;
    QString src_file;
    POD_ResourceRecord *current_pod_ptr;
    ResourceRecord current_rr;
    QString current_dest_ext;
    QString current_info;
    u64i next_header_offset = 0; // =0 - важно!
    do
    {
        tlv_header = (TLV_Header*)&shm_buffer[next_header_offset];
        if ( debug_mode) debug_window->info_text->append("\ntype:" + QString::number(int(tlv_header->type)) + " length:" + QString::number(int(tlv_header->length)));
        switch(tlv_header->type)
        {
        case TLV_Type::SrcFile: // эти TLV всегда идут первыми, один за одним
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
                formats_counters[current_format] = 0;
            }
            ++formats_counters[current_format];
            break;
        }
        case TLV_Type::POD:
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
            ///
            break;
        }
        case TLV_Type::Terminator:
        {
            if ( debug_mode) debug_window->info_text->append("-> Terminator:");
            break;
        }
        }
        next_header_offset += (sizeof(TLV_Header) + tlv_header->length);
    } while(tlv_header->type != TLV_Type::Terminator);

    if ( debug_mode) debug_window->info_text->append("\nTOTAL RESOURCES FOUND: " + QString::number(total_resources_found));

    shared_memory.detach();
    delete sys_semaphore;
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
