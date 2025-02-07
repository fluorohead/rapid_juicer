// DIRLIST WINDOW
#include "pw.h"
#include "settings.h"
#include "task.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QMouseEvent>
#include <QFileDialog>

extern Settings *settings;
extern Task     task;

const QString paths_list_info_txt[int(Langs::MAX)]
{
    "To toggle directory recursion,\nset Yes or No in \"Recursion\" column",
    "Для переключения обхода подкаталогов\nвыберите Yes или No в колонке \"Рекурсия\""
};

const QString paths_table_header_txt[int(Langs::MAX)][3]
{
    { "", "Recurs.", "Path mask"  },
    { "", "Рекурс.", "Маска пути" }
};

const QString remove_all_txt[int(Langs::MAX)]
{
    "   Clear list",
    "   Удалить всё"
};

YesNoMicroButton::YesNoMicroButton(QWidget *parent, u32i row_index, QPixmap *no_state_pixmap,
                                   QPixmap *yes_state_pixmap,
                                   QPixmap *no_state_hover_pixmap,
                                   QPixmap *yes_state_hover_pixmap)
    : QLabel(parent)
    , my_row_index(row_index)
    , taskIdx(row_index - 1)
    , main_pixmap{no_state_pixmap, yes_state_pixmap}
    , hover_pixmap{no_state_hover_pixmap, yes_state_hover_pixmap}
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setFixedSize(main_pixmap[0]->size());
    this->setPixmap(*main_pixmap[task.getTaskRecursion(taskIdx)]);

    this->setMask(main_pixmap[0]->mask());
}

void YesNoMicroButton::txUpdateMyRowIndex(u32i removed_row_index)
{
    if (my_row_index > removed_row_index) {
        taskIdx--;
        my_row_index--;
    }
}

void YesNoMicroButton::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        event->accept();
    }
}

void YesNoMicroButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // settings.setTaskRecursionFlag(taskIdx, !task.getTaskRecursion(taskIdx));
        task.setTaskRecursion(taskIdx, !task.getTaskRecursion(taskIdx));
        //qInfo() << "Recursion set to" << task.getTaskRecursion(taskIdx) << "in task_id=" << taskIdx << "in row at index=" << my_row_index;
        this->setPixmap(*hover_pixmap[task.getTaskRecursion(taskIdx)]);
        event->accept();
    }
}

void YesNoMicroButton::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
}

void YesNoMicroButton::enterEvent(QEnterEvent *event)
{
    this->setPixmap(*hover_pixmap[task.getTaskRecursion(taskIdx)]);
    event->accept();
}

void YesNoMicroButton::leaveEvent(QEvent *event)
{
    this->setPixmap(*main_pixmap[task.getTaskRecursion(taskIdx)]);
    event->accept();
}

YesNoMicroButton::~YesNoMicroButton()
{
    // qInfo() << "yes/no-microbutton destructor called";
}

DeleteMicroButton::DeleteMicroButton(QWidget *parent, u32i row_index, QPixmap *main_pixmap, QPixmap *hover_pixmap)
    : QLabel(parent)
    , my_row_index(row_index)
    , main_pixmap(main_pixmap)
    , hover_pixmap(hover_pixmap)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setFixedSize(main_pixmap->size());
    this->setPixmap(*main_pixmap);
    //this->setMask(main_pixmap->mask());
}

DeleteMicroButton::~DeleteMicroButton()
{
    // qInfo() << "delete-microbutton destructor called";
}

void DeleteMicroButton::txUpdateMyRowIndex(u32i removed_row_index)
{
    if (my_row_index > removed_row_index) {
        my_row_index--;
    }
}

void DeleteMicroButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        Q_EMIT imReleased(my_row_index);
        event->accept();
    }
}

void DeleteMicroButton::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        event->accept();
    }
}

void DeleteMicroButton::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
}

void DeleteMicroButton::enterEvent(QEnterEvent *event)
{
    this->setPixmap(*hover_pixmap);
    event->accept();
}

