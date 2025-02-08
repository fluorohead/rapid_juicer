//PATHS WINDOW
#ifndef PW_H
#define PW_H

#include "elements.h"
#include <QLabel>
#include <QTableWidget>
#include <QCommonStyle>

#define PATHS_WINDOW_MIN_WIDTH 1043
#define PATHS_WINDOW_MIN_HEIGHT 361
#define PATHS_TABLE_ROW_H 26
#define PATHS_TABLE_MAX_PATHS 100
#define PATHS_TABLE_COL0_W 32
#define PATHS_TABLE_COL1_W 59

class MainWindow;
class PathsWindow;

class YesNoMicroButton: public QLabel
{
    Q_OBJECT
    QPixmap* main_pixmap[2];
    QPixmap* hover_pixmap[2];
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    u32i taskIdx;
    u32i my_row_index;
public:
    YesNoMicroButton(QWidget *parent, u32i row_index,   QPixmap *no_state_pixmap,
                                                        QPixmap *yes_state_pixmap,
                                                        QPixmap *no_state_hover_pixmap,
                                                        QPixmap *yes_state_hover_pixmap);
    ~YesNoMicroButton();
public Q_SLOTS:
    void txUpdateMyRowIndex(u32i removed_row_index);
};

class DeleteMicroButton: public QLabel
{
    Q_OBJECT
    QPixmap *main_pixmap;
    QPixmap *hover_pixmap;
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    u32i my_row_index;
public:
    DeleteMicroButton(QWidget *parent, u32i row_index, QPixmap *main_pixmap, QPixmap *hover_pixmap);
    ~DeleteMicroButton();
Q_SIGNALS:
    void imReleased(u32i row_index);
public Q_SLOTS:
    void txUpdateMyRowIndex(u32i removed_row_index);
};

class PathsTable: public QTableWidget
{
    Q_OBJECT
    QCommonStyle *my_style;
    void fill_header();
    void update_header();
public:
    PathsTable(QWidget *parent = nullptr);
    ~PathsTable();
Q_SIGNALS:
    void txUpdateYourRowIndexes(u32i removed_row_index); // посылает всем DeleteMicroButton и YesNoMicroButton
    void txUpdatePathsButton(bool forced = false); // посылает кнопке типа DynamicInfoButton в виджете CentralWidget
public Q_SLOTS:
    void rxRemoveRow(u32i row_index);
    void rxRemoveAll();
    friend PathsWindow;
};

class CornerGrip: public QLabel
{
    Q_OBJECT
    QPointF prev_cursor_pos;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
public:
    CornerGrip(QWidget *parent);
Q_SIGNALS:
    void txDiffXY(int x_diff, int y_diff);
};

class PathsWindow: public QWidget
{
    Q_OBJECT
    int current_width {PATHS_WINDOW_MIN_WIDTH};    // эти два изменяются при таскании за уголок //
    int current_height {PATHS_WINDOW_MIN_HEIGHT};  //                                           //
    int paths_table_width;
    int paths_table_height;
    QPointF prev_cursor_pos;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void changeEvent(QEvent *event);
    QLabel *central_widget;
    PathsTable *paths_table;
    CornerGrip *grip;
    OneStateButton *close_button;
    OneStateButton *minimize_button;
    OneStateButton *remove_all_button;
    QLabel *remove_all_label;
    QLabel *info_label;
    QPixmap *no_pixmap;
    QPixmap *yes_pixmap;
    QPixmap *no_hover_pixmap;
    QPixmap *yes_hover_pixmap;
    QPixmap *delete_pixmap;
    QPixmap *delete_hover_pixmap;
    int *paths_count {nullptr};
    void remove_all();
public:
    PathsWindow();
    ~PathsWindow();
public Q_SLOTS:
    void rxDiffXY(int x_diff, int y_diff);
    void rxAddFilenames(QStringList filenames);
    void rxAddDirname(QString dirname);
    friend MainWindow;
};

#endif // PW_H
