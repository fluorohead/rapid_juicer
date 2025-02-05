#include "elements.h"
#include "settings.h"
#include <QMouseEvent>
#include <QPushButton>

extern Settings *settings;

extern const QString ok_txt[int(Langs::MAX)];

OneStateButton::OneStateButton(QWidget *parent, const QString &main_resource, const QString &hover_resource)
    : QLabel(parent)
    , main_pixmap(main_resource)
    , hover_pixmap(hover_resource)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setFixedSize(main_pixmap.width(), main_pixmap.height());
    this->setPixmap(main_pixmap);
    this->setMask(main_pixmap.mask());
}

void OneStateButton::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        event->accept();
    }
}

void OneStateButton::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        Q_EMIT imReleased();
        event->accept();
    }
}

void OneStateButton::mouseMoveEvent(QMouseEvent *event) {
    event->accept();
}

void OneStateButton::enterEvent(QEnterEvent *event) {
    event->accept();
    this->setPixmap(hover_pixmap);
    //this->setMask(hover_pixmap.mask());
}

void OneStateButton::leaveEvent(QEvent *event) {
    event->accept();
    this->setPixmap(main_pixmap);
    //this->setMask(main_pixmap.mask());
}

DynamicInfoButton::DynamicInfoButton(QWidget *parent,   const QString &main_resource,
                                                        const QString &hover_resource,
                                                        const QString *text, // указатель на массив (т.к. multilang)
                                                        int main_font_size,
                                                        int hover_font_size,
                                                        int *number_ptr)
    : OneStateButton(parent, main_resource, hover_resource)
    , my_text(text)
    , my_num_ptr(number_ptr)
{
    prev_number = *my_num_ptr;

    main_font = new QFont(*skin_font());
    hover_font = new QFont(*skin_font());
    main_font->setPixelSize(main_font_size);
    hover_font->setPixelSize(hover_font_size);
    main_font->setItalic(true);
    hover_font->setItalic(true);

    text_label.move(0, 0);
    text_label.setFixedSize(this->size());
    text_label.setAlignment(Qt::AlignCenter);
    text_label.setStyleSheet("color: #fffef9");
    text_label.setFont(*main_font);
    updateText(true);
}

DynamicInfoButton::~DynamicInfoButton()
{
    delete main_font;
    delete hover_font;
}

void DynamicInfoButton::updateText(bool forced)
{
    if ((prev_number != *my_num_ptr) or forced) { // изменение текста только в случае изменения значения числа
        //text_label.setText(my_text[curr_lang()] + QString::number(*my_num_ptr));
        text_label.setText(my_text[curr_lang()].arg(QString::number(*my_num_ptr)));
        prev_number = *my_num_ptr;
    }
}

void DynamicInfoButton::mousePressEvent(QMouseEvent *event) {
    OneStateButton::mousePressEvent(event);
}

void DynamicInfoButton::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        Q_EMIT imReleased();
        event->accept();
    }
}

void DynamicInfoButton::mouseMoveEvent(QMouseEvent *event) {
    OneStateButton::mouseMoveEvent(event);
}

void DynamicInfoButton::enterEvent(QEnterEvent *event)
{
    OneStateButton::enterEvent(event); // вызов родительского метода
    text_label.setFont(*hover_font);
}

void DynamicInfoButton::leaveEvent(QEvent *event)
{
    OneStateButton::leaveEvent(event); // вызов родительского метода
    text_label.setFont(*main_font);
}

TwoStatesButton::TwoStatesButton(QWidget *parent, bool *flag_ptr,   const QString &false_state_resource,
                                                                    const QString &true_state_resource,
                                                                    const QString &false_state_hover_resource,
                                                                    const QString &true_state_hover_resource)
    : QLabel(parent)
    , state_flag(flag_ptr)
    , main_pixmap{false_state_resource, true_state_resource}
    , hover_pixmap{false_state_hover_resource, true_state_hover_resource}
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setFixedSize(main_pixmap[0].width(), main_pixmap[0].height());
    this->setPixmap(main_pixmap[*state_flag]);
    this->setMask(main_pixmap[0].mask());
}

