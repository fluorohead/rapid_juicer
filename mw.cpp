#include "mw.h"
#include "settings.h"
#include "task.h"
#include "session.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>
#include <QCommonStyle>
#include <QScrollBar>
#include <QFileDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QApplication>

extern QHash <u32i, QString> categories;
extern QMap <QString, FileFormat> fformats;

extern Task task;
extern SessionsPool sessions_pool;

const QString filter_labels_txt[int(Langs::MAX)][MAX_FILTERS]
{
    { "raster/vector",    "video/animation", "audio/music",  "3D objects/scenes", "platform specific", "performance risk", "outdated",   "other formats", "all formats" },
    { "растры/векторные", "видео/анимация",  "аудио/музыка", "3D объекты/сцены",  "платф.зависимые",   "сложные",          "устаревшие", "другие",        "все форматы" }
};

const QString scrup_mode_txt[int(Langs::MAX)]
{
    "scrupulous mode:",
    "тщательный поиск:"
};

const QString tasks_label_txt[int(Langs::MAX)]
{
    "Running Tasks:",
    "Задач в работе:"
};

const QString paths_button_txt[int(Langs::MAX)]
{
    "Paths: %1 (Max.100)",
    "Путей: %1 (Мax.100)"
};

const QString header_txt[int(Langs::MAX)][3]
{
    { "Form.", "Description", "Category"  },
    { "Форм.", "Описание",    "Категория" }
};

const u64i filters[MAX_FILTERS] { CAT_IMAGE,
                                  CAT_VIDEO,
                                  CAT_AUDIO | CAT_MUSIC,
                                  CAT_3D,
#ifdef _WIN64
                                  CAT_DOS | CAT_MAC | CAT_AMIGA | CAT_ATARI | CAT_NIX,
#elif defined(__gnu_linux__) || defined(__linux__)
                                  CAT_DOS | CAT_AMIGA | CAT_ATARI | CAT_MAC,
#elif defined(__APPLE__) || defined(__MACH__)
                                  CAT_DOS | CAT_AMIGA | CAT_ATARI | CAT_NIX,
#endif
                                  CAT_PERFRISK,
                                  CAT_OUTDATED,
                                  CAT_OTHER,
                                  CAT_IMAGE | CAT_VIDEO | CAT_AUDIO | CAT_3D | CAT_MUSIC | CAT_OTHER | CAT_PERFRISK | CAT_OUTDATED };

const QMap <u64i, QString> FormatsTable::categories_resources {
    { CAT_IMAGE, ":/gui/main/cat_image.png"}, {CAT_VIDEO,    ":/gui/main/cat_video.png"},    {CAT_AUDIO,    ":/gui/main/cat_audio.png"},
    { CAT_MUSIC, ":/gui/main/cat_music.png"}, {CAT_3D,       ":/gui/main/cat_3d.png"},       {CAT_WIN,      ":/gui/main/cat_win.png"},
    { CAT_DOS,   ":/gui/main/cat_dos.png"},   {CAT_MAC,      ":/gui/main/cat_mac.png"},      {CAT_AMIGA,    ":/gui/main/cat_amiga.png"},
    { CAT_WEB,   ":/gui/main/cat_web.png"},   {CAT_OUTDATED, ":/gui/main/cat_outdated.png"}, {CAT_PERFRISK, ":/gui/main/cat_perfrisk.png"}
};

FilterButton::FilterButton(QWidget *parent, FilterAction action, u64i categories, QPoint position, QPixmap *main_pixmap, QPixmap *hover_pixmap)
    : QLabel(parent)
    , my_action(action)
    , my_categories(categories)
    , my_main_pixmap(main_pixmap)
    , my_hover_pixmap(hover_pixmap)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setFixedSize(my_main_pixmap->width(), my_main_pixmap->height());
    this->setPixmap(*my_main_pixmap);
    this->setMask(main_pixmap->mask());
    this->move(position);
}

