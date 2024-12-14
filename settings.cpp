#include "settings.h"
#include <QDebug>
#include <QThread>
#include <QDir>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QComboBox>
#include <QListWidget>
#include <QCommonStyle>
#include <QPushButton>
#include <QLineEdit>
#include <QMouseEvent>
#include <QApplication>
#include <QScrollBar>

extern Settings settings;

const QString settings_txt[int(Langs::MAX)][5]
{
    { "Interface Language :", "Read Buffer Size :", "Search In Subdirs :",  "File Search Mask :", "Exclude list :" },
    { "Язык интерфейса :",    "Буфер чтения :",     "Поиск в подкат-х. :",   "Файловая маска :",   "Исключать :" },
};

const QString languages_txt[int(Langs::MAX)][int(Langs::MAX)]
{
    { "English", "Russian" },
    { "Английский", "Русский"  }
};

extern const QString mebibytes_txt[int(Langs::MAX)]
{
    " MiB", " МиБ"
};

extern const QString yes_no_txt[int(Langs::MAX)][2]
{
    { "No",  "Yes" },
    { "Нет", "Да " }
};

const QString enter_here_txt[int(Langs::MAX)]
{
    "enter here",  "введите здесь"
};

const QString add_txt[int(Langs::MAX)]
{
    "Add",  "Добавить"
};

const QString del_txt[int(Langs::MAX)]
{
    "Delete",  "Удалить"
};

const QString clear_txt[int(Langs::MAX)]
{
    "Clear\nList",  "Очистить\nсписок"
};

const QString defaults_txt[int(Langs::MAX)]
{
    "Load\nDefaults",  "Настройки\nпо-умолч."
};

const QString ok_txt[int(Langs::MAX)]
{
    "OK",  "OK"
};

const QString cancel_txt[int(Langs::MAX)]
{
    "Cancel",  "Отмена"
};

extern const QList<u64i> permitted_buffers {2, 10, 50}; // разрешённые размеры буфера чтения (в мегабайтах)

const QString QS_LANGUAGE  {"lang_idx"};
const QString QS_BUFF_IDX  {"buff_idx"};
const QString QS_RECURSION {"recursion"};
const QString QS_FILE_MASK {"mask"};
const QString QS_EXCLUDING {"excluding"};

const Config default_config
{
    true,             // scrupulous
    u32i(Langs::Eng), // English language
    1,                // index 1 is 10Mbytes
    false,            // recursion
    "*.*",            // any files to search for
#ifdef _WIN64
    {"zip", "7z", "rar", "arj", "lzh", "ace"}
#elif defined(__gnu_linux__) || defined(__linux__)
    {"zip", "7z", "rar", "gz", "tgz", "tar.gz", "bz", "tbz", "tar.bz", "bz2", "tbz2", "tar.bz2", "lz", "lzo", "xz", "cpio"}
#elif defined(__APPLE__) || defined(__MACH__)
    {"zip", "7z", "rar", "gz", "tgz", "tar.gz", "bz", "tbz", "tar.bz", "bz2", "tbz2", "tar.bz2"}
#endif
};

const Skin default_skin
{
    "Roboto Mono",
    nullptr
};

