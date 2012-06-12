#include "ambient/utils/ceil.h"
//#define LAYOUT_ACCESS_CHECK

namespace ambient { namespace models { namespace velvet {

    // {{{ layout model

    inline layout::~layout(){
        for(int x = 0; x < this->entries.size(); x++){
            for(int y = 0; y < this->entries[x].size(); y++){
                delete this->entries[x][y];
            }
        }
    }

    inline layout::layout(size_t t_size, dim2 b_size, dim2 size)
    : spec(t_size*b_size.x*b_size.y), mem_dim(b_size), dim(size), placement(NULL), grid_dim(0,0), master(0), mesh_dim(0,0)
    {
        this->set_dim(size);
    }

    inline const memspec& layout::get_spec() const {
        return this->spec;
    }

    inline void layout::embed(void* memory, size_t x, size_t y, size_t bound){
        this->get(x,y)->set_memory(memory, bound);
    }

    inline layout::entry* layout::get(size_t x, size_t y){
#ifdef LAYOUT_ACCESS_CHECK
        if(y >= this->get_grid_dim().y || x >= this->get_grid_dim().x)
        printf("%ld: Trying to access %ld x %ld of %ld x %ld\n", this->sid, x, y, this->get_grid_dim().x, this->get_grid_dim().y);
#endif
        return this->entries[x][y];
    }

    inline void layout::mesh(){
        this->grid_dim = dim2(__a_ceil(this->dim.x / this->mem_dim.x), 
                              __a_ceil(this->dim.y / this->mem_dim.y));
        dim2 dim = this->grid_dim;
        if(this->mesh_dim.x >= dim.x && this->mesh_dim.y >= dim.y) return;

        for(size_t x = this->mesh_dim.x; x < dim.x; x++)
            entries.push_back(std::vector<entry*>());
        
        for(size_t x = 0; x < this->mesh_dim.x; x++)
            for(size_t y = this->mesh_dim.y; y < dim.y; y++)
                entries[x].push_back(new layout::entry());

        for(size_t x = this->mesh_dim.x; x < dim.x; x++)
            for(size_t y = 0; y < dim.y; y++)
                entries[x].push_back(new layout::entry());

        if(dim.x > this->mesh_dim.x) this->mesh_dim.x = dim.x;
        if(dim.y > this->mesh_dim.y) this->mesh_dim.y = dim.y;
    }

    inline size_t layout::id(){
        return this->sid;
    }

    inline size_t layout::get_master(){
        return this->master;
    }

    inline void layout::set_dim(dim2 dim){
        this->dim = dim;
        this->mesh();
    }

    inline dim2 layout::get_dim() const {
        return this->dim;
    }

    inline dim2 layout::get_mem_dim() const { 
        return this->mem_dim;
    }

    inline dim2 layout::get_grid_dim() const {
        return this->grid_dim;
    }

    // }}}

    // {{{ layout::entry + layout::marker //

    inline bool layout::entry::trylock(){
        if(this->locked == true) return false;
        this->locked = true;
        return true;
    }

    inline void layout::entry::unlock(){
        this->locked = false;
    }

    inline layout::entry::~entry(){
        free(this->header);
    }

    inline layout::entry::entry()
    : header(NULL), request(false), locked(false)
    {
    }

    inline void layout::entry::set_memory(void* memory, size_t bound){
        this->header = memory;
        this->data = (void*)((size_t)memory + bound);
    }

    inline void* layout::entry::get_memory(){
        return this->header;
    }

    inline bool layout::entry::valid(){
        if(this->header != NULL) return true;
        return false;
    }

    inline bool layout::entry::requested(){
        return this->request;
    }

    inline std::list<controllers::velvet::cfunctor*>& layout::entry::get_assignments(){
        return this->assignments;
    }

    inline std::list<size_t>& layout::entry::get_path(){
        return this->path;
    }

    inline layout::marker::marker(){
        this->active = true;
        this->xmarker = 0;
        this->ymarker = 0;
    }

    inline bool layout::marker::marked(size_t x, size_t y){
        if(!this->active) return false;
        if(x >= this->xmarker || y >= this->ymarker) return true;
        return false;
    }

    inline void layout::marker::mark(size_t x, size_t y){
        this->active = true;
        this->xmarker = x;
        this->ymarker = y;
    }

    inline void layout::marker::clear(){
        this->active = false;
    }

    // }}}

} } }
