#ifndef TGA_CHECK_H
#define TGA_CHECK_H

#include <QtTypes>
#include <QSet>

enum class TgaErr: qsizetype { InvalidHeader = 0, ValidHeader = 1, TruncDataAbort = 2, TooMuchPixAbort = 3, Success = 4 };

#pragma pack(push,1)
struct TgaHeader
{
    quint8  id_len;
    quint8  cmap_type;
    quint8  img_type;
    quint16 cmap_start;
    quint16 cmap_len;
    quint8  cmap_depth;
    quint16 x_offset;
    quint16 y_offset;
    quint16 width;
    quint16 height;
    quint8  pix_depth;
    quint8  img_descr;
};
struct TgaInfo
{
    int width;
    int height;
    int pixel_depth; // в битах : 8, 16, 24, 32
    qsizetype bytes_per_line; // размер сканлинии в байтах
    qint64 total_size; // размер массива декодированных данных
    qint8 type;
    QString id_string;
};
#pragma pack(pop)
class TgaDecoder
{
private:
    static const QSet<quint8> valid_img_types;
    static const QSet<quint8> valid_cmap_depths;
    static const QSet<qint8> valid_pix_depths;
    quint8 *src_array;
    size_t src_size;
    TgaHeader *header;
    qint64 pix_data_offset;
    qint64 total_size_p; // полный ожидаемый размер раскодированных данных в пикселях
    qint64 total_size_b; // полный ожидаемый размер раскодированных данных в байтах
    quint16 width;
    quint16 height;
    qsizetype bytes_per_line;
    quint8 one_pix_depth; // размер пикселя в битах
    quint8 one_pix_size; // размер пикселя в байтах
    quint8 cmap_elem_depth; // размер элемента палитры в битах
    quint8 cmap_elem_size; // размер элемента палитры в байтах
    quint16 cmap_len; // количество элементов в палитре (от 1 до 256)
    qint8 image_type;
    qint64 cmap_offset;
    QString id_string;
private:
    TgaErr decode_tc_15();
    TgaErr decode_tc_16();
    TgaErr decode_tc_24();
    TgaErr decode_tc_32();
    TgaErr decode_tc_rle15();
    TgaErr decode_tc_rle16();
    TgaErr decode_tc_rle24();
    TgaErr decode_tc_rle32();
public:
    TgaDecoder();
    TgaDecoder(const TgaDecoder&) = delete;
    TgaDecoder& operator=(const TgaDecoder&) = delete;
    ~TgaDecoder();

    void init(uchar *object_ptr, size_t object_size);
    TgaErr validate_header(int max_width = 8192, int max_height = 16384);
    TgaErr decode_checkup();
    TgaInfo info();
};
#endif // TGA_CHECK_H
