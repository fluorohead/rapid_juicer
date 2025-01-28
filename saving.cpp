#include "saving.h"
#include "formats.h"
#include <QTextEdit>
#include <QApplication>

SavingWindow::SavingWindow(const QString &shm_key, const QString &shm_size, const QString &ssem_key)
    : QWidget(nullptr)
{
    this->setFixedSize(800, 600);
    auto info_text = new QTextEdit(this);
    info_text->setFixedSize(800 - 16, 600 - 16);
    info_text->move(8, 8);
    info_text->append("My process Id: " + QString::number(QApplication::applicationPid()));
    info_text->append("Shared memory key: " + shm_key);
    info_text->append("Shared memory size: " + shm_size);
    info_text->append("System semaphore key: " + ssem_key);

    sys_semaphore = new QSystemSemaphore(ssem_key, 1, QSystemSemaphore::Open);
    if ( sys_semaphore->error() != QSystemSemaphore::NoError )
    {
        info_text->append("System semaphore opening error? -> : " + sys_semaphore->errorString());
    }

    shared_memory.setKey(shm_key);
    shared_memory.attach(QSharedMemory::ReadOnly);
    sys_semaphore->release();
    if ( shared_memory.error() != QSharedMemory::NoError )
    {
        info_text->append("Shared memory attach error? -> : " + shared_memory.errorString());
    }
    info_text->append("Shared memory real size: " + QString::number(shared_memory.size()));

    u8i *shm_buffer = (u8i*)shared_memory.data();
    TLV_Header *tlv_header;
    QString current_format;
    QString current_source;
    POD_ResourceRecord *current_pod_ptr;
    ResourceRecord current_rr;
    QString current_dest_ext;
    QString current_info;
    u64i next_header_offset = 0; // =0 - важно!

    do
    {
        tlv_header = (TLV_Header*)&shm_buffer[next_header_offset];
        info_text->append("type:" + QString::number(int(tlv_header->type)) + " length:" + QString::number(int(tlv_header->length)));
        switch(tlv_header->type)
        {
        case TLV_Type::FmtChange:
        {
            current_format = QString::fromLatin1((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            info_text->append("-> FmtChange: '" + current_format + "'");
            if ( !resources_db.contains(current_format) ) // формат встретился первый раз?
            {
                formats_counters[current_format] = 0;
            }
            ++formats_counters[current_format];
            break;
        }
        case TLV_Type::SrcChange:
        {
            current_source = QString::fromUtf8((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            info_text->append("-> SrcChange: '" + current_source + "'");
            break;
        }
        case TLV_Type::POD:
        {
            current_pod_ptr = (POD_ResourceRecord*)((char*)tlv_header + sizeof(TLV_Header));
            current_rr.order_number = current_pod_ptr->order_number;
            current_rr.offset = current_pod_ptr->offset;
            current_rr.size = current_pod_ptr->size;
            info_text->append("-> POD : order_number: " + QString::number(current_rr.order_number) + "; offset: " + QString::number(current_rr.offset) + "; size: " + QString::number(current_rr.size));
            break;
        }
        case TLV_Type::DstExtension:
        {
            current_dest_ext = QString::fromLatin1((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            info_text->append("-> DstExtension: '" + current_dest_ext + "'");
            current_rr.dest_extension = current_dest_ext;
            break;
        }
        case TLV_Type::Info:
        {
            current_info = QString::fromUtf8((char*)tlv_header + sizeof(TLV_Header), tlv_header->length);
            info_text->append("-> Info: '" + current_info + "'");
            current_rr.info = current_info;
            // TLV "Info" всегда идёт завершающей, значит можно сделать запись в БД
            //resources_db[current_format][current_source][current_rr.order_number] = current_rr;
            resources_db[current_format][current_source].append(current_rr);
            break;
        }
        case TLV_Type::Terminator:
        {
            info_text->append("-> Terminator:");
            break;
        }
        }
        next_header_offset += (sizeof(TLV_Header) + tlv_header->length);
    } while(tlv_header->type != TLV_Type::Terminator);

    delete sys_semaphore;
}
