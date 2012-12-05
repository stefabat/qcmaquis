namespace ambient { namespace controllers { namespace velvet {

    template<typename T>
    inline c_revision::operator T* (){
        revision& c = *(revision*)this;
        if(!c.valid()) ambient::controller.calloc(c);
        return (T*)c;
    }
    template<typename T>
    inline s_revision::operator T* (){
        revision& c = *(revision*)this;
        revision& p = *c.parent;
        assert(!c.valid());
        if(!p.valid()){
            ambient::controller.calloc(c);
        }else if(p.locked()){
            ambient::controller.alloc(c);
            memcpy((T*)c, (T*)p, p.spec->size);
        }else{
            c.reuse(p);
        }
        return (T*)c;
    }
    template<typename T>
    inline w_revision::operator T* (){
        revision& c = *(revision*)this;
        assert(!c.valid());
        revision& p = *c.parent;
        if(!p.valid() || p.locked()) ambient::controller.alloc(c);
        else c.reuse(p);
        return (T*)c;
    }
    template<typename T>
    inline p_revision::operator T* (){
        memset((T*)*(w_revision*)this, 0, ((revision*)this)->spec->size); 
        return (T*)*(revision*)this;
    }
    
} } }
