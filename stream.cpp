#include <windows.h>

#include <string>
#include <cstdlib>
#include <cstdio>
using namespace std;

#include "stream.h"

INT DataStream::Find(LPCVOID p, INT size, INT start) const
{
    for(INT i = start; i <= m_size - size; i++)
    {
        if (memcmp(m_p + i, p, size) == 0)
            return i;
    }
    return -1;
}

#ifdef UNITTEST
#include <assert.h>

int main(void)
{
    DataStream ds;

    for(int i = 0; i < 4000; i++)
    {
        ds.AppendSz("ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n");
        ds.AppendSz("ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n");
        ds.AppendSz("ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n");
        ds.Skip(28);
        ds.Skip(28);
    }
    if (ds.Size() == 28 * 4000)
        printf("ok\n");
    puts(ds.Ptr());
    return 0;
}
#endif