void FilterButton::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        event->accept();
    }
}

void FilterButton::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit imReleased(my_action, my_categories);
        event->accept();
    }
}

void FilterButton::mouseMoveEvent(QMouseEvent *event) {
    event->accept();
}

void FilterButton::enterEvent(QEnterEvent *event) {
    event->accept();
    this->setPixmap(*my_hover_pixmap);
}

void FilterButton::leaveEvent(QEvent *event) {
    event->accept();
    this->setPixmap(*my_main_pixmap);
}

FormatLabel::FormatLabel(const QString &format_key, const QString &text, QPixmap *corner_pixmap, QWidget *parent)
    : my_format_key(format_key)
    , QLabel(text, parent)
    , corner_pixmap(corner_pixmap)
{
    corner_label.setFixedSize(50, 50);
    corner_label.setMask(corner_pixmap->mask());
    if ( settings.selected_formats.contains(my_format_key) ) corner_label.setPixmap(*corner_pixmap);
}

void FormatLabel::rxToggle()
{
    if ( settings.selected_formats.contains(my_format_key) ) {
        settings.selected_formats.remove(my_format_key);
        corner_label.clear();
    }
    else
    {
        settings.selected_formats.insert(my_format_key);
        corner_label.setPixmap(*corner_pixmap);
    }
}

void FormatLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) rxToggle();
    event->accept();
}

DescriptionLabel::DescriptionLabel(const QString &text, QWidget *parent):
    QLabel(text, parent)
{
}

DescriptionLabel::DescriptionLabel(QWidget *parent):
    QLabel(parent)
{
}

void DescriptionLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) emit txToggle();
    event->accept();
}

CategoryLabel::CategoryLabel(QString id, const QMap<u64i, QPixmap *> &pixmaps, QWidget *parent)
    : QLabel(parent)
{
    this->setFixedSize(108, 50 - 1);
    int next_x_pos = 8;
    for (u32i cat_idx = 0; cat_idx < 3; ++cat_idx) {
        if (fformats[id].base_categories[cat_idx] != CAT_NONE) {
             auto cat_icon = new QLabel(this);
             cat_icon->setFixedSize(28, 28);
             cat_icon->setStyleSheet("QToolTip:enabled {background : #888663; color: #ffffff; border: 0px}");
             cat_icon->setToolTip(categories[fformats[id].base_categories[cat_idx]]);
             cat_icon->setPixmap(*pixmaps[fformats[id].base_categories[cat_idx]]);
             cat_icon->move(next_x_pos, (50 - 28) / 2);
             next_x_pos += (28 + 6);
        }
    }
}

void CategoryLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) emit txToggle();
    event->accept();
}

