#ifndef AMBIENT_MODELS_VELVET_MEMSPEC
#define AMBIENT_MODELS_VELVET_MEMSPEC

namespace ambient { namespace models { namespace velvet {

    struct memspec {
        inline void latch(size_t, dim2, dim2);
        inline void* alloc() const;
        inline void* calloc() const;
        inline size_t get_bound() const;
        inline void* get_packet_t() const;
        dim2 block;
        dim2 grid;
        dim2 dim;
        size_t size;
    };

} } }

#endif
