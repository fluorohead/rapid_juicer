#include "tga_check.h"
#include <cstring>
#include <QtDebug>
#include <QFile>

const QSet<quint8> TgaDecoder::valid_img_types = { 1, 2, 3, 9, 10, 11 };
const QSet<quint8> TgaDecoder::valid_cmap_depths = { 15, 16, 24, 32 };
const QSet<qint8> TgaDecoder::valid_pix_depths = { 8, 15, 16, 24, 32 };

TgaDecoder::TgaDecoder()
{
}

TgaDecoder::~TgaDecoder()
{
}

void TgaDecoder::init(uchar *object_ptr, size_t object_size)
{
    src_array = (quint8*)object_ptr;
    src_size = object_size;
    header = (TgaHeader*)object_ptr;
    pix_data_offset = -1;

    total_size_p = -1;
    total_size_b = -1;
    one_pix_depth = 0;
    one_pix_size = 0;
    width = 0;
    height = 0;
    bytes_per_line = 0;
    image_type = -1;
    cmap_elem_depth = 0;
    cmap_elem_size = 0;
    cmap_len = 0;
    id_string.clear();
}

// может возвращать ошибки : ValidHeader, InvalidHeader, NotInitialized
TgaErr TgaDecoder::validate_header(int max_width, int max_height)
{
    bool is_valid = true;
    if ( src_size < sizeof(TgaHeader) ) // в исходном объекте не хватает места на заголовок
    {
        is_valid = false;
    }
    else
    {
        if ( header->cmap_type > 1 ) is_valid = false; // неизвестный тип цветовой таблицы
        if ( ( header->cmap_type == 1 ) and ( !valid_cmap_depths.contains(header->cmap_depth) ) ) is_valid = false; // есть таблица? проверяем битность её элементов
        if ( !valid_img_types.contains(header->img_type) ) is_valid = false;
        if ( !valid_pix_depths.contains(header->pix_depth) ) is_valid = false;
        if ( ( header->width == 0 ) or ( header->height == 0 ) ) is_valid = false;
    }
    if ( is_valid )
    {
        if ( ( header->img_type == 2 ) or ( header->img_type == 10 ) )
        {
            quint8 alpha = header->img_descr & 0b00001111;
            if ( ( header->pix_depth == 15 ) and ( alpha > 0 ) ) is_valid = false;
            if ( ( header->pix_depth == 16 ) and ( alpha > 1 ) ) is_valid = false;
            if ( ( header->pix_depth == 24 ) and ( alpha > 0 ) ) is_valid = false;
        }
        if ( ( header->img_type == 3 ) or ( header->img_type == 11 ) )
        {
            if ( header->pix_depth != 8 ) is_valid = false;
        }
        if ( header->img_type == 9 )
        {
            if ( header->cmap_type != 1 ) is_valid = false;
            if ( header->pix_depth != 8 ) is_valid = false;
            if ( ( header->cmap_depth != 24 ) and ( header->cmap_depth != 32 ) ) is_valid = false;
            if ( header->cmap_len > 256 ) is_valid = false;
        }
    }
    if ( is_valid )
    {
        cmap_offset = sizeof(TgaHeader) + header->id_len;
        pix_data_offset = cmap_offset + header->cmap_type * (header->cmap_len * (header->cmap_depth / 8));
        if ( src_size < pix_data_offset )
        {
            pix_data_offset = -1;
            cmap_offset = -1;
            is_valid = false;
        }
    }
    if ( ( header->width > max_width ) or ( header->height > max_height ) ) is_valid = false;
    if ( is_valid )
    {
        one_pix_depth = header->pix_depth;
        one_pix_size = one_pix_depth / 8;
        width = header->width;
        height = header->height;
        bytes_per_line = width * 4; // раскодирование всегда в формат 0xAARRGGBB (little-endian)
        total_size_p = width * height;
        total_size_b = total_size_p * 4; // изображение любого типа всегда раскодируется в формат 0xAARRGGBB; в памяти (и файле) лежит так : BB GG RR AA
        image_type = header->img_type;
        cmap_elem_depth = header->cmap_depth;
        cmap_elem_size = cmap_elem_depth / 8;
        cmap_len = header->cmap_len;
        id_string.clear();
        for(quint8 id_idx; id_idx < header->id_len; ++id_idx)
        {
            auto ch = src_array[sizeof(TgaHeader) + id_idx];
            if ( !ch ) break;
            id_string += QChar(ch);
        }

    }
    return is_valid ? TgaErr::ValidHeader : TgaErr::InvalidHeader;
}

// может возвращать ошибки : TruncDataAbort, TooMuchPixAbort, Success, MemAllocErr, NeedHeaderValidation
TgaErr TgaDecoder::decode_checkup()
{
    switch(image_type)
    {
    case 2: // non-rle truecolor
    {
        switch(one_pix_depth)
        {
        case 15:
            return decode_tc_15();
        case 16:
            return decode_tc_16();
        case 24:
            return decode_tc_24();
        case 32:
            return decode_tc_32();
        }
    }
    case 10: // rle truecolor
    {
        switch(one_pix_depth)
        {
        case 15:
            return decode_tc_rle15();
        case 16:
            return decode_tc_rle16();
        case 24:
            return decode_tc_rle24();
        case 32:
            return decode_tc_rle32();
        }
    }
    default:
    {
        return TgaErr::InvalidHeader;
    }
    }
}