Settings::Settings()
{
    threads_num = QThread::idealThreadCount();
    if (threads_num < 2) threads_num = 2;

    config = default_config; // эти значения могут быть переопределены при чтении файла настроек,
    skin   = default_skin;   // если таковой существует

    file_path = QDir::homePath() + "/.rj/";
    QDir dir;
    bool ok = dir.exists(file_path);
    if (!ok)
    {
        ok = dir.mkdir(file_path);
    }
    if (ok)
    {
        file.setFileName(file_path + "settings.json");
        if (!file.exists())
        {
            ok = file.open(QIODevice::ReadWrite | QIODevice::NewOnly); // создаём файл
        }
        else
        {
            ok = file.open(QIODevice::ReadWrite); // либо открываем существующий
            QByteArray ba {file.readAll()};
            QJsonParseError jspe;
            QJsonDocument js_doc = QJsonDocument::fromJson(ba, &jspe);
            if (!js_doc.isNull())
            {
                if ( !js_doc[QS_LANGUAGE].isUndefined() and (js_doc[QS_LANGUAGE].type() == QJsonValue::Double) )
                {
                    cand_config.lang_idx = js_doc[QS_LANGUAGE].toInt();
                    if ( (cand_config.lang_idx < int(Langs::MAX)) and (cand_config.lang_idx >= 0) )
                    {
                        config.lang_idx = cand_config.lang_idx;
                    }
                }
                if ( !js_doc[QS_BUFF_IDX].isUndefined() and (js_doc[QS_BUFF_IDX].type() == QJsonValue::Double) )
                {
                    cand_config.bfr_size_idx = js_doc[QS_BUFF_IDX].toInt();
                    if ( (cand_config.bfr_size_idx < (permitted_buffers.count())) and (config.bfr_size_idx >= 0) )
                    {
                        config.bfr_size_idx = cand_config.bfr_size_idx;
                    }
                }
                if ( !js_doc[QS_RECURSION].isUndefined() and (js_doc[QS_RECURSION].type() == QJsonValue::Double) )
                {
                    int recursion = js_doc[QS_RECURSION].toInt();
                    if ( (recursion == 0) or (recursion == 1))
                    {
                        config.recursion = recursion;
                    }
                }
                if ( (!js_doc[QS_FILE_MASK].isUndefined()) and (js_doc[QS_FILE_MASK].type() == QJsonValue::String) )
                {
                    cand_config.file_mask = js_doc[QS_FILE_MASK].toString();
                    if ( mask_validator(cand_config.file_mask) == QValidator::Acceptable )
                    {
                        config.file_mask = cand_config.file_mask;
                    }
                }
                if ( (!js_doc[QS_EXCLUDING].isUndefined()) and (js_doc[QS_EXCLUDING].type() == QJsonValue::Array) )
                {
                    QJsonArray jsa = js_doc[QS_EXCLUDING].toArray();
                    QString tmp_str;
                    for (auto it = jsa.cbegin(); it < jsa.cend(); ++it)
                    {
                        if (it->type() == QJsonValue::String)
                        {
                            tmp_str = it->toString();
                            if ( excluding_validator(it->toString(tmp_str)) == QValidator::Acceptable )
                            {
                                cand_config.excluding.append(tmp_str);
                            }
                        }
                    }
                    config.excluding = cand_config.excluding;
                    cand_config.excluding.clear();
                    cand_config.excluding.squeeze();
                }
            }
            else
            {
                qInfo() << "incorrect or absent json data in settings.json (possibly syntax error)";
            }
        }
    }
}

Settings::~Settings()
{
    delete skin.main_font;
    if (file.isOpen())
    {
        file.resize(0);
        QString write_str = QString(    "{\r\n"
                                        "\"%1\": %2,\r\n"
                                        "\"%3\": %4,\r\n"
                                        "\"%5\": %6,\r\n"
                                        "\"%7\": \"%8\",\r\n"
                                        "\"%9\": [%10]\r\n"
                                        "}"
                                    ).arg(
                                    QS_LANGUAGE,  QString::number(config.lang_idx),
                                    QS_BUFF_IDX,  QString::number(config.bfr_size_idx),
                                    QS_RECURSION, QString::number(config.recursion),
                                    QS_FILE_MASK, config.file_mask,
                                    QS_EXCLUDING, config.excluding.isEmpty() ? "" : "\"" + config.excluding.join(R"(", ")") + "\""
                                    );
        file.write(write_str.toUtf8());
        file.flush();
        file.close();
    }
}

void Settings::initSkin()
{
    skin.main_font = new QFont {skin.font_name};
}

u64i Settings::getBufferSizeInBytes()
{
    return permitted_buffers[config.bfr_size_idx] * 1024 * 1024;
}