void DeleteMicroButton::leaveEvent(QEvent *event)
{
    this->setPixmap(*main_pixmap);
    event->accept();
}

PathsTable::PathsTable(QWidget *parent)
    : QTableWidget(parent)
{
    this->verticalHeader()->hide();
    this->horizontalHeader()->hide();
    this->setColumnCount(3);
    this->setStyleSheet("color: #dfdfdf; gridline-color: #b4b4b4; background-color: #929167");
    this->setColumnWidth(0, DIRTABLE_COL0_W);
    this->setColumnWidth(1, DIRTABLE_COL1_W);
    this->setRowCount(DIRTABLE_MAX_PATHS + 1); // где "+1" это строка №0, которая будет нести функцию заголовка
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setFocusPolicy(Qt::NoFocus); // отключаем прямоугольник фокуса при клике на ячейке
    this->setEditTriggers(QAbstractItemView::NoEditTriggers); // отключаем редактирование ячеек
    this->setFocusPolicy(Qt::NoFocus);
    this->setFrameShape(QFrame::NoFrame);
    this->setSortingEnabled(false);
    this->horizontalHeader()->setHighlightSections(false);
    this->horizontalHeader()->setStretchLastSection(true);
    this->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    this->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    this->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    //this->horizontalScrollBar()->hide();
    my_style = new QCommonStyle;
    this->verticalScrollBar()->setStyle(my_style);
    this->verticalScrollBar()->setStyleSheet(   ":vertical {background-color: #7e7b5f; width: 10px}"
                                                "::handle:vertical {background-color: #6e6b4f; border-radius: 5px; min-height: 48px}"
                                                "::sub-line:vertical {height: 0px}"
                                                "::add-line:vertical {height: 0px}");

    fill_header();

    QFont tmpFont {*skin_font()};
    tmpFont.setBold(true);
    tmpFont.setPixelSize(13); // выставляем размер шрифта для всего, кроме строки 0 (для неё шрифт выставляется в методе drawZeroRowHeader)
    this->setFont(tmpFont);

    for (u32i row_idx = 0; row_idx < (DIRTABLE_MAX_PATHS + 1); ++row_idx) // выставляем высоту всех строк, включая "заголовок" в строке 0
    {
        this->setRowHeight(row_idx, DIRTABLE_ROW_H);
    }
}

PathsTable::~PathsTable()
{
    delete my_style;
}

void PathsTable::fill_header()
{
    QFont tmpFont {*skin_font()};
    tmpFont.setPixelSize(12);
    tmpFont.setBold(true);
    for (u32i column = 0; column < 3; ++column) // выставляем настройки всех трёх колонок строки 0, которая вместо обычного заголовка таблицы
    {
        auto header_label = new QLabel(paths_table_header_txt[curr_lang()][column]);
        header_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        header_label->setFont(tmpFont);
        header_label->setStyleSheet("color: #e7e7e7");
        this->setCellWidget(0, column, header_label);
    }
}

void PathsTable::update_header()
{
    for (u32i column = 0; column < 3; ++column) // выставляем настройки всех трёх колонок строки 0, которая вместо обычного заголовка таблицы
    {
        ((QLabel*)this->cellWidget(0, column))->setText(paths_table_header_txt[curr_lang()][column]);
    }
}

void PathsTable::rxRemoveRow(u32i row_index)
{
    // qInfo() << "removeRow("<< row_index << ") called";
    task.delTaskPath(row_index - 1);
    this->setUpdatesEnabled(false); // чтобы виджет не перерисовывался, пока идёт изменение количества строк
    this->setRowCount(this->rowCount() + 1);
    this->setRowHeight(this->rowCount() - 1, DIRTABLE_ROW_H);
    this->removeRow(row_index);
    Q_EMIT txUpdateYourRowIndexes(row_index);
    Q_EMIT txUpdatePathsButton();
    this->setUpdatesEnabled(true); // закончили изменения, можно перерисоваться
}

