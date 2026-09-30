#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <new>
static unsigned allocations,live;
void *operator new[](std::size_t size)
{ ++allocations;++live;void *p=std::malloc(size);assert(p);return p; }
void operator delete[](void *p) noexcept
{ if(p) {assert(live);--live;std::free(p);} }
#include "../src/platform/amiga/framework/Palette24Bit.cpp"
int main()
{
    uint32_t source[256],current[256],copy[256];
    for(unsigned i=0;i<256;++i) source[i]=(i*7919U)&0xffffff;
    std::memcpy(copy,source,sizeof source);
    {
        Palette24Bit owned(source,256);
        unsigned before=allocations;
        {
            Palette24Bit borrowed(source,current,256);
            for(unsigned base=0;base<3;++base) {
                owned.setFadeBaseColor(base*0x654321);
                borrowed.setFadeBaseColor(base*0x654321);
                for(unsigned fade=0;fade<256;++fade) {
                    owned.setFade(fade);borrowed.setFade(fade);
                    assert(owned.update()==borrowed.update());
                    for(unsigned i=0;i<256;++i) assert(owned[i]==borrowed[i]);
                    assert(!borrowed.update());
                }
            }
            assert(allocations==before && !std::memcmp(source,copy,sizeof source));
        }
        assert(allocations==before && live==2);
        {
            Palette24Bit borrowed(source,current,256);
            borrowed.setColors(copy,32); // Explicitly switches to owned storage.
            assert(allocations==before+2 && live==4);
            borrowed.update();
            for(unsigned i=0;i<32;++i) assert(borrowed[i]==source[i]);
        }
        assert(live==2);
    }
    assert(!live);
    std::puts("Framework borrowed palette: all fades match owning path, zero borrowed allocations/frees, ownership transition passes");
}