u64i Settings::getBufferSizeByIndex(int idx)
{
    return permitted_buffers[idx];
}

u32i Settings::getThreadsNum()
{
    return threads_num;
}


QValidator::State mask_validator(const QString &input)
{
    static const QString forbidden_symbols {R"(&;|'"`[]()$<>{}^#\/%! )"}; // пробел тоже запрещён
    if ( input.isEmpty() or (input == ".") )
    {
        return QValidator::Intermediate;
    }
    for (auto && one_char : input) {
        for (auto && forbidden_char : forbidden_symbols) {
            if (one_char == forbidden_char) {
                return QValidator::Invalid;
            }
        }
    }
    return QValidator::Acceptable;
}

QValidator::State excluding_validator(const QString &input)
{
    static const QString forbidden_symbols {R"(*?&;|'"`[]()$<>{}^#\/%! )"}; // пробел тоже запрещён
    for (auto && one_char : input) {
        for (auto && forbidden_char : forbidden_symbols)
        {
            if (one_char == forbidden_char) {
                return QValidator::Invalid;
            }
        }
    }
    if (input.startsWith('.'))
    {
        return QValidator::Invalid;
    }
    return QValidator::Acceptable;
}


MaskValidator::MaskValidator(QObject *parent)
    : QValidator(parent)
{

}

QValidator::State MaskValidator::validate(QString &input, int &pos) const
{
    return mask_validator(input);
}

ExcludingValidator::ExcludingValidator(QObject *parent)
    : QValidator(parent)
{

}

QValidator::State ExcludingValidator::validate(QString &input, int &pos) const
{
    return excluding_validator(input);
}

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    const int y_offset {10};
    previous_lang_idx = settings.config.lang_idx; // запоминаем текущий язык, чтобы потом сравнить изменился ли он

    // копирование текущего конфига в конфиг-кандидат
    // дальнейшие изменения будут производиться только в кандидате
    settings.cand_config = settings.config;
    //

    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowModality(Qt::WindowModal);
    QPixmap bg_pixmap {":/gui/settings/sw.png"}; // background pixmap
    this->setFixedSize(bg_pixmap.size());
    background.setFixedSize(bg_pixmap.size());
    background.move(0, 0);
    background.setPixmap(bg_pixmap);

    QFont tmpFont {*skin_font()};
    tmpFont.setPixelSize(13);
    tmpFont.setBold(true);

    QLabel* tmp_label;
    for (int idx = 0; idx < 5; ++idx) { // надписи
        tmp_label = new QLabel;
        tmp_label->setFixedSize(164, 16);
        tmp_label->setFont(tmpFont);
        tmp_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tmp_label->setStyleSheet("color: #fffef9"/*; background-color: #009167"*/);
        tmp_label->setParent(&background);
        tmp_label->move(76, y_offset + 80 + 36 * idx);
        tmp_label->setText(settings_txt[curr_lang()][idx]);
    }

    auto lang_combo = new QComboBox(&background);
    lang_combo->setAttribute(Qt::WA_NoMousePropagation); // иначе при выборе и одновременном движении мышью начинало перемещаться всё окно
    lang_combo->setFixedSize(148, 28);
    lang_combo->setStyleSheet(
        "QComboBox::drop-down { width: 0px;}"
        "QComboBox {"
        "color: #fffef9;"
        "background-color: #4d7099;"
        "border-width: 2px;"
        "border-style: solid;"
        "border-radius: 14px;"
        "border-color: #b6c7c7;"
        "padding-left: 8px;}"
        );
    tmpFont.setPixelSize(12);
    tmpFont.setBold(false);
    lang_combo->setFont(tmpFont);
    lang_combo->move(252, y_offset + 74);
    for (int idx = 0; idx < int(Langs::MAX); ++idx) {
        lang_combo->addItem(languages_txt[curr_lang()][idx]);
    }
    lang_combo->setCurrentIndex(curr_lang());

    auto bufsz_combo = new QComboBox(&background);
    bufsz_combo->setAttribute(Qt::WA_NoMousePropagation); // иначе при выборе и одновременном движении мышью начинало перемещаться всё окно
    bufsz_combo->setFixedSize(72, 28);
    bufsz_combo->setStyleSheet(
        "QComboBox::drop-down { width: 0px;}"
        "QComboBox {"
        "color: #fffef9;"
        "background-color: #4d7099;"
        "border-width: 2px;"
        "border-style: solid;"
        "border-radius: 14px;"
        "border-color: #b6c7c7;"
        "padding-left: 8px;}"
        );
    bufsz_combo->setFont(tmpFont);
    bufsz_combo->move(252, y_offset + 110);
    auto maxBSVars = permitted_buffers.count();
    for (int idx = 0; idx < maxBSVars; ++idx) {
        bufsz_combo->addItem(QString::number(settings.getBufferSizeByIndex(idx)) + mebibytes_txt[curr_lang()]);
    }
    bufsz_combo->setCurrentIndex(settings.cand_config.bfr_size_idx);

    auto recurs_combo = new QComboBox(&background);
    recurs_combo->setAttribute(Qt::WA_NoMousePropagation); // иначе при выборе и одновременном движении мышью начинает перемещаться всё окно
    recurs_combo->setFixedSize(56, 28);
    recurs_combo->setStyleSheet(
        "QComboBox::drop-down { width: 0px;}"
        "QComboBox {"
        "color: #fffef9;"
        "background-color: #4d7099;"
        "border-width: 2px;"
        "border-style: solid;"
        "border-radius: 14px;"
        "border-color: #b6c7c7;"
        "padding-left: 8px;}"
        );
    recurs_combo->setFont(tmpFont);
    recurs_combo->move(252, y_offset + 146);
    recurs_combo->addItem(yes_no_txt[curr_lang()][false]); // "No"
    recurs_combo->addItem(yes_no_txt[curr_lang()][true]);  // "Yes"
    recurs_combo->setCurrentIndex(settings.cand_config.recursion);

    auto mask_le = new QLineEdit(&background); // file search mask
    mask_le->setFixedSize(178, 28);
    mask_le->setStyleSheet(
        "QLineEdit {"
        "color: #fffef9;"
        "background-color: #4d7099;"
        "border-width: 2px;"
        "border-style: solid;"
        "border-radius: 14px;"
        "border-color: #b6c7c7;"
        "padding-left: 8px;}"
        );
    mask_le->setFont(tmpFont);
    mask_le->move(252, y_offset + 182);
    mask_le->setClearButtonEnabled(true);
    mask_le->setText(settings.cand_config.file_mask);
    mask_le->setMaxLength(32);
    mask_le->setValidator(&fmv);

    tmpFont.setBold(true);
    tmpFont.setPixelSize(12);

    // Таблица исключений
    auto excluding_table = new QListWidget(&background);
    excluding_table->setFixedSize(96, 112);
    excluding_table->setFont(tmpFont);
    auto my_style = new QCommonStyle;
    excluding_table->verticalScrollBar()->setStyle(my_style);
    excluding_table->verticalScrollBar()->setFixedSize(10, 100);
    excluding_table->verticalScrollBar()->setStyleSheet(    ":vertical {background-color: #4d7099; width: 10px;}"
                                                        "::handle:vertical {background-color: #b6c7c7; border-radius: 5px; min-height: 32px;}"
                                                        "::sub-line:vertical {height: 0px;}"
                                                        "::add-line:vertical {height: 0px;}");
    excluding_table->setStyleSheet("QListWidget {"
                                   "color: #fffef9;"
                                   "gridline-color: #4d7099;"
                                   "background-color: #4d7099;"
                                   "border-width: 2px;"
                                   "border-style: solid;"
                                   "border-radius: 14px;"
                                   "border-color: #b6c7c7;"
                                   "padding-top: 4px;"
                                   "padding-bottom: 4px;"
                                   "padding-right: 6px;"
                                   "padding-left: 8px;}"
                                   );
    excluding_table->addItems(settings.cand_config.excluding);
    excluding_table->move(200, y_offset + 236);

    auto excluding_le = new QLineEdit(&background);
    excluding_le->setFixedSize(118, 28);
    excluding_le->setStyleSheet(
        "QLineEdit {"
        "color: #fffef9;"
        "background-color: #4d7099;"
        "border-width: 2px;"
        "border-style: solid;"
        "border-radius: 14px;"
        "border-color: #b6c7c7;"
        "padding-left: 8px;}"
        );
    excluding_le->setFont(tmpFont);
    excluding_le->move(312, y_offset + 236);
    excluding_le->setClearButtonEnabled(true);
    excluding_le->setMaxLength(16);
    excluding_le->setPlaceholderText(enter_here_txt[curr_lang()]);
    excluding_le->setValidator(&eev);

    static const QString buttons_style {    "QPushButton:enabled {color: #fffef9; background-color: #4d7099; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
        "QPushButton:hover   {color: #fffef9; background-color: #2d5079; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
        "QPushButton:pressed {color: #fffef9; background-color: #2d5079; border-width: 0px;}"
    };

    auto add_button = new QPushButton(&background);
    add_button->setAttribute(Qt::WA_NoMousePropagation);
    add_button->setFixedSize(96, 28);
    add_button->setStyleSheet(buttons_style);
    add_button->setFont(tmpFont);
    add_button->setText(add_txt[curr_lang()]);
    add_button->move(324, y_offset + 268);

    auto del_button = new QPushButton(&background);
    del_button->setAttribute(Qt::WA_NoMousePropagation);
    del_button->setFixedSize(96, 28);
    del_button->setStyleSheet(buttons_style);
    del_button->setFont(tmpFont);
    del_button->setText(del_txt[curr_lang()]);
    del_button->move(324, y_offset + 320);

    auto clear_button = new QPushButton(&background);
    clear_button->setAttribute(Qt::WA_NoMousePropagation);
    clear_button->setFixedSize(86, 56);
    clear_button->setStyleSheet(buttons_style);
    clear_button->setFont(tmpFont);
    clear_button->setText(clear_txt[curr_lang()]);
    clear_button->move(100, y_offset + 294);

    auto defaults_button = new QPushButton(&background);
    defaults_button->setAttribute(Qt::WA_NoMousePropagation);
    defaults_button->setFixedSize(86, 40);
    defaults_button->setStyleSheet( "QPushButton:enabled {color: #fffef9; background-color: #9ca14e; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                   "QPushButton:hover   {color: #fffef9; background-color: #7c812e; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                   "QPushButton:pressed {color: #fffef9; background-color: #7c812e; border-width: 0px;}"
                                   );
    defaults_button->setFont(tmpFont);
    defaults_button->setText(defaults_txt[curr_lang()]);
    defaults_button->move(80, y_offset + 384);

    auto ok_button = new QPushButton(&background);
    ok_button->setAttribute(Qt::WA_NoMousePropagation);
    ok_button->setFixedSize(72, 32);
    ok_button->setStyleSheet(   "QPushButton:enabled {color: #fffef9; background-color: #7fab6d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                             "QPushButton:hover   {color: #fffef9; background-color: #5f8b4d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                             "QPushButton:pressed {color: #fffef9; background-color: #5f8b4d; border-width: 0px;}"
                             );
    ok_button->setFont(tmpFont);
    ok_button->setText(ok_txt[curr_lang()]);
    ok_button->move(272, y_offset + 392);

    auto cancel_button = new QPushButton(&background);
    cancel_button->setAttribute(Qt::WA_NoMousePropagation);
    cancel_button->setFixedSize(72, 32);
    cancel_button->setStyleSheet(   "QPushButton:enabled {color: #fffef9; background-color: #ab6152; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                 "QPushButton:hover   {color: #fffef9; background-color: #8b4132; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                 "QPushButton:pressed {color: #fffef9; background-color: #8b4132; border-width: 0px;}"
                                 );
    cancel_button->setFont(tmpFont);
    cancel_button->setText(cancel_txt[curr_lang()]);
    cancel_button->move(356, y_offset + 392);

    connect(lang_combo, &QComboBox::activated, [](int index){
        settings.cand_config.lang_idx = index;
    });
    connect(bufsz_combo, &QComboBox::activated, [](int index){
        settings.cand_config.bfr_size_idx = index;
    });
    connect(recurs_combo, &QComboBox::activated, [](int index){
        settings.cand_config.recursion = bool(index);
    });
    connect(mask_le, &QLineEdit::editingFinished, [=](){
        settings.cand_config.file_mask = mask_le->text();
    });
    connect(add_button, &QPushButton::clicked, [=](){
        if ( (excluding_le->text().length() > 0) and (settings.cand_config.excluding.count() < 64) ) // ограничение на размер списка исключений
        {
            settings.cand_config.excluding.append(excluding_le->text());
            excluding_le->clear();
            excluding_table->addItem(settings.cand_config.excluding.last());
        }
    });
    connect(del_button, &QPushButton::clicked, [=](){
        int row_to_remove = excluding_table->currentRow();
        if ( row_to_remove >= 0 )
        {
            settings.cand_config.excluding.removeAt(row_to_remove);
            settings.cand_config.excluding.squeeze();
            excluding_table->clear();
            excluding_table->addItems(settings.cand_config.excluding);
            if ( row_to_remove < excluding_table->count() )
            {
                excluding_table->setCurrentRow(row_to_remove);
            }
            else
            {
                if (row_to_remove > 0)
                {
                    excluding_table->setCurrentRow(row_to_remove - 1);
                }
            }
        }
    });
    connect(clear_button, &QPushButton::clicked, [=](){
        settings.cand_config.excluding.clear();
        settings.cand_config.excluding.squeeze();
        excluding_table->clear();
    });
    connect(defaults_button, &QPushButton::clicked, [=](){
        settings.cand_config = default_config;
        lang_combo->clear();
        for (int idx = 0; idx < int(Langs::MAX); ++idx) {
            lang_combo->addItem(languages_txt[curr_lang()][idx]);
        }
        lang_combo->setCurrentIndex(settings.cand_config.lang_idx);
        bufsz_combo->clear();
        auto maxBSVars = permitted_buffers.count();
        for (int idx = 0; idx < maxBSVars; ++idx) {
            bufsz_combo->addItem(QString::number(settings.getBufferSizeByIndex(idx)) + mebibytes_txt[curr_lang()]);
        }
        bufsz_combo->setCurrentIndex(settings.cand_config.bfr_size_idx);
        recurs_combo->setCurrentIndex(settings.cand_config.recursion);
        mask_le->setText(settings.cand_config.file_mask);
        excluding_table->clear();
        excluding_table->addItems(settings.cand_config.excluding);
    });
    connect(ok_button, &QPushButton::clicked, [this](){
        settings.config = settings.cand_config;
        settings.cand_config.excluding.clear();
        settings.cand_config.excluding.squeeze();
        this->close();
    });
    connect(cancel_button, &QPushButton::clicked, [this](){
        settings.cand_config.excluding.clear();
        settings.cand_config.excluding.squeeze();
        this->close(); // после вызова close() модальное окно уничтожается автоматически
    });
}

SettingsWindow::~SettingsWindow()
{
    if ( settings.config.lang_idx != previous_lang_idx ) // изменился язык => разошлём всем gui-элементам событие
    {
        QApplication::postEvent(this->parent(), new QEvent(QEvent::LanguageChange)); // отсылаем родителю, т.е. CentralWidget
    }
}

void SettingsWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void SettingsWindow::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        prev_cursor_pos = event->globalPosition();
    }
}