void PathsTable::rxRemoveAll()
{
    this->setUpdatesEnabled(false); // чтобы виджет не перерисовывался, пока идёт изменение количества строк
    task.delAllTaskPaths();
    this->clear();
    fill_header();
    Q_EMIT txUpdatePathsButton();
    this->setUpdatesEnabled(true); // закончили изменения, можно перерисоваться
}

CornerGrip::CornerGrip(QWidget *parent)
    : QLabel(parent)
{
    QPixmap pixmap {":/gui/paths/grip.png"};
    this->setFixedSize(pixmap.size());
    this->setMask(pixmap.mask());
    this->setPixmap(pixmap);
}

void CornerGrip::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        int x_diff = (event->globalPosition() - prev_cursor_pos).toPoint().x();
        int y_diff = (event->globalPosition() - prev_cursor_pos).toPoint().y();
        Q_EMIT txDiffXY(x_diff, y_diff);
        prev_cursor_pos = event->globalPosition();
        event->accept();
    }
}

void CornerGrip::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        prev_cursor_pos = event->globalPosition();
        event->accept();
    }
}

DirlistWindow::DirlistWindow(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setMinimumSize(DIRLIST_MIN_WIDTH, DIRLIST_MIN_HEIGHT);
    this->resize(DIRLIST_MIN_WIDTH, DIRLIST_MIN_HEIGHT);

    QPixmap tmpPixmap;

    tmpPixmap.load(":/gui/paths/dlwh_l.png"); // header left
    header_height = tmpPixmap.height();
    header_left_width = tmpPixmap.width();
    header_left.move(0, 0);
    header_left.resize(header_left_width, header_height);
    header_left.setPixmap(tmpPixmap);

    tmpPixmap.load(":/gui/paths/dlwh_r.png"); // header right
    header_right_width = tmpPixmap.width();
    header_right.move(current_width - header_right_width, 0);
    header_right.resize(header_right_width, header_height);
    header_right.setPixmap(tmpPixmap);

    hfiller_width = current_width - header_right_width - header_left_width; // = 1

    tmpPixmap.load(":/gui/paths/dlwf_l.png"); // footer left
    footer_height = tmpPixmap.height();
    int footerBeginY = current_height - footer_height;
    footer_left_width = tmpPixmap.width();
    footer_left.move(0, footerBeginY);
    footer_left.resize(footer_left_width, footer_height);
    footer_left.setPixmap(tmpPixmap);

    tmpPixmap.load(":/gui/paths/dlwf_r.png"); // footer right
    footer_right_width = tmpPixmap.width();
    footer_right.move(current_width - footer_right_width, footerBeginY);
    footer_right.resize(footer_right_width, footer_height);
    footer_right.setPixmap(tmpPixmap);

    tmpPixmap.load(":/gui/paths/dlwh_hf.png"); // header horizontal filler
    header_hfiller.move(header_left_width, 0);
    header_hfiller.resize(hfiller_width, header_height);
    header_hfiller.setScaledContents(true);
    header_hfiller.setPixmap(tmpPixmap);

    tmpPixmap.load(":/gui/paths/dlwf_hf.png"); // footer horizontal filler
    footer_hfiller.move(footer_left_width, footerBeginY);
    footer_hfiller.resize(hfiller_width, footer_height);
    footer_hfiller.setScaledContents(true);
    footer_hfiller.setPixmap(tmpPixmap);

    tmpPixmap.load(":/gui/paths/dlwb_vfl.png"); // body vertical filler left
    body_vfiller_left_width = tmpPixmap.width();
    body_height = current_height - header_height - footer_height;
    body_vfiller_left.move(0, header_height);
    body_vfiller_left.setScaledContents(true);
    body_vfiller_left.resize(body_vfiller_left_width, body_height);
    body_vfiller_left.setPixmap(tmpPixmap);

    tmpPixmap.load(":/gui/paths/dlwb_vfr.png"); // body vertical filler right
    body_vfiller_right_width = tmpPixmap.width();
    body_vfiller_right.move(current_width - body_vfiller_left_width, header_height);
    body_vfiller_right.setScaledContents(true);
    body_vfiller_right.resize(body_vfiller_right_width, body_height);
    body_vfiller_right.setPixmap(tmpPixmap);

    dirtable_width  = current_width - 48;
    dirtable_height = current_height - footer_height - 8;
    dirtable.move(24, 24);
    dirtable.raise();

    grip.move(footer_right_width - grip.width() - 8, footer_height - grip.height() - 8);

    QFont tmpFont {*skin_font()};
    tmpFont.setItalic(true);
    tmpFont.setBold(false);
    tmpFont.setPixelSize(12);

    info_label.setFixedSize(368, 32);
    info_label.setStyleSheet("color: #fffef9"/*;  background-color: #009167"*/);
    info_label.setFont(tmpFont);
    info_label.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    info_label.move(36, current_height - 44);
    info_label.setText(paths_list_info_txt[curr_lang()]);

    close_button.move(current_width - 128, current_height - 40);
    minimize_button.move(current_width - 88, current_height - 40);

    tmpPixmap.load(":/gui/paths/tc_lbl.png");
    trashcan_label.resize(tmpPixmap.size());
    trashcan_label.setPixmap(tmpPixmap);
    tclWidthHalf =  trashcan_label.width() / 2;
    trashcan_label.move(432 + (((current_width - 132) - 432)/2 - tclWidthHalf), current_height - 45);

    tmpFont.setItalic(false);
    tmpFont.setBold(false);
    tmpFont.setPixelSize(13);

    trashcan_text.setFixedSize(128, 24);
    trashcan_text.setStyleSheet("color: #fffef9"/*;  background-color: #009167"*/);
    trashcan_text.setFont(tmpFont);
    trashcan_text.setAlignment(Qt::AlignCenter);
    trashcan_text.move(16, 6);
    trashcan_text.setText(remove_all_txt[curr_lang()]);

    cleanAll.move(148, 5);

    connect(&cleanAll, &OneStateButton::imReleased, &dirtable, &PathsTable::rxRemoveAll);
    connect(&grip, &CornerGrip::txDiffXY, this, &DirlistWindow::rxDiffXY);
    connect(&close_button, &OneStateButton::imReleased, this, &DirlistWindow::hide);
    connect(&minimize_button, &OneStateButton::imReleased, this, &DirlistWindow::showMinimized);

    paths_count = &task.paths_count;
}

void DirlistWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void DirlistWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        prev_cursor_pos = event->globalPosition();
    }
}

void DirlistWindow::paintEvent(QPaintEvent *event)
{
    header_right.move(current_width - header_right_width, 0);
    footer_left.move(0, current_height - footer_height);
    footer_right.move(current_width - footer_right_width, current_height - footer_height);
    body_height = current_height - header_height - footer_height;
    body_vfiller_left.resize(body_vfiller_left_width, body_height);
    body_vfiller_right.move(current_width - body_vfiller_right_width, header_height);
    body_vfiller_right.resize(body_vfiller_right_width, body_height);
    hfiller_width = current_width - header_left_width - header_right_width;
    header_hfiller.resize(hfiller_width, header_height);
    footer_hfiller.move(footer_left_width, current_height - footer_height);
    footer_hfiller.resize(hfiller_width, footer_height);
    dirtable_width  = current_width - 48;
    dirtable_height = current_height - footer_height - 8;
    dirtable.resize(dirtable_width, dirtable_height);
    info_label.move(36, current_height - 44);
    trashcan_label.move(432 + (((current_width - 132) - 432)/2 - tclWidthHalf), current_height - 45);
    close_button.move(current_width - 128, current_height - 40);
    minimize_button.move(current_width - 88, current_height - 40);
}

void DirlistWindow::changeEvent(QEvent *event)
{
    if ( event->type() == QEvent::LanguageChange )
    {
        info_label.setText(paths_list_info_txt[curr_lang()]);
        trashcan_text.setText(remove_all_txt[curr_lang()]);
        dirtable.update_header();
    }
    event->accept();
}