FormatsTable::FormatsTable(QWidget *parent)
    : QTableWidget(parent)
{
    this->setFixedSize(FORMATS_TABLE_W, FORMATS_TABLE_H);
    this->setColumnCount(3);
    this->setStyleSheet("color: #cdc8c7; gridline-color: #a4a4a4; background-color: #794642");
    this->setColumnWidth(0, FORMATS_TABLE_COL0_W);
    this->setColumnWidth(1, FORMATS_TABLE_COL1_W);
    this->setRowCount(fformats.size() + 1);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->verticalHeader()->hide();
    this->horizontalHeader()->hide();
    this->setFrameShape(QFrame::NoFrame);
    this->setSortingEnabled(false);
    this->horizontalHeader()->setHighlightSections(false);
    this->horizontalHeader()->setStretchLastSection(true);
    this->horizontalHeader()->setFixedHeight(24);
    this->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    this->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    this->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    this->verticalScrollBar()->setStyle(new QCommonStyle);
    this->verticalScrollBar()->setStyleSheet(           ":vertical {background-color: #794642; width: 10px}"
                                                        "::handle:vertical {background-color: #895652; border-radius: 5px;}"
                                                        "::sub-line:vertical {height: 0px}"
                                                        "::add-line:vertical {height: 0px}");

    prepCategPixmaps();

    QFont column0_font {*skin_font()};
    column0_font.setPixelSize(14);
    column0_font.setBold(true);
    QFont header_font {column0_font};
    header_font.setPixelSize(11);
    QFont column1_font {column0_font};
    column1_font.setPixelSize(13);
    column1_font.setBold(false);
    QFont comment_font {column1_font};
    comment_font.setPixelSize(11);
    comment_font.setBold(false);
    comment_font.setItalic(true);
    this->setRowHeight(0, 24);
    for (int column = 0; column <= 2; ++column)
    {
        auto header_label = new QLabel(header_txt[curr_lang()][column]);
        header_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        header_label->setFont(header_font);
        header_label->setStyleSheet("color: #b4a384");
        this->setCellWidget(0, column, header_label);
    }
    for (auto it = fformats.begin(); it != fformats.end(); ++it)
    {
        this->setRowHeight(it.value().index + 1, FORMATS_TABLE_ROW_H);
        auto fmt_label = new FormatLabel(it.key(), /*&it.value().selected,*/ it.value().extension, &corner_fmt_pixmap);
        fmt_label->setFixedSize(FORMATS_TABLE_COL0_W - 1, FORMATS_TABLE_ROW_H - 1);
        fmt_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        fmt_label->setFont(column0_font);
        this->setCellWidget(it.value().index + 1, 0, fmt_label);
        DescriptionLabel *descr_label;
        if (it.value().commentary.isEmpty())
        { // column 1
            descr_label = new DescriptionLabel(it.value().description);
            descr_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            descr_label->setFont(column1_font);
            descr_label->setStyleSheet("QToolTip:enabled {background : #888663; color: #ffffff; border: 0px}");
            descr_label->setToolTip(it.value().cat_str);
        }
        else
        {
            descr_label = new DescriptionLabel;
            descr_label->setFixedSize(FORMATS_TABLE_COL1_W - 1, FORMATS_TABLE_ROW_H - 1);
            descr_label->setStyleSheet("QToolTip:enabled {background : #888663; color: #ffffff; border: 0px}");
            descr_label->setToolTip(it.value().cat_str);
            auto child_descr_label = new QLabel(descr_label);
            child_descr_label->setFixedSize(FORMATS_TABLE_COL1_W, FORMATS_TABLE_ROW_H / 2);
            child_descr_label->setFont(column1_font);
            child_descr_label->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
            child_descr_label->setText(it.value().description);
            child_descr_label->move(0, 0);
            auto child_comment_label = new QLabel(descr_label);
            child_comment_label->setFixedSize(FORMATS_TABLE_COL1_W, FORMATS_TABLE_ROW_H / 2);
            child_comment_label->setFont(comment_font);
            child_comment_label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
            child_comment_label->setText(QString("(%1)").arg(it.value().commentary));
            child_comment_label->move(0, FORMATS_TABLE_ROW_H / 2);
        }
        connect(descr_label, &DescriptionLabel::txToggle, fmt_label, &FormatLabel::rxToggle);
        this->setCellWidget(it.value().index + 1, 1, descr_label);

        auto categ_label = new CategoryLabel(it.key(), categories_pixmaps); // column 2
        connect(categ_label, &CategoryLabel::txToggle, fmt_label, &FormatLabel::rxToggle);
        this->setCellWidget(it.value().index + 1, 2, categ_label);
    }
    delCategPixmaps();
}

void FormatsTable::prepCategPixmaps()
{
    for (auto it = categories_resources.begin(); it != categories_resources.end(); ++it)
    {
        categories_pixmaps[it.key()] = new QPixmap(it.value());
    }

}

void FormatsTable::delCategPixmaps()
{
    for (auto it = categories_pixmaps.begin(); it != categories_pixmaps.end(); ++it)
    {
        delete it.value();
    }
    categories_pixmaps.clear();
}

