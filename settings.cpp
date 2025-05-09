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
#include <QMap>
#include <QSettings>

extern Settings *settings;
extern QMap <QString, FileFormat> fformats;

extern bool first_start;

const QString settings_txt[int(Langs::MAX)][6]
{
    { "Interface Language :", "Read Buffer Size :", "Search In Subdirs :",  "File Search Mask :", "OS Context Menu :",     "Exclude list :" },
    { "Язык интерфейса :",    "Буфер чтения :",     "Поиск в подкат-х. :",  "Файловая маска :",   "Контекстное меню ОС :", "Исключать :" },
};

const QString languages_txt[int(Langs::MAX)][int(Langs::MAX)]
{
    { "English", "Russian" },
    { "Английский", "Русский" }
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

extern const QString ok_txt[int(Langs::MAX)]
{
    "OK",  "OK"
};

const QString cancel_txt[int(Langs::MAX)]
{
    "Cancel",  "Отмена"
};

const QString scan_with_rj[int(Langs::MAX)]
{
    "Scan with Rapid Juicer", "Сканировать в Rapid Juicer"
};

extern const QList<u64i> permitted_buffers {2, 10, 50}; // разрешённые размеры гранулярности (в мибибайтах)

const QString QS_LANGUAGE  { "lang_idx" };
const QString QS_BUFF_IDX  { "buff_idx" };
const QString QS_RECURSION { "recursion" };
const QString QS_FILE_MASK { "mask" };
const QString QS_EXCLUDING { "excluding" };
const QString QS_SELECTED  { "selected" };
const QString QS_LAST_SRC  { "last_src_dir" };
const QString QS_LAST_DST  { "last_dst_dir" };
const QString QS_CTX_MENU  { "ctx_menu" };

const Config default_config
{
    true,             // scrupulous
    u32i(Langs::Eng), // English language
    2,                // index 2 is 50Mbytes
    true,             // recursion
    true,             // os context menu
    "*",              // any files to search for
#ifdef _WIN64
    {"zip", "7z", "rar", "arj", "lzh", "ace"},
#elif defined(__gnu_linux__) || defined(__linux__)
    {"zip", "7z", "rar", "gz", "tgz", "tar.gz", "bz", "tbz", "tar.bz", "bz2", "tbz2", "tar.bz2", "lz", "lzo", "xz", "cpio"},
#elif defined(__APPLE__) || defined(__MACH__)
    {"zip", "7z", "rar", "gz", "tgz", "tar.gz", "bz", "tbz", "tar.bz", "bz2", "tbz2", "tar.bz2"},
#endif
    "", // last_src_dir
    ""  // last_dst_dir
};

const Skin default_skin
{
    "Roboto Mono",
    nullptr
};

Settings::Settings()
{
    for (auto it = fformats.cbegin(); it != fformats.cend(); ++it) // сразу наполняем список выбранных форматов всеми форматами
    {
        selected_formats.insert(it.key());
    }
    config = default_config; // эти значения могут быть переопределены при чтении файла настроек,
    skin   = default_skin;   // если таковой существует

    file_path = QDir::homePath() + "/.rj/";
    QDir dir;
    bool ok = dir.exists(file_path);
    if ( !ok )
    {
        ok = dir.mkdir(file_path);
    }
    if ( ok )
    {
        file.setFileName(file_path + "settings.json");
        if ( !file.exists() )
        {
            ok = file.open(QIODevice::ReadWrite | QIODevice::NewOnly); // создаём файл
            if ( first_start ) return;
        }
        else
        {
            ok = file.open(QIODevice::ReadWrite); // либо открываем существующий
            if ( first_start ) return;
            if ( ok and ( file.size() <= 1024 * 1024 ) )
            {
                QByteArray ba {file.readAll()};
                QJsonParseError jspe;
                QJsonDocument js_doc = QJsonDocument::fromJson(ba, &jspe);
                if ( !js_doc.isNull() )
                {
                    if ( !js_doc[QS_LANGUAGE].isUndefined() and ( js_doc[QS_LANGUAGE].type() == QJsonValue::Double ) )
                    {
                        cand_config.lang_idx = js_doc[QS_LANGUAGE].toInt();
                        if ( ( cand_config.lang_idx < int(Langs::MAX) ) and ( cand_config.lang_idx >= 0 ) )
                        {
                            config.lang_idx = cand_config.lang_idx;
                        }
                    }
                    if ( !js_doc[QS_BUFF_IDX].isUndefined() and ( js_doc[QS_BUFF_IDX].type() == QJsonValue::Double ) )
                    {
                        cand_config.bfr_size_idx = js_doc[QS_BUFF_IDX].toInt();
                        if ( ( cand_config.bfr_size_idx < (permitted_buffers.count()) ) and ( config.bfr_size_idx >= 0 ) )
                        {
                            config.bfr_size_idx = cand_config.bfr_size_idx;
                        }
                    }
                    if ( !js_doc[QS_RECURSION].isUndefined() and ( js_doc[QS_RECURSION].type() == QJsonValue::Double ) )
                    {
                        int recursion = js_doc[QS_RECURSION].toInt();
                        if ( ( recursion == 0 ) or ( recursion == 1 ) )
                        {
                            config.recursion = recursion;
                        }
                    }
                    if ( ( !js_doc[QS_FILE_MASK].isUndefined() ) and ( js_doc[QS_FILE_MASK].type() == QJsonValue::String ) )
                    {
                        cand_config.file_mask = js_doc[QS_FILE_MASK].toString();
                        if ( mask_validator(cand_config.file_mask) == QValidator::Acceptable )
                        {
                            config.file_mask = cand_config.file_mask;
                        }
                    }
                    if ( ( !js_doc[QS_EXCLUDING].isUndefined() ) and ( js_doc[QS_EXCLUDING].type() == QJsonValue::Array ) )
                    {
                        QJsonArray jsa = js_doc[QS_EXCLUDING].toArray();
                        QString tmp_str;
                        for (auto it = jsa.cbegin(); it < jsa.cend(); ++it)
                        {
                            if ( it->type() == QJsonValue::String )
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
                    if ( ( !js_doc[QS_SELECTED].isUndefined() ) and ( js_doc[QS_SELECTED].type() == QJsonValue::Array ) )
                    {
                        QJsonArray jsa = js_doc[QS_SELECTED].toArray();
                        QString tmp_str;
                        selected_formats.clear();
                        for (auto it = jsa.cbegin(); it < jsa.cend(); ++it)
                        {
                            if ( it->type() == QJsonValue::String )
                            {
                                tmp_str = it->toString();
                                if ( fformats.contains(tmp_str) )
                                {
                                    selected_formats.insert(tmp_str);
                                }
                            }
                        }
                    }
                    if ( ( !js_doc[QS_LAST_SRC].isUndefined() ) and ( js_doc[QS_LAST_SRC].type() == QJsonValue::String ) )
                    {
                        config.last_src_dir = js_doc[QS_LAST_SRC].toString();
                    }
                    if ( ( !js_doc[QS_LAST_DST].isUndefined() ) and ( js_doc[QS_LAST_DST].type() == QJsonValue::String ) )
                    {
                        config.last_dst_dir = js_doc[QS_LAST_DST].toString();
                    }
                    if ( !js_doc[QS_CTX_MENU].isUndefined() and ( js_doc[QS_CTX_MENU].type() == QJsonValue::Double ) )
                    {
                        int ctx_menu = js_doc[QS_CTX_MENU].toInt();
                        if ( ( ctx_menu == 0 ) or ( ctx_menu == 1 ) )
                        {
                            config.ctx_menu = ctx_menu;
                        }
                    }
                }
                else
                {
                    qInfo() << "incorrect or absent json data in settings.json (possibly syntax error)";
                }
            }
            else
            {
                qInfo() << "error during opening settings file or size is too big (possibly corrupted)";
            }
        }
    }
}

Settings::~Settings()
{

}

void Settings::dump_to_file()
{
    delete skin.main_font;
    if ( file.isOpen() )
    {
        file.resize(0);
        QStringList selected_formats_list;
        for (auto &&one_format: selected_formats)
        {
            selected_formats_list.append(one_format);
        }
        QString write_str = QString("{\r\n"
                                    "\"%1\": %2,\r\n"
                                    "\"%3\": %4,\r\n"
                                    "\"%5\": %6,\r\n"
                                    "\"%7\": \"%8\",\r\n"
                                    "\"%9\": [%10],\r\n"
                                    "\"%11\": [%12],\r\n"
                                    "\"%13\": \"%14\",\r\n"
                                    "\"%15\": \"%16\",\r\n"
                                    "\"%17\": %18\r\n}"
                                    ).arg(
                                    QS_LANGUAGE,  QString::number(config.lang_idx),
                                    QS_BUFF_IDX,  QString::number(config.bfr_size_idx),
                                    QS_RECURSION, QString::number(config.recursion),
                                    QS_FILE_MASK, config.file_mask,
                                    QS_EXCLUDING, config.excluding.isEmpty() ? "" : "\"" + config.excluding.join(R"(", ")") + "\"",
                                    QS_SELECTED,  selected_formats.isEmpty() ? "" : "\"" + selected_formats_list.join(R"(",")") + "\"",
                                    QS_LAST_SRC,  config.last_src_dir,
                                    QS_LAST_DST,  config.last_dst_dir,
                                    QS_CTX_MENU,  QString::number(config.ctx_menu)
                                    );
        file.write(write_str.toUtf8());
        file.flush();
        file.close();
    }
}

void Settings::initSkin()
{
    skin.main_font = new QFont(skin.font_name);
    skin.main_font->setHintingPreference(QFont::PreferFullHinting);
    skin.main_font->setStyleHint(QFont::Monospace, QFont::PreferAntialias);
    skin.main_font->setKerning(true);
}

u64i Settings::getBufferSizeInBytes()
{
    return permitted_buffers[config.bfr_size_idx] * 1024 * 1024;
}

u64i Settings::getBufferSizeByIndex(int idx)
{
    return permitted_buffers[idx];
}

QValidator::State mask_validator(const QString &input)
{
    static const QString forbidden_symbols {R"(&;|'"`[]()$<>{}^#\/%! )"}; // пробел тоже запрещён
    if ( input.isEmpty() or ( input == "." ) )
    {
        return QValidator::Intermediate;
    }
    for (auto && one_char : input)
    {
        for (auto && forbidden_char : forbidden_symbols)
        {
            if ( one_char == forbidden_char )
            {
                return QValidator::Invalid;
            }
        }
    }
    return QValidator::Acceptable;
}

QValidator::State excluding_validator(const QString &input)
{
    static const QString forbidden_symbols {R"(*?&;|'"`[]()$<>{}^#\/%! )"}; // пробел тоже запрещён
    for (auto && one_char : input)
    {
        for (auto && forbidden_char : forbidden_symbols)
        {
            if ( one_char == forbidden_char )
            {
                return QValidator::Invalid;
            }
        }
    }
    if ( input.startsWith('.') )
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
    const int y_offset = 28;
    previous_lang_idx = settings->config.lang_idx; // запоминаем текущий язык, чтобы потом сравнить изменился ли он;

    // копирование текущего конфига в конфиг-кандидат
    // дальнейшие изменения будут производиться только в кандидате
    settings->cand_config = settings->config;
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

    QFont common_font {*skin_font()};
    common_font.setPixelSize(13);
    common_font.setBold(true);

    QLabel* tmp_label;
    for (int idx = 0; idx < 6; ++idx) // надписи
    {
        tmp_label = new QLabel;
        tmp_label->setFixedSize(164, 16);
        tmp_label->setFont(common_font);
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
    common_font.setPixelSize(12);
    common_font.setBold(false);
    lang_combo->setFont(common_font);
    lang_combo->move(252, y_offset + 74);
    for (int idx = 0; idx < int(Langs::MAX); ++idx)
    {
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
    bufsz_combo->setFont(common_font);
    bufsz_combo->move(252, y_offset + 110);
    auto maxBSVars = permitted_buffers.count();
    for (int idx = 0; idx < maxBSVars; ++idx) {
        bufsz_combo->addItem(QString::number(settings->getBufferSizeByIndex(idx)) + mebibytes_txt[curr_lang()]);
    }
    bufsz_combo->setCurrentIndex(settings->cand_config.bfr_size_idx);

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
    recurs_combo->setFont(common_font);
    recurs_combo->move(252, y_offset + 146);
    recurs_combo->addItem(yes_no_txt[curr_lang()][false]); // "No"
    recurs_combo->addItem(yes_no_txt[curr_lang()][true]);  // "Yes"
    recurs_combo->setCurrentIndex(settings->cand_config.recursion);

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
    mask_le->setFont(common_font);
    mask_le->move(252, y_offset + 182);
    mask_le->setClearButtonEnabled(true);
    mask_le->setText(settings->cand_config.file_mask);
    mask_le->setMaxLength(32);
    mask_le->setValidator(&fmv);

    auto ctx_menu_combo = new QComboBox(&background);
    ctx_menu_combo->setAttribute(Qt::WA_NoMousePropagation); // иначе при выборе и одновременном движении мышью начинает перемещаться всё окно
    ctx_menu_combo->setFixedSize(56, 28);
    ctx_menu_combo->setStyleSheet(
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
    ctx_menu_combo->setFont(common_font);
    ctx_menu_combo->move(252, y_offset + 218);
    ctx_menu_combo->addItem(yes_no_txt[curr_lang()][false]); // "No"
    ctx_menu_combo->addItem(yes_no_txt[curr_lang()][true]);  // "Yes"
    ctx_menu_combo->setCurrentIndex(settings->cand_config.ctx_menu);

    common_font.setBold(true);
    common_font.setPixelSize(12);

    const int eb_y_offset = 32; // excluding block additional y_offset

    // Таблица исключений
    auto excluding_table = new QListWidget(&background);
    excluding_table->setFixedSize(96, 112);
    excluding_table->setFont(common_font);
    auto my_style = new QCommonStyle;
    excluding_table->verticalScrollBar()->setStyle(my_style);
    excluding_table->verticalScrollBar()->setFixedSize(10, 100);
    excluding_table->verticalScrollBar()->setStyleSheet(":vertical {background-color: #4d7099; width: 10px;}"
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
    excluding_table->addItems(settings->cand_config.excluding);
    excluding_table->move(200, y_offset + 244 + eb_y_offset); // was 236

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
    excluding_le->setFont(common_font);
    excluding_le->move(312, y_offset + 244 + eb_y_offset);
    excluding_le->setClearButtonEnabled(true);
    excluding_le->setMaxLength(16);
    excluding_le->setPlaceholderText(enter_here_txt[curr_lang()]);
    excluding_le->setValidator(&eev);

    static const QString buttons_style {"QPushButton:enabled {color: #fffef9; background-color: #4d7099; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                        "QPushButton:hover   {color: #fffef9; background-color: #2d5079; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                        "QPushButton:pressed {color: #fffef9; background-color: #2d5079; border-width: 0px;}"
    };

    auto add_button = new QPushButton(&background);
    add_button->setAttribute(Qt::WA_NoMousePropagation);
    add_button->setFixedSize(96, 28);
    add_button->setStyleSheet(buttons_style);
    add_button->setFont(common_font);
    add_button->setText(add_txt[curr_lang()]);
    add_button->move(324, y_offset + 276 + eb_y_offset);

    auto del_button = new QPushButton(&background);
    del_button->setAttribute(Qt::WA_NoMousePropagation);
    del_button->setFixedSize(96, 28);
    del_button->setStyleSheet(buttons_style);
    del_button->setFont(common_font);
    del_button->setText(del_txt[curr_lang()]);
    del_button->move(324, y_offset + 328 + eb_y_offset);

    auto clear_button = new QPushButton(&background);
    clear_button->setAttribute(Qt::WA_NoMousePropagation);
    clear_button->setFixedSize(86, 56);
    clear_button->setStyleSheet(buttons_style);
    clear_button->setFont(common_font);
    clear_button->setText(clear_txt[curr_lang()]);
    clear_button->move(100, y_offset + 302 + eb_y_offset);

    auto defaults_button = new QPushButton(&background);
    defaults_button->setAttribute(Qt::WA_NoMousePropagation);
    defaults_button->setFixedSize(86, 40);
    defaults_button->setStyleSheet( "QPushButton:enabled {color: #fffef9; background-color: #9ca14e; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:hover   {color: #fffef9; background-color: #7c812e; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:pressed {color: #fffef9; background-color: #7c812e; border-width: 0px;}"
                                   );
    defaults_button->setFont(common_font);
    defaults_button->setText(defaults_txt[curr_lang()]);
    defaults_button->move(80, y_offset + 384 + eb_y_offset);

    auto ok_button = new QPushButton(&background);
    ok_button->setAttribute(Qt::WA_NoMousePropagation);
    ok_button->setFixedSize(72, 32);
    ok_button->setStyleSheet(   "QPushButton:enabled {color: #fffef9; background-color: #7fab6d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:hover   {color: #fffef9; background-color: #5f8b4d; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:pressed {color: #fffef9; background-color: #5f8b4d; border-width: 0px;}"
                             );
    ok_button->setFont(common_font);
    ok_button->setText(ok_txt[curr_lang()]);
    ok_button->move(272, y_offset + 392 + eb_y_offset);

    auto cancel_button = new QPushButton(&background);
    cancel_button->setAttribute(Qt::WA_NoMousePropagation);
    cancel_button->setFixedSize(72, 32);
    cancel_button->setStyleSheet(   "QPushButton:enabled {color: #fffef9; background-color: #ab6152; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:hover   {color: #fffef9; background-color: #8b4132; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                    "QPushButton:pressed {color: #fffef9; background-color: #8b4132; border-width: 0px;}"
                                 );
    cancel_button->setFont(common_font);
    cancel_button->setText(cancel_txt[curr_lang()]);
    cancel_button->move(356, y_offset + 392 + eb_y_offset);

    connect(lang_combo, &QComboBox::activated, [](int index){
        settings->cand_config.lang_idx = index;
    });
    connect(bufsz_combo, &QComboBox::activated, [](int index){
        settings->cand_config.bfr_size_idx = index;
    });
    connect(recurs_combo, &QComboBox::activated, [](int index){
        settings->cand_config.recursion = bool(index);
    });
    connect(mask_le, &QLineEdit::editingFinished, [=](){
        settings->cand_config.file_mask = mask_le->text();
    });
    connect(ctx_menu_combo, &QComboBox::activated, [](int index){
        settings->cand_config.ctx_menu = bool(index);
    });
    connect(add_button, &QPushButton::clicked, [=](){
        if ( (excluding_le->text().length() > 0) and (settings->cand_config.excluding.count() < 64) ) // ограничение на размер списка исключений
        {
            settings->cand_config.excluding.append(excluding_le->text());
            excluding_le->clear();
            excluding_table->addItem(settings->cand_config.excluding.last());
        }
    });
    connect(del_button, &QPushButton::clicked, [=](){
        int row_to_remove = excluding_table->currentRow();
        if ( row_to_remove >= 0 )
        {
            settings->cand_config.excluding.removeAt(row_to_remove);
            settings->cand_config.excluding.squeeze();
            excluding_table->clear();
            excluding_table->addItems(settings->cand_config.excluding);
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
        settings->cand_config.excluding.clear();
        settings->cand_config.excluding.squeeze();
        excluding_table->clear();
    });
    connect(defaults_button, &QPushButton::clicked, [=](){
        settings->cand_config = default_config;
        lang_combo->clear();
        for (int idx = 0; idx < int(Langs::MAX); ++idx)
        {
            lang_combo->addItem(languages_txt[curr_lang()][idx]);
        }
        lang_combo->setCurrentIndex(settings->cand_config.lang_idx);
        bufsz_combo->clear();
        auto maxBSVars = permitted_buffers.count();
        for (int idx = 0; idx < maxBSVars; ++idx)
        {
            bufsz_combo->addItem(QString::number(settings->getBufferSizeByIndex(idx)) + mebibytes_txt[curr_lang()]);
        }
        bufsz_combo->setCurrentIndex(settings->cand_config.bfr_size_idx);
        recurs_combo->setCurrentIndex(settings->cand_config.recursion);
        ctx_menu_combo->setCurrentIndex(settings->cand_config.ctx_menu);
        mask_le->setText(settings->cand_config.file_mask);
        excluding_table->clear();
        excluding_table->addItems(settings->cand_config.excluding);
    });
    connect(ok_button, &QPushButton::clicked, [this](){
        settings->config = settings->cand_config;
        settings->cand_config.excluding.clear();
        settings->cand_config.excluding.squeeze();
        this->close(); // после вызова close() модальное окно уничтожается автоматически
    });
    connect(cancel_button, &QPushButton::clicked, [this](){
        settings->cand_config.excluding.clear();
        settings->cand_config.excluding.squeeze();
        this->close(); // после вызова close() модальное окно уничтожается автоматически
    });
}

SettingsWindow::~SettingsWindow()
{
    if ( settings->config.lang_idx != previous_lang_idx ) // изменился язык => разошлём всем gui-элементам событие
    {
        QApplication::postEvent(this->parent(), new QEvent(QEvent::LanguageChange)); // отсылаем родителю, т.е. MainWindow
    }
#ifdef _WIN64
    settings->config.ctx_menu ? install_ctx_menu() : remove_ctx_menu();
#elif defined(__gnu_linux__) || defined(__linux__)
    // FFU
#elif defined(__APPLE__) || defined(__MACH__)
    // FFU
#endif
}

void SettingsWindow::install_ctx_menu()
{
#ifdef _WIN64
    QString rapid_juicer_path = std::move(QApplication::arguments()[0]);

    QSettings win_registry_for_file(R"(HKEY_CURRENT_USER\SOFTWARE\Classes\*\shell\)" + scan_with_rj[curr_lang()], QSettings::Registry64Format);
    win_registry_for_file.setValue("Icon", rapid_juicer_path + ",0");
    win_registry_for_file.setValue("SeparatorAfter", "");
    win_registry_for_file.setValue("SeparatorBefore", "");
    win_registry_for_file.beginGroup("command");
    win_registry_for_file.setValue(R"(Default)", QString(R"("%1" )").arg(rapid_juicer_path) + R"("-scan_file" "%1")");
    win_registry_for_file.endGroup();

    QSettings win_registry_for_dir(R"(HKEY_CURRENT_USER\SOFTWARE\Classes\Directory\shell\)" + scan_with_rj[curr_lang()], QSettings::Registry64Format);
    win_registry_for_dir.setValue("Icon", rapid_juicer_path + ",0");
    win_registry_for_dir.setValue("SeparatorAfter", "");
    win_registry_for_dir.setValue("SeparatorBefore", "");
    win_registry_for_dir.beginGroup("command");
    win_registry_for_dir.setValue(R"(Default)", QString(R"("%1" )").arg(rapid_juicer_path) + R"("-scan_dir" "%1")");
    win_registry_for_dir.endGroup();

    QSettings win_registry_for_drive(R"(HKEY_CURRENT_USER\SOFTWARE\Classes\Drive\shell\)" + scan_with_rj[curr_lang()], QSettings::Registry64Format);
    win_registry_for_drive.setValue("Icon", rapid_juicer_path + ",0");
    win_registry_for_drive.setValue("SeparatorAfter", "");
    win_registry_for_drive.setValue("SeparatorBefore", "");
    win_registry_for_drive.beginGroup("command");
    win_registry_for_drive.setValue(R"(Default)", QString(R"("%1" )").arg(rapid_juicer_path) + R"("-scan_drv" "%1")");
    win_registry_for_drive.endGroup();
#elif defined(__gnu_linux__) || defined(__linux__)
    // FFU
#elif defined(__APPLE__) || defined(__MACH__)
    // FFU
#endif
}

void SettingsWindow::remove_ctx_menu()
{
#ifdef _WIN64
    QSettings win_registry_for_file(R"(HKEY_CURRENT_USER\SOFTWARE\Classes\*\shell)", QSettings::Registry64Format);
    win_registry_for_file.remove(scan_with_rj[int(Langs::Eng)]);
    win_registry_for_file.remove(scan_with_rj[int(Langs::Rus)]);
    QSettings win_registry_for_dir(R"(HKEY_CURRENT_USER\SOFTWARE\Classes\Directory\shell)", QSettings::Registry64Format);
    win_registry_for_dir.remove(scan_with_rj[int(Langs::Eng)]);
    win_registry_for_dir.remove(scan_with_rj[int(Langs::Rus)]);
    QSettings win_registry_for_drive(R"(HKEY_CURRENT_USER\SOFTWARE\Classes\Drive\shell)", QSettings::Registry64Format);
    win_registry_for_drive.remove(scan_with_rj[int(Langs::Eng)]);
    win_registry_for_drive.remove(scan_with_rj[int(Langs::Rus)]);
#elif defined(__gnu_linux__) || defined(__linux__)
    // FFU
#elif defined(__APPLE__) || defined(__MACH__)
    // FFU
#endif
}

void SettingsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if ( event->buttons() == Qt::LeftButton )
    {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void SettingsWindow::mousePressEvent(QMouseEvent *event)
{
    if ( event->buttons() == Qt::LeftButton )
    {
        prev_cursor_pos = event->globalPosition();
    }
}