TgaErr TgaDecoder::decode_tc_15()
{
    qint64 need_src_size = (width * height) << 1; // требуемое количество исходных байт : (w*h*2)
    qint64 remain_size = src_size - pix_data_offset; // фактическое количество исходных байт
    if ( remain_size < need_src_size )
    {
        return TgaErr::TruncDataAbort;
    }
    else
    {
        return TgaErr::Success;
    }
}

TgaErr TgaDecoder::decode_tc_16()
{
    qint64 need_src_size = (width * height) << 1; // требуемое количество исходных байт : (w*h*2)
    qint64 remain_size = src_size - pix_data_offset; // фактическое количество исходных байт
    if ( remain_size < need_src_size )
    {
        return TgaErr::TruncDataAbort;
    }
    else
    {
        return TgaErr::Success;
    }
}

TgaErr TgaDecoder::decode_tc_24()
{
    qint64 need_src_size = width * height * 3; // требуемое количество исходных байт
    qint64 remain_size = src_size - pix_data_offset;
    if ( remain_size < need_src_size )
    {
        return TgaErr::TruncDataAbort;
    }
    else
    {
        return TgaErr::Success;
    }
}

TgaErr TgaDecoder::decode_tc_32()
{
    qint64 remain_size = src_size - pix_data_offset;
    bool truncated = remain_size < total_size_b;
    if ( remain_size < total_size_b )
    {
        return TgaErr::TruncDataAbort;
    }
    else
    {
        return TgaErr::Success;
    }
}