void FormatsTable::rxCommand(FilterAction action, u64i categories)
{
    for (auto it = fformats.begin(); it != fformats.end(); ++it) // проход по всем форматам
    {
        for (u32i bcIdx = 0; bcIdx < 3; ++bcIdx) { // проход по базовым категориям отдельного формата
            if ( it.value().base_categories[bcIdx] & categories ) // если совпадёт хотя бы одна
            {
                switch (action)
                {
                case FilterAction::Include:
                    if ( !settings.selected_formats.contains(it.key()) )
                    {
                        ((FormatLabel*)this->cellWidget(it.value().index + 1, 0))->rxToggle();
                    }
                    break;
                case FilterAction::Exclude:
                    if ( settings.selected_formats.contains(it.key()) )
                    {
                        ((FormatLabel*)this->cellWidget(it.value().index + 1, 0))->rxToggle();
                    }
                    break;
                default:;
                }
                break;
            }
        }
    }
}

CentralWidget::CentralWidget(QWidget *parent, DirlistWindow *dirlist)
    : QLabel(parent)
    , _dirlist(dirlist)
{
    QPixmap background {":/gui/main/cw.png"};
    this->setFixedSize(background.size());
    this->move(0, 0);
    this->setPixmap(background);
    play_button.move(232, 96);
    add_file_button.move(23, 32);
    add_dir_button.move(187, 32);
    settings_button.move(133, 26);
    scrup_button.move(176, 110);
    minimize_button.move(778, 50);
    close_button.move(684, 8);
    formats_table.move(336, 42);

    QFont tmpFont {*skin_font()};
    tmpFont.setPixelSize(13);
    tmpFont.setBold(false);

    // кнопка Тщательный режим
    scrup_label.move(32, 112);
    scrup_label.resize(140, 24);
    scrup_label.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    scrup_label.setStyleSheet("color: #b6c7c7");
    scrup_label.setFont(tmpFont);
    scrup_label.setText(scrup_mode_txt[curr_lang()]);

    // Фильтры категорий
    tmpFont.setBold(true);
    for (int idx = 0; idx < MAX_FILTERS; ++idx)
    {
        categ_labels[idx] = new QLabel(filter_labels_txt[curr_lang()][idx], this);
        categ_labels[idx]->setFixedSize(144, 24);
        categ_labels[idx]->move(40, 175 + 35 * idx);
        categ_labels[idx]->setFont(tmpFont);
        categ_labels[idx]->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        categ_labels[idx]->setStyleSheet("color: #b6c7c7");
        includeButtons[idx] = new FilterButton(this, FilterAction::Include, filters[idx], {193, 177 + 35 * int(idx)}, &filter_main_pixmaps[0], &filter_hover_pixmaps[0]);
        excludeButtons[idx] = new FilterButton(this, FilterAction::Exclude, filters[idx], {242, 177 + 35 * int(idx)}, &filter_main_pixmaps[1], &filter_hover_pixmaps[1]);
        connect(includeButtons[idx], &FilterButton::imReleased, &formats_table, &FormatsTable::rxCommand);
        connect(excludeButtons[idx], &FilterButton::imReleased, &formats_table, &FormatsTable::rxCommand);
    }

    // Лейбл с информацией о версии
    tmpFont.setBold(false);
    version_label.move(32, 504);
    version_label.setStyleSheet("color: #fffef9");
    version_label.setFont(tmpFont);

    // Кнопка с количеством добавленных путей
    paths_button = new DynamicInfoButton(this, ":/gui/main/pth_btn.png", ":/gui/main/pth_btn_h.png", paths_button_txt, 12, 10, &task.paths_count);
    paths_button->move(424, 499);

    // Лейбл с информацией о количестве запущенных задач
    tmpFont.setItalic(true);
    tmpFont.setPixelSize(12);
    tasks_label.move(624, 504);
    tasks_label.setStyleSheet("color: #fffef9");
    tasks_label.setFont(tmpFont);
    tasks_label.setText(tasks_label_txt[curr_lang()]);

    connect(&add_file_button, &OneStateButton::imReleased, this, &CentralWidget::addFiles);
    connect(&add_dir_button, &OneStateButton::imReleased, this, &CentralWidget::addDir);
    connect(this, &CentralWidget::txFilenames, dirlist, &DirlistWindow::rxAddFilenames);
    connect(this, &CentralWidget::txDirname, dirlist, &DirlistWindow::rxAddDirname);
    connect(&minimize_button, &OneStateButton::imReleased, parent, &MainWindow::showMinimized);
    connect(&close_button, &OneStateButton::imReleased, parent, &MainWindow::close);
    connect(&dirlist->dirtable, &DirlistTable::txUpdatePathsButton, paths_button, &DynamicInfoButton::updateText);
    connect(paths_button, &DynamicInfoButton::imReleased, dirlist, &DirlistWindow::show);
    connect(&settings_button, &OneStateButton::imReleased, this, &CentralWidget::showSettings);
    connect(&play_button, &OneStateButton::imReleased, this, &CentralWidget::showNewSessionWindow);
}

