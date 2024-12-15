// MAIN WINDOW
#ifndef MW_H
#define MW_H

#include "formats.h"
#include "settings.h"
#include "elements.h"
#include "dlw.h"
#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QTableWidget>
#include <QCommonStyle>
#include <QWindow>
#include <QDialog>

#define FORMATS_TABLE_W 412
#define FORMATS_TABLE_H 432
#define FORMATS_TABLE_COL0_W 50
#define FORMATS_TABLE_COL1_W 240
#define FORMATS_TABLE_ROW_H 50

#define MAX_FILTERS 9

#define VERSION_TEXT "Rapid Juicer :: 0.0.1"

extern Settings settings;

enum class FilterAction { Include = 0, Exclude = 1, MAX };

class FilterButton: public QLabel {
    Q_OBJECT
    FilterAction my_action;
    u64i my_categories;
    QPixmap *my_main_pixmap;
    QPixmap *my_hover_pixmap;
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
public:
    FilterButton(QWidget *parent, FilterAction action, u64i categories, QPoint position, QPixmap *main_pixmap, QPixmap *hover_pixmap);
signals:
    void imReleased(FilterAction action, u64i categories);
};

class FormatLabel: public QLabel {
    Q_OBJECT
    void mousePressEvent(QMouseEvent *event);
    QLabel corner_label {this};
    QPixmap *corner_pixmap;
    QString my_format_key;
public:
    FormatLabel(const QString &format_key, const QString &text, QPixmap *corner_pixmap, QWidget *parent = nullptr);
public slots:
    void rxToggle();
};

class DescriptionLabel: public QLabel {
    Q_OBJECT
    void mousePressEvent(QMouseEvent *event);
public:
    DescriptionLabel(const QString &text, QWidget *parent = nullptr);
    DescriptionLabel(QWidget *parent = nullptr);
signals:
    void txToggle();
};

class CategoryLabel: public QLabel {
    Q_OBJECT
    void mousePressEvent(QMouseEvent *event);
public:
    CategoryLabel(QString id, const QMap <u64i, QPixmap*> &pixmaps, QWidget *parent = nullptr);
signals:
    void txToggle();
};

class FormatsTable: public QTableWidget {
    Q_OBJECT
    QPixmap corner_fmt_pixmap {":/gui/main/selector.png"};
    static const QMap <u64i, QString> categories_resources; // проинициализированно в mw.cpp
    QMap <u64i, QPixmap*> categories_pixmaps;
    void prepCategPixmaps();
    void delCategPixmaps();
public:
    FormatsTable(QWidget *parent);
public slots:
    void rxCommand(FilterAction action, u64i categories);
};

// class CentralWidget: public QLabel {
//     Q_OBJECT
//     OneStateButton play_button {this, ":/gui/main/play.png", ":/gui/main/play_h.png"};
//     OneStateButton add_file_button {this, ":/gui/main/afile.png", ":/gui/main/afile_h.png"};
//     OneStateButton add_dir_button {this, ":/gui/main/adir.png", ":/gui/main/adir_h.png"};
//     OneStateButton settings_button {this, ":/gui/main/setts.png", ":/gui/main/setts_h.png"};
//     OneStateButton minimize_button {this, ":/gui/main/min.png", ":gui/main//min_h.png"};
//     OneStateButton close_button {this, ":/gui/main/close.png", ":/gui/main/close_h.png"};
//     TwoStatesButton scrup_button {this, &settings.config.scrupulous, ":/gui/main/scrpm_off.png", ":/gui/main/scrpm_on.png", ":/gui/main/scrpm_off_h.png", ":/gui/main/scrpm_on_h.png"};
//     DynamicInfoButton *paths_button;
//     QLabel version_label {VERSION_TEXT, this};
//     QLabel tasks_label {this};
//     QLabel scrup_label {this};
//     FormatsTable formats_table {this};
//     void mouseMoveEvent(QMouseEvent *event);
//     void mousePressEvent(QMouseEvent *event);
//     void changeEvent(QEvent *event);
//     FilterButton* includeButtons[MAX_FILTERS];
//     FilterButton* excludeButtons[MAX_FILTERS];
//     QPixmap filter_main_pixmaps  [2] { {":/gui/main/incl.png"},   {":/gui/main/excl.png"}   }; // [0] - for include, [1] - for exclude
//     QPixmap filter_hover_pixmaps [2] { {":/gui/main/incl_h.png"}, {":/gui/main/excl_h.png"} }; // [0] - for include, [1] - for exclude
//     DirlistWindow *_dirlist;
//     QLabel *categ_labels[MAX_FILTERS];
// public:
//     CentralWidget(QWidget *parent, DirlistWindow *dirlist);
// public slots:
//     void addFiles();
//     void addDir();
//     void showSettings();
//     void showNewSessionWindow();
// signals:
//     void txFilenames(QStringList filenames);
//     void txDirname(QString dirname);
// };

class MainWindow: public QWidget {
    Q_OBJECT
    DirlistWindow dirlist {nullptr};
    QPixmap filter_main_pixmaps  [2];
    QPixmap filter_hover_pixmaps [2];
    QPointF prev_cursor_pos;
    DynamicInfoButton *paths_button;
    QLabel *tasks_label;
    QLabel *scrup_label;
    QLabel *categ_labels[MAX_FILTERS];
    FormatsTable *formats_table;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void changeEvent(QEvent *event);
    void closeEvent(QCloseEvent *event);
public:
    MainWindow();
    ~MainWindow();
public slots:
    void addFiles();
    void addDir();
    void showSettings();
    void showNewSessionWindow();
signals:
    void txFilenames(QStringList filenames);
    void txDirname(QString dirname);
};
#endif // MW_H
