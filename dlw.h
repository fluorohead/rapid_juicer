#ifndef DLW_H
#define DLW_H

#include "elements.h"
#include <QLabel>
#include <QTableWidget>
#include <QCommonStyle>

#define DIRLIST_MIN_WIDTH 1043
#define DIRLIST_MIN_HEIGHT 361
#define DIRTABLE_X 28
#define DIRTABLE_Y 70
#define DIRTABLE_ROW_H 26
#define DIRTABLE_MAX_PATHS 100
#define DIRTABLE_COL0_W 32
#define DIRTABLE_COL1_W 59

class YesNoMicroButton: public QLabel {
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
public slots:
    void txUpdateMyRowIndex(u32i removed_row_index);
};

class DeleteMicroButton: public QLabel {
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
signals:
    void imReleased(u32i row_index);
public slots:
    void txUpdateMyRowIndex(u32i removed_row_index);
};

class DirlistTable: public QTableWidget {
    Q_OBJECT
    QCommonStyle *my_style;
    void fillHeaderZeroRow();
public:
    DirlistTable(QWidget *parent = nullptr);
    ~DirlistTable();
signals:
    void txUpdateYourRowIndexes(u32i removed_row_index); // посылает всем DeleteMicroButton и YesNoMicroButton
    void txUpdatePathsButton(bool forced = false); // посылает кнопке типа DynamicInfoButton в виджете CentralWidget
public slots:
    void rxRemoveRow(u32i row_index);
    void rxRemoveAll();
};

class CornerGrip: public QLabel {
    Q_OBJECT
    QPointF prev_cursor_pos;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
public:
    CornerGrip(QWidget *parent);
signals:
    void txDiffXY(int x_diff, int y_diff);
};

class DirlistWindow: public QWidget {
    Q_OBJECT
    int header_height;
    int body_height;   // эти два изменяются при таскании за уголок //
    int hfiller_width; //                                           //
    int footer_height;
    int header_left_width;
    int header_right_width;
    int footer_left_width;
    int footer_right_width;
    int body_vfiller_left_width;
    int body_vfiller_right_width;
    int dirtable_width;
    int dirtable_height;
    int tclWidthHalf;
    QLabel header_left {this};
    QLabel header_hfiller {this}; // horizontal header filler
    QLabel header_right {this};
    QLabel body_vfiller_left {this};
    QLabel body_vfiller_right {this};
    QLabel footer_left {this};
    QLabel footer_hfiller {this};
    QLabel footer_right {this};
    QLabel infoText {this};
    QLabel trashcanLbl {this};
    QLabel trashcanText {&trashcanLbl};
    OneStateButton cleanAll {&trashcanLbl, ":/gui/dirlist/tc_btn.png", ":/gui/dirlist/tc_btn_h.png"};
    OneStateButton close_button {this, ":/gui/dirlist/dlw_close.png", ":/gui/dirlist/dlw_close_h.png"};
    OneStateButton minimize_button {this, ":/gui/dirlist/dlw_min.png", ":/gui/dirlist/dlw_min_h.png"};
    CornerGrip grip {&footer_right};
    int current_width {DIRLIST_MIN_WIDTH};    // эти два изменяются при таскании за уголок //
    int current_height {DIRLIST_MIN_HEIGHT};  //                                           //
    int *paths_count {nullptr};
    QPixmap no_pixmap {":/gui/dirlist/rcrs_no.png"}, yes_pixmap {":/gui/dirlist/rcrs_yes.png"};
    QPixmap no_hover_pixmap {":/gui/dirlist/rcrs_no_h.png"}, yes_hover_pixmap {":/gui/dirlist/rcrs_yes_h.png"};
    QPixmap delete_pixmap {":/gui/dirlist/dt_del.png"}, delete_hover_pixmap {":/gui/dirlist/dt_del_h.png"};
    QPointF prev_cursor_pos;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void changeEvent(QEvent *event);
public:
    DirlistTable dirtable {this};
    DirlistWindow(QWidget *parent);
public slots:
    void rxAddFilenames(QStringList filenames);
    void rxAddDirname(QString dirname);
    void rxDiffXY(int x_diff, int y_diff);
};

#endif // DLW_H