void DirlistWindow::rxAddFilenames(QStringList filenames)
{
    for (auto && one : filenames) {
        if (*paths_count < DIRTABLE_MAX_PATHS) {
            if (!task.isTaskPathPresent(one)) {
                task.addTaskPath(TaskPath {one, "", false});
                auto delete_button = new DeleteMicroButton(nullptr, *paths_count, &delete_pixmap, &delete_hover_pixmap);
                delete_button->setFixedSize(delete_pixmap.size());
                dirtable.setCellWidget(*paths_count, 0, delete_button);
                connect(delete_button, &DeleteMicroButton::imReleased, &dirtable, &PathsTable::rxRemoveRow);
                connect(&dirtable, &PathsTable::txUpdateYourRowIndexes, delete_button, &DeleteMicroButton::txUpdateMyRowIndex);
                auto path_item = new QTableWidgetItem(QDir::toNativeSeparators(one));
                path_item->setFlags(Qt::NoItemFlags);
                dirtable.setItem(*paths_count, 2, path_item);
            }
        }
    }
}

void DirlistWindow::rxAddDirname(QString dirname)
{
    // в dirname путь с перевёрнутыми слэшами a'la linux

    if ((*paths_count < DIRTABLE_MAX_PATHS) and (!task.isTaskPathPresent(dirname))) {
        if ( ( dirname.length() > 1 ) and ( dirname.endsWith('/') ) ) // чтобы исключить добавление пути "C:/", к которому потом прицепится второй '/' : поэтому удаляем '/', если он есть и если это не linux (len > 1)
        {
            dirname.removeLast();
        }
        task.addTaskPath(TaskPath {dirname, settings->config.file_mask, settings->config.recursion});
        auto delete_button = new DeleteMicroButton(nullptr, *paths_count, &delete_pixmap, &delete_hover_pixmap);
        delete_button->setFixedSize(delete_pixmap.size());
        dirtable.setCellWidget(*paths_count, 0, delete_button);
        connect(delete_button, &DeleteMicroButton::imReleased, &dirtable, &PathsTable::rxRemoveRow);
        connect(&dirtable, &PathsTable::txUpdateYourRowIndexes, delete_button, &DeleteMicroButton::txUpdateMyRowIndex);
        auto yesno_button = new YesNoMicroButton(nullptr, *paths_count, &no_pixmap, &yes_pixmap, &no_hover_pixmap, &yes_hover_pixmap);
        yesno_button->setFixedSize(no_pixmap.size());
        dirtable.setCellWidget(*paths_count, 1, yesno_button);
        connect(&dirtable, &PathsTable::txUpdateYourRowIndexes, yesno_button, &YesNoMicroButton::txUpdateMyRowIndex);
        auto path_item = new QTableWidgetItem(QDir::toNativeSeparators(dirname) + QDir::separator() + settings->config.file_mask);
        path_item->setFlags(Qt::NoItemFlags);
        dirtable.setItem(*paths_count, 2, path_item);
    }
}

void DirlistWindow::rxDiffXY(int x_diff, int y_diff)
{
    int new_width  = current_width  + x_diff;
    int new_height = current_height + y_diff;
    if ( new_width  < DIRLIST_MIN_WIDTH ) new_width = current_width;
    if ( new_height < DIRLIST_MIN_HEIGHT ) new_height = current_height;
    if ((new_width != current_width) or ( new_height != current_height ))
    {
        current_width = new_width;
        current_height = new_height;
        this->resize(new_width, new_height);
    }
}

void DirlistWindow::remove_all()
{
    dirtable.rxRemoveAll();
}


