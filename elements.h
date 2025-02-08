// COMMON GUI ELEMENTS
#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "formats.h"
#include <QLabel>
#include <QPushButton>

// OneStateButton Without Text
class OneStateButton: public QLabel
{
    Q_OBJECT
    QPixmap main_pixmap;
    QPixmap hover_pixmap;
protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
public:
    OneStateButton(QWidget *parent, const QString &main_resource, const QString &hover_resource);
Q_SIGNALS:
    void imReleased();
};

// OneStateButton With Text and Dynamic Number Field
class DynamicInfoButton: public OneStateButton
{
    Q_OBJECT
    const QString *my_text; // указатель на массив (т.к. multilang)
    int *my_num_ptr;
    u32i prev_number;
    QLabel text_label {this};
    QFont *main_font;
    QFont *hover_font;
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEnterEvent *event) final;
    void leaveEvent(QEvent *event) final;
public:
    DynamicInfoButton(QWidget *parent, const QString &main_resource, const QString &hover_resource, const QString *text, int main_font_size, int hover_font_size, int *number_ptr);
    ~DynamicInfoButton();
public Q_SLOTS:
    void updateText(bool forced = false);
Q_SIGNALS:
    void imReleased();
};

// Two States Button Without Text
class TwoStatesButton: public QLabel
{
    Q_OBJECT
    QPixmap main_pixmap[2];
    QPixmap hover_pixmap[2];
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    bool *state_flag;
public:
    TwoStatesButton(QWidget *parent, bool *flag_ptr,    const QString &false_state_resource,
                                                        const QString &true_state_resource,
                                                        const QString &false_state_hover_resource,
                                                        const QString &true_state_hover_resource);
};

// Модальное информационное окно
class ModalInfoWindow: public QWidget
{
    Q_OBJECT
    QPointF prev_cursor_pos;
    void mouseMoveEvent(QMouseEvent *event);  // событие будет возникать и спускаться из Qlabel background
    void mousePressEvent(QMouseEvent *event); // событие будет возникать и спускаться из Qlabel background
public:
    enum class Type {JustInfo, Warning, Error};
    ModalInfoWindow(QWidget *parent, const QString &title_text, const QString &info_text, Type type);
    ~ModalInfoWindow();
};

#endif // ELEMENTS_H