void TwoStatesButton::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        event->accept();
    }
}

void TwoStatesButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        *state_flag = !(*state_flag);
        //this->setPixmap(hover_pixmap[*state_flag]);
        event->accept();
    }
}

void TwoStatesButton::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
}

void TwoStatesButton::enterEvent(QEnterEvent *event)
{
    this->setPixmap(hover_pixmap[*state_flag]);
    //this->setMask(hover_pixmap[*state_flag].mask());
    event->accept();
}

void TwoStatesButton::leaveEvent(QEvent *event)
{
    this->setPixmap(main_pixmap[*state_flag]);
    this->setMask(main_pixmap[*state_flag].mask());
    event->accept();
}

ModalInfoWindow::ModalInfoWindow(QWidget *parent, const QString &title_text, const QString &info_text, Type type)
    : QWidget(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowModality(Qt::WindowModal);

    this->setFixedSize(362, 160);

    auto background = new QLabel;
    background->setFixedSize(this->width(), this->height());
    background->setStyleSheet("background-color: #5c8685; border-width: 3px; border-style: solid; border-radius: 24px; border-color: #b6c7c7;");
    background->setParent(this);
    background->move(0, 0);

    QPixmap *icon_pixmap;
    switch(type)
    {
    case Type::JustInfo:
        icon_pixmap = new QPixmap();
        break;
    case Type::Warning:
        icon_pixmap = new QPixmap(":/gui/modal/warning.png");
        break;
    case Type::Error:
        icon_pixmap = new QPixmap();
        break;
    }

    auto icon = new QLabel;
    icon->setFixedSize(icon_pixmap->width(), icon_pixmap->height());
    icon->setPixmap(*icon_pixmap);
    icon->setParent(this);
    icon->move(16, 40);

    delete icon_pixmap;

    QFont tmpFont {*skin_font()};
    tmpFont.setBold(true);
    tmpFont.setPixelSize(14);
    auto title = new QLabel;
    title->setFixedSize(background->width(), 32);
    title->setStyleSheet("color: #e3b672");
    title->setFont(tmpFont);
    title->setAlignment(Qt::AlignCenter);
    title->setText(title_text);
    title->setParent(this);
    title->move(0, 4);

    auto info = new QLabel;
    info->setFixedSize(216, 64);
    info->setStyleSheet("color: #fffef9");
    info->setFont(tmpFont);
    info->setAlignment(Qt::AlignCenter);
    info->setText(info_text);
    info->setParent(this);
    info->move(96, 44);

    tmpFont.setPixelSize(12);
    auto ok_button = new QPushButton(this);
    ok_button->setAttribute(Qt::WA_NoMousePropagation);
    ok_button->setFixedSize(72, 28);
    ok_button->setStyleSheet(   "QPushButton:enabled {color: #fffef9; background-color: #b38642; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:hover   {color: #fffef9; background-color: #e3b672; border-width: 2px; border-style: solid; border-radius: 14px; border-color: #b6c7c7;}"
                                "QPushButton:pressed {color: #fffef9; background-color: #e3b672; border-width: 0px;}");
    ok_button->setFont(tmpFont);
    ok_button->setText(ok_txt[curr_lang()]);
    ok_button->move(background->width() - 84, background->height() - 38);

    connect(ok_button, &QPushButton::clicked, [this](){ this->close(); }); // после вызова close() модальное окно уничтожается автоматически
}

ModalInfoWindow::~ModalInfoWindow()
{

}

void ModalInfoWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        this->move(this->pos() + (event->globalPosition() - prev_cursor_pos).toPoint());
        prev_cursor_pos = event->globalPosition();
    }
}

void ModalInfoWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        prev_cursor_pos = event->globalPosition();
    }
}
