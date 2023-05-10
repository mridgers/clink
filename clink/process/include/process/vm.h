// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class Vm
{
public:
    enum
    {
        access_read     = 1 << 0,
        access_write    = 1 << 1,
        access_execute  = 1 << 2,
        access_cow      = 1 << 3, // copy-on-write
        access_rw       = access_read|access_write,
        access_rx       = access_read|access_execute,
        access_rwx      = access_read|access_execute|access_write,
    };

    struct Region
    {
        void*           base;
        unsigned int    page_count;
    };

                        Vm(int pid=-1);
                        ~Vm();
    static size_t       get_block_granularity();
    static size_t       get_page_size();
    void*               get_alloc_base(void* address);
    Region              get_region(void* address);
    void*               get_page(void* address);
    Region              alloc(unsigned int page_count, unsigned int access=access_read|access_write);
    void                free(const Region& Region);
    int                 get_access(const Region& Region);
    void                set_access(const Region& Region, unsigned int access);
    bool                read(void* dest, const void* src, size_t size);
    bool                write(void* dest, const void* src, size_t size);
    void                flush_icache(const Region& Region={});

private:
    void*               _handle;
};