void CentralWidget::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore(); // отправляем событие ниже до MainWindow
}

void CentralWidget::mousePressEvent(QMouseEvent *event)
{
    event->ignore(); // отправляем событие ниже до MainWindow
}

void CentralWidget::changeEvent(QEvent *event)
{
    if ( event->type() == QEvent::LanguageChange )
    {
        scrup_label.setText(scrup_mode_txt[curr_lang()]);
        paths_button->updateText(true);
        tasks_label.setText(tasks_label_txt[curr_lang()]);
        for (int idx = 0; idx < MAX_FILTERS; ++idx)
        {
            categ_labels[idx]->setText(filter_labels_txt[curr_lang()][idx]);
        }
        ((QLabel*)formats_table.cellWidget(0, 0))->setText(header_txt[curr_lang()][0]);
        ((QLabel*)formats_table.cellWidget(0, 1))->setText(header_txt[curr_lang()][1]);
        ((QLabel*)formats_table.cellWidget(0, 2))->setText(header_txt[curr_lang()][2]);
        QApplication::postEvent(_dirlist, new QEvent(QEvent::LanguageChange)); // отсылаем в DirlistWindow
    }
    event->accept();
}

void CentralWidget::addFiles()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this);
    if (!filenames.isEmpty()) {
        emit txFilenames(filenames);
        paths_button->updateText();
    }
}

void CentralWidget::addDir()
{
    QString dirname = QFileDialog::getExistingDirectory(this);
    if (!dirname.isEmpty()) {
        emit txDirname(dirname);
        paths_button->updateText();
    }
}

void CentralWidget::showSettings()
{
    SettingsWindow* tw = new SettingsWindow(this);
    tw->show(); // модальное окно уничтожается автоматически после вызова close()
}

void CentralWidget::showNewSessionWindow()
{
    u32i new_session_id = sessions_pool.get_new_session_id();
    if ( new_session_id != 0 )
    {
        auto new_session_window = new SessionWindow(new_session_id);
        if ( sessions_pool.write_new_session(new_session_window, new_session_id) == 0 )
        {
            // если по какой-то причине не удалось записать указатель в пул с переданным id, тогда уничтожаем окно
            delete new_session_window;
        }
        else
        {
            new_session_window->show();
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    central.move(0, 0);
    this->resize(central.size());
}

MainWindow::~MainWindow()
{

}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        prev_cursor_pos = event->globalPosition();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (u32i idx = 0; idx < sessions_pool._size; ++idx) // закрываем окна сессий, если они есть
    {
        if ( sessions_pool.pool[idx] != nullptr )
        {
            sessions_pool.pool[idx]->close();
        }
    }
    dirlist.close();
}
