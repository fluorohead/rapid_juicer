// COMMON GUI ELEMENTS
#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "formats.h"
#include <QLabel>
#include <QPushButton>

// OneStateButton Without Text
class OneStateButton: public QLabel {
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
signals:
    void imReleased();
};

// OneStateButton With Text and Dynamic Number Field
class DynamicInfoButton: public OneStateButton {
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
public slots:
    void updateText(bool forced = false);
signals:
    void imReleased();
};

// Two States Button Without Text
class TwoStatesButton: public QLabel {
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

#endif // ELEMENTS_H
