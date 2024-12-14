#ifndef SETTINGS_H
#define SETTINGS_H

#include "formats.h"
#include <QFont>
#include <QFile>
#include <QValidator>
#include <QLabel>

enum class Langs: int {Eng = 0, Rus = 1, MAX};

#define curr_lang() settings.config.lang_idx
#define skin_font() settings.skin.main_font

#define MAX_SESSIONS 3


struct Config
{
    bool        scrupulous;
    int         lang_idx;
    int         bfr_size_idx;
    bool        recursion;
    QString     file_mask;
    QStringList excluding;
};


struct Skin
{
    QString font_name;
    QFont   *main_font;
};


extern const Config default_config;


class Settings
{
    int     threads_num; // 2 by default
    QFile   file;
    QString file_path;

public:
    Settings();
    ~Settings();

    Config config;
    Config cand_config;
    Skin   skin;

    QSet<QString> selected_formats;

    void initSkin();

    u64i getBufferSizeInBytes();
    u64i getBufferSizeByIndex(int idx);

    u32i getThreadsNum();
};


QValidator::State mask_validator(const QString &input);
QValidator::State excluding_validator(const QString &input);


class MaskValidator: public QValidator {
public:
    MaskValidator(QObject *parent);
    QValidator::State validate(QString &input, int &pos) const;
};


class ExcludingValidator: public QValidator {
public:
    ExcludingValidator(QObject *parent);
    QValidator::State validate(QString &input, int &pos) const;
};


class SettingsWindow: public QWidget {
    Q_OBJECT
    int previous_lang_idx;
    QLabel background {this};
    QPointF prev_cursor_pos;
    MaskValidator fmv {this}; // file mask validator
    ExcludingValidator eev {this}; // excluding extensions validator
    void mouseMoveEvent(QMouseEvent *event);  // событие будет возникать и спускаться из Qlabel background
    void mousePressEvent(QMouseEvent *event); // событие будет возникать и спускаться из Qlabel background
public:
    SettingsWindow(QWidget *parent);
    ~SettingsWindow();
};

#endif // SETTINGS_H
