class DataStream
{
    LPSTR m_p;
    INT m_size;
public:
    DataStream()
    {
        m_p = (LPSTR)malloc(1);
        if (m_p == NULL) throw bad_alloc();
        m_size = 0;
        m_p[m_size] = '\0';
    }
    DataStream(LPVOID p, INT size)
    {
        m_size = size;
        m_p = (LPSTR)malloc(m_size + 1);
        if (m_p == NULL) throw bad_alloc();
        CopyMemory(m_p, p, m_size);
        m_p[m_size] = '\0';
    }
    DataStream(const DataStream& s) : m_p(NULL) { *this = s; }
    ~DataStream() { if (m_p != NULL) free(m_p); }
    LPSTR Ptr() { return m_p; }
    INT Size() const { return m_size; }
    DataStream& operator=(const DataStream &s)
    {
        m_p = (LPSTR)realloc(m_p, s.m_size + 1);
        if (m_p == NULL) throw bad_alloc();
        m_size = s.m_size;
        CopyMemory(m_p, s.m_p, m_size);
        m_p[m_size] = '\0';
        return *this;
    }
    VOID Append(LPCVOID p, INT size)
    {
        m_p = (LPSTR)realloc(m_p, m_size + size + 1);
        if (m_p == NULL) throw bad_alloc();
        CopyMemory(m_p + m_size, p, size);
        m_size += size;
        m_p[m_size] = '\0';
    }
    VOID AppendSz(LPCSTR psz)
    {
        Append(psz, lstrlenA(psz));
    }
    VOID AppendF(LPCSTR pszFormat, ...)
    {
        CHAR sz[1024];
        va_list va;
        va_start(va, pszFormat);
        wvsprintfA(sz, pszFormat, va);
        va_end(va);
        AppendSz(sz);
    }
    VOID Skip(INT size)
    {
        INT newsize = m_size - size;
        MoveMemory(m_p, m_p + size, newsize);
        m_p = (LPSTR)realloc(m_p, newsize + 1);
        if (m_p == NULL) throw bad_alloc();
        m_size = newsize;
        m_p[m_size] = '\0';
    }
    INT Find(LPCVOID p, INT size, INT start = 0) const;
    VOID Clear(VOID)
    {
        m_p = (LPSTR)realloc(m_p, 1);
        if (m_p == NULL) throw bad_alloc();
        m_size = 0;
        m_p[m_size] = '\0';
    }
};