PathsWindow::PathsWindow()
    : QWidget(nullptr, Qt::FramelessWindowHint)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setMinimumSize(DIRLIST_MIN_WIDTH, DIRLIST_MIN_HEIGHT);
    this->resize(DIRLIST_MIN_WIDTH, DIRLIST_MIN_HEIGHT);

    central_widget = new QLabel(this);
    central_widget->move(0, 0);
    central_widget->resize(DIRLIST_MIN_WIDTH, DIRLIST_MIN_HEIGHT);
    central_widget->setObjectName("pwcw");
    central_widget->setStyleSheet("QLabel#pwcw {background-color: #929167; border-width: 4px; border-style: solid; border-radius: 36px; border-color: #665f75;}");

    grip = new CornerGrip(central_widget);

    paths_table = new PathsTable(central_widget);
    paths_table->move(24, 24);

    close_button = new OneStateButton(central_widget, ":/gui/paths/dlw_close.png", ":/gui/paths/dlw_close_h.png");
    minimize_button = new OneStateButton(central_widget, ":/gui/paths/dlw_min.png", ":/gui/paths/dlw_min_h.png");

    yes_pixmap = new QPixmap(":/gui/paths/rcrs_yes.png");
    no_pixmap = new QPixmap(":/gui/paths/rcrs_no.png");
    yes_hover_pixmap = new QPixmap(":/gui/paths/rcrs_yes_h.png");
    no_hover_pixmap = new QPixmap(":/gui/paths/rcrs_no_h.png");
    delete_pixmap = new QPixmap(":/gui/paths/dt_del.png");
    delete_hover_pixmap = new QPixmap(":/gui/paths/dt_del_h.png");

    QFont common_font {*skin_font()};
    common_font.setItalic(false);
    common_font.setBold(false);
    common_font.setPixelSize(12);

    info_label = new QLabel(central_widget);
    info_label->setFixedSize(344, 36);
    info_label->setStyleSheet("color: #fffef9; background-color: #997751; border-width: 0px; border-radius: 18px;");
    info_label->setFont(common_font);
    info_label->setAlignment(Qt::AlignCenter);
    info_label->setText(paths_list_info_txt[curr_lang()]);

    common_font.setBold(true);
    common_font.setPixelSize(12);

    remove_all_label = new QLabel(central_widget);
    remove_all_label->setFixedSize(152, 36);
    remove_all_label->setStyleSheet("color: #fffef9; background-color: #997751; border-width: 0px; border-radius: 18px;");
    remove_all_label->setFont(common_font);
    remove_all_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    remove_all_label->setText(remove_all_txt[curr_lang()]);

    remove_all_button = new OneStateButton(remove_all_label, ":/gui/paths/tc_btn.png", ":/gui/paths/tc_btn_h.png");
    remove_all_button->move(112, 5);

    connect(grip, &CornerGrip::txDiffXY, this, &PathsWindow::rxDiffXY);
    connect(close_button, &OneStateButton::imReleased, this, &PathsWindow::hide);
    connect(minimize_button, &OneStateButton::imReleased, this, &PathsWindow::showMinimized);
    connect(remove_all_button, &OneStateButton::imReleased, paths_table, &PathsTable::rxRemoveAll);

    paths_count = &task.paths_count;
}

PathsWindow::~PathsWindow()
{
    delete no_pixmap;
    delete yes_pixmap;
    delete no_hover_pixmap;
    delete yes_hover_pixmap;
    delete delete_pixmap;
    delete delete_hover_pixmap;
}

void PathsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if ( event->buttons() == Qt::LeftButton )
    {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void PathsWindow::mousePressEvent(QMouseEvent *event)
{
    if ( event->buttons() == Qt::LeftButton )
    {
        prev_cursor_pos = event->globalPosition();
    }
}

void PathsWindow::paintEvent(QPaintEvent *event)
{
    central_widget->resize(current_width, current_height);
    grip->move(current_width - grip->width() - 8, current_height - grip->height() - 8);
    paths_table_width  = current_width - 48;
    paths_table_height = current_height - 78;
    paths_table->resize(paths_table_width, paths_table_height);
    close_button->move(current_width - 128, current_height - 40);
    minimize_button->move(current_width - 88, current_height - 40);
    info_label->move(20, current_height - 46);
    remove_all_label->move(432 + (((current_width - 132) - 432)/2 - remove_all_label->width() / 2), current_height - 45);
}

void PathsWindow::changeEvent(QEvent *event)
{
    if ( event->type() == QEvent::LanguageChange )
    {
        info_label->setText(paths_list_info_txt[curr_lang()]);
        remove_all_label->setText(remove_all_txt[curr_lang()]);
        paths_table->update_header();
    }
    event->accept();
}

void PathsWindow::rxDiffXY(int x_diff, int y_diff)
{
    int new_width  = current_width  + x_diff;
    int new_height = current_height + y_diff;
    if ( new_width < DIRLIST_MIN_WIDTH ) new_width = current_width;
    if ( new_height < DIRLIST_MIN_HEIGHT ) new_height = current_height;
    if ( ( new_width != current_width ) or ( new_height != current_height ) )
    {
        current_width = new_width;
        current_height = new_height;
        this->resize(new_width, new_height); // вызывает paintEvent, где все дочерние элементы будут изменять положение и размер
    }
}

void PathsWindow::rxAddFilenames(QStringList filenames)
{
    for (auto && one : filenames) {
        if ( *paths_count < DIRTABLE_MAX_PATHS )
        {
            if ( !task.isTaskPathPresent(one) )
            {
                task.addTaskPath(TaskPath {one, "", false});
                auto delete_button = new DeleteMicroButton(nullptr, *paths_count, delete_pixmap, delete_hover_pixmap);
                delete_button->setFixedSize(delete_pixmap->size());
                paths_table->setCellWidget(*paths_count, 0, delete_button);
                connect(delete_button, &DeleteMicroButton::imReleased, paths_table, &PathsTable::rxRemoveRow);
                connect(paths_table, &PathsTable::txUpdateYourRowIndexes, delete_button, &DeleteMicroButton::txUpdateMyRowIndex);
                auto path_item = new QTableWidgetItem(QDir::toNativeSeparators(one));
                path_item->setFlags(Qt::NoItemFlags);
                paths_table->setItem(*paths_count, 2, path_item);
            }
        }
    }
}

void PathsWindow::rxAddDirname(QString dirname)
{
    // в dirname путь с перевёрнутыми слэшами a'la linux
    if ( ( *paths_count < DIRTABLE_MAX_PATHS ) and ( !task.isTaskPathPresent(dirname) ) )
    {
        if ( ( dirname.length() > 1 ) and ( dirname.endsWith('/') ) ) // чтобы исключить добавление пути "C:/", к которому потом прицепится второй '/' : поэтому удаляем '/', если он есть и если это не linux (len > 1)
        {
            dirname.removeLast();
        }
        task.addTaskPath(TaskPath {dirname, settings->config.file_mask, settings->config.recursion});
        auto delete_button = new DeleteMicroButton(nullptr, *paths_count, delete_pixmap, delete_hover_pixmap);
        delete_button->setFixedSize(delete_pixmap->size());
        paths_table->setCellWidget(*paths_count, 0, delete_button);
        connect(delete_button, &DeleteMicroButton::imReleased, paths_table, &PathsTable::rxRemoveRow);
        connect(paths_table, &PathsTable::txUpdateYourRowIndexes, delete_button, &DeleteMicroButton::txUpdateMyRowIndex);
        auto yesno_button = new YesNoMicroButton(nullptr, *paths_count, no_pixmap, yes_pixmap, no_hover_pixmap, yes_hover_pixmap);
        yesno_button->setFixedSize(no_pixmap->size());
        paths_table->setCellWidget(*paths_count, 1, yesno_button);
        connect(paths_table, &PathsTable::txUpdateYourRowIndexes, yesno_button, &YesNoMicroButton::txUpdateMyRowIndex);
        auto path_item = new QTableWidgetItem(QDir::toNativeSeparators(dirname) + QDir::separator() + settings->config.file_mask);
        path_item->setFlags(Qt::NoItemFlags);
        paths_table->setItem(*paths_count, 2, path_item);
    }
}

void PathsWindow::remove_all()
{
    paths_table->rxRemoveAll();
}
