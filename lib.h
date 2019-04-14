
// fast "C" macros
#define Q_isupper(c)    ((c) >= 'A' && (c) <= 'Z')
#define Q_islower(c)    ((c) >= 'a' && (c) <= 'z')
#define Q_isdigit(c)    ((c) >= '0' && (c) <= '9')
#define Q_isalpha(c)    (Q_isupper(c) || Q_islower(c))
#define Q_isalnum(c)    (Q_isalpha(c) || Q_isdigit(c))
#define Q_isprint(c)    ((c) >= 32 && (c) < 127)
#define Q_isgraph(c)    ((c) > 32 && (c) < 127)
#define Q_isspace(c)    (c == ' ' || c == '\f' || c == '\n' || \
                         c == '\r' || c == '\t' || c == '\v')

// tests if specified character is valid quake path character
#define Q_ispath(c)     (Q_isalnum(c) || (c) == '_' || (c) == '-')

// tests if specified character has special meaning to quake console
#define Q_isspecial(c)  ((c) == '\r' || (c) == '\n' || (c) == 127)

static inline int Q_charhex(int c)
{
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    return -1;
}

// converts quake char to ASCII equivalent
static inline int Q_charascii(int c)
{
    c &= 127; // strip high bits
    if (Q_isgraph(c) || Q_isspace(c)) {
        return c;
    }
    switch (c) {
    // handle bold brackets
    case 16:
        return '[';
    case 17:
        return ']';
    }
    return '.'; // don't output control chars, etc
}


#define CMP1(a,b) (a!=b)
#define CMP2(a,b) (a[0]!=b[0]||a[1]!=b[1])
#define CMP3(a,b) (a[0]!=b[0]||a[1]!=b[1]||a[2]!=b[2])
#define CMP4(a,b) (a[0]!=b[0]||a[1]!=b[1]||a[2]!=b[2]||a[3]!=b[3])

#define CPY1(a,b) (a=b)
#define CPY2(a,b) (a[0]=b[0],a[1]=b[1])
#define CPY3(a,b) (a[0]=b[0],a[1]=b[1],a[2]=b[2])
#define CPY4(a,b) (a[0]=b[0],a[1]=b[1],a[2]=b[2],a[3]=b[3])