TgaErr TgaDecoder::decode_tc_rle15()
{
    quint8 *rle_array = &src_array[pix_data_offset];
    qint64 rle_size = src_size - pix_data_offset; // rle array size (from pix_data_offset to the end of source file)
    qint64 src_idx = 0; // byte index in rle_array
    qint64 pix_cnt = 0; // decoded pixels counter
    qint64 group_cnt; // group counter for rle or non-rle pixels
    qint64 not_packed_bytes;
    do {
        /// хватает ли места для очередного счётчика группы ?
        if ( rle_size - src_idx < 1 )
            return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка байтов исходных данных

        group_cnt = (rle_array[src_idx] & 0b01111111) + 1; // счётчик группы всегда кодирует минимум 1 пиксель
        pix_cnt += group_cnt; // обновляем общий счётчик пикселей
        if ( pix_cnt > total_size_p )
            return TgaErr::TooMuchPixAbort; // досрочный выход : вылезли за пределы размеров изображения, некорректные rle-данные

        if ( (rle_array[src_idx] >> 7) == 1 ) // пакет rle-группы
        {
            ++src_idx; // перестановка на байты пикселя
            if ( rle_size - src_idx >= (16/8) ) // хватает ли места в исходном буфере на 2 байта пикселя?
            {
                /// мультипликация байтов пикселя
                /// ...
                ///
                src_idx += (16/8); // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }
        else // пакет не-rle группы
        {
            ++src_idx; // перестановка на байты пикселя
            not_packed_bytes = group_cnt << 1; // group_cnt*(16/8);
            if ( rle_size - src_idx >= not_packed_bytes ) // хватает ли места в исходном буфере на group_cnt*2 байтов пикселей?
            {
                /// копирование байтов пикселей
                /// ...
                ///
                src_idx += not_packed_bytes; // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }

    } while(pix_cnt < total_size_p); // декодировали пикселей столько, сколько должны => конец цикла

    return TgaErr::Success;
}

TgaErr TgaDecoder::decode_tc_rle16()
{
    quint8 *rle_array = &src_array[pix_data_offset];
    qint64 rle_size = src_size - pix_data_offset; // rle array size (from pix_data_offset to the end of source file)
    qint64 src_idx = 0; // byte index in rle_array
    qint64 pix_cnt = 0; // decoded pixels counter
    qint64 group_cnt; // group counter for rle or non-rle pixels
    qint64 not_packed_bytes;
    do {
        /// хватает ли места для очередного счётчика группы ?
        if ( rle_size - src_idx < 1 )
            return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка байтов исходных данных

        group_cnt = (rle_array[src_idx] & 0b01111111) + 1; // счётчик группы всегда кодирует минимум 1 пиксель
        pix_cnt += group_cnt; // обновляем общий счётчик пикселей
        if ( pix_cnt > total_size_p )
            return TgaErr::TooMuchPixAbort; // досрочный выход : вылезли за пределы размеров изображения, некорректные rle-данные

        if ( (rle_array[src_idx] >> 7) == 1 ) // пакет rle-группы
        {
            ++src_idx; // перестановка на байты пикселя
            if ( rle_size - src_idx >= (16/8) ) // хватает ли места в исходном буфере на 2 байта пикселя?
            {
                /// мультипликация байтов пикселя
                /// ...
                ///
                src_idx += 2; // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }
        else // пакет не-rle группы
        {
            ++src_idx; // перестановка на байты пикселя
            not_packed_bytes = group_cnt << 1; // group_cnt * 2
            if ( rle_size - src_idx >= not_packed_bytes ) // хватает ли места в исходном буфере на group_cnt*2 байтов пикселей?
            {
                /// копирование байтов пикселей
                /// ...
                ///
                src_idx += not_packed_bytes; // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }

    } while(pix_cnt < total_size_p); // декодировали пикселей столько, сколько должны => конец цикла

    return TgaErr::Success;
}

TgaErr TgaDecoder::decode_tc_rle24()
{
    quint8 *rle_array = &src_array[pix_data_offset];
    qint64 rle_size = src_size - pix_data_offset; // rle array size (from pix_data_offset to the end of source file)
    qint64 src_idx = 0; // byte index in rle_array
    qint64 pix_cnt = 0; // decoded pixels counter
    qint64 group_cnt; // group counter for rle or non-rle pixels
    qint64 not_packed_bytes;
    do {
        /// хватает ли места для очередного счётчика группы ?
        if ( rle_size - src_idx < 1 )
            return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка байтов исходных данных

        group_cnt = (rle_array[src_idx] & 0b01111111) + 1; // счётчик группы всегда кодирует минимум 1 пиксель
        pix_cnt += group_cnt; // обновляем общий счётчик пикселей
        if ( pix_cnt > total_size_p )
            return TgaErr::TooMuchPixAbort; // досрочный выход : вылезли за пределы размеров изображения, некорректные rle-данные

        if ( (rle_array[src_idx] >> 7) == 1 ) // пакет rle-группы
        {
            ++src_idx; // перестановка на байты пикселя
            if ( rle_size - src_idx >= 3 ) // хватает ли места в исходном буфере на 3 байта пикселя?
            {
                /// мультипликация байтов пикселя
                /// ...
                ///
                src_idx += (24/8); // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }
        else // пакет не-rle группы
        {
            ++src_idx; // перестановка на байты пикселя
            not_packed_bytes = group_cnt * 3;
            if ( rle_size - src_idx >= not_packed_bytes ) // хватает ли места в исходном буфере на group_cnt*3 байтов пикселей?
            {
                /// копирование байтов пикселей
                /// ...
                ///
                src_idx += not_packed_bytes; // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }

    } while(pix_cnt < total_size_p); // декодировали пикселей столько, сколько должны => конец цикла

    return TgaErr::Success;
}

TgaErr TgaDecoder::decode_tc_rle32()
{
    quint8 *rle_array = &src_array[pix_data_offset];
    qint64 rle_size = src_size - pix_data_offset; // rle array size (from pix_data_offset to the end of source file)
    qint64 src_idx = 0; // byte index in rle_array
    qint64 pix_cnt = 0; // decoded pixels counter
    qint64 group_cnt; // group counter for rle or non-rle pixels
    qint64 not_packed_bytes;
    do {
        /// хватает ли места для очередного счётчика группы ?
        if ( rle_size - src_idx < 1 )
            return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка байтов исходных данных

        group_cnt = (rle_array[src_idx] & 0b01111111) + 1; // счётчик группы всегда кодирует минимум 1 пиксель
        pix_cnt += group_cnt; // обновляем общий счётчик пикселей
        if ( pix_cnt > total_size_p )
            return TgaErr::TooMuchPixAbort; // досрочный выход : вылезли за пределы размеров изображения, некорректные rle-данные

        if ( (rle_array[src_idx] >> 7) == 1 ) // пакет rle-группы
        {
            ++src_idx; // перестановка на байты пикселя
            if ( rle_size - src_idx >= 4 ) // хватает ли места в исходном буфере на 4 байта пикселя?
            {
                /// мультипликация байтов пикселя
                /// ...
                ///
                src_idx += 4; // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }
        else // пакет не-rle группы
        {
            ++src_idx; // перестановка на байты пикселя
            not_packed_bytes = group_cnt << 2; // group_cnt * 4
            if ( rle_size - src_idx >= not_packed_bytes ) // хватает ли места в исходном буфере на group_cnt*4 байтов пикселей?
            {
                /// копирование байтов пикселей
                /// ...
                ///
                src_idx += not_packed_bytes; // перестановка на следующий счётчик группы
            }
            else // не хватает места на байты пикселя
            {
                return TgaErr::TruncDataAbort; // досрочный выход из цикла : нехватка исходных байтов данных
            }
        }

    } while(pix_cnt < total_size_p); // декодировали пикселей столько, сколько должны => конец цикла

    return TgaErr::Success;
}

TgaInfo TgaDecoder::info()
{
    return TgaInfo{
                    width,
                    height,
                    one_pix_depth,
                    bytes_per_line,
                    total_size_b,
                    image_type,
                    id_string,
    };
}
