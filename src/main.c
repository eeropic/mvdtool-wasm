#include "main.h"
#include "option.h"

void fatal( const char *fmt, ... ) {
    va_list argptr;

    va_start( argptr, fmt );
    fprintf( stderr, "ERROR: " );
    vfprintf( stderr, fmt, argptr );
    fprintf( stderr, "\n" );
    va_end( argptr );

    exit( 1 );
}

void read_raw( void *buf, size_t len, FILE *fp ) {
    if( fread( buf, 1, len, fp ) != len ) {
        if( ferror( fp ) ) {
            fatal( "failed to read file: %s", strerror( errno ) );
        } else {
            fatal( "unexpected end of file" );
        }
    }
}

#if 0
void read_raw_safe( void *buf, size_t len, FILE *fp ) {
    if( fread( buf, 1, len, fp ) != len ) {
        if( ferror( fp ) ) {
            fprintf( stderr, "WARNING: error reading file: %s\n", strerror( errno ) );
        }
        return false;
    }
    return true;
}
#endif

void write_raw( void *buf, size_t len, FILE *fp ) {
    if( fwrite( buf, 1, len, fp ) != len ) {
        fatal( "failed to write file: %s", strerror( errno ) );
    }
}

// 12345
// 12345.1
// 12:45
// 12:45.1
bool parse_timespec( const char *s, unsigned *blocknum ) {
    unsigned c1, c2, c3;
    char *p;

    c1 = strtoul( s, &p, 10 );
    if( !*p ) {
        *blocknum = c1; // block number
        return true;
    }
    if( *p == '.' ) {
        c2 = strtoul( p + 1, &p, 10 );
        if( *p || c2 > 9 ) {
            return false;
        }
        *blocknum = c1 * 10 + c2; // sec.frac
        return true;
    }
    if( *p == ':' ) {
        c2 = strtoul( p + 1, &p, 10 );
        if( !*p ) {
            *blocknum = c1 * 600 + c2 * 10; // min:sec
            return true;
        }
        if( *p == '.' ) {
            c3 = strtoul( p + 1, &p, 10 );
            if( *p || c3 > 9 ) {
                return false;
            }
            *blocknum = c1 * 600 + c2 * 10 + c3; // min:sec.frac
            return true;
        }
        return false;
    }
    return false;
}

static int help_main( void ) {
    fprintf( stderr,
        "Usage: mvdtool <mode> [options]\n"
        "Version: " MVDTOOL_VERSION "\n"
        "\n"
        "Mode     Description\n"
        "-------- ----------------------------------------------\n"
        "convert  Convert MVDs between binary and text formats\n"
        "hash     Compute SHA-1 hashes of MVD binary blocks\n"
        "strings  Parse MVD and print messages and timestamps\n"
        "split    Split MVD by timestamps into multiple files\n"
        "cut      Seamlessy remove chosen parts of MVD recording\n"
        "index    Build index file from MVD file\n"
        "diff     Compare MVDs block-by-block\n"
        "help     Show this message\n" );
    return 0;
}

int main( int argc, char **argv ) {
    char *name;

    if( argc < 2 ) {
        goto usage;
    }

    name = argv[1];
    cmd_argc = argc - 1;
    cmd_argv = argv + 1;

    if( !strcmp( name, "convert" ) ) {
        return convert_main();
    }
    if( !strcmp( name, "hash" ) ) {
        return hash_main();
    }
    if( !strcmp( name, "strings" ) ) {
        return strings_main();
    }
    if( !strcmp( name, "split" ) ) {
        return split_main();
    }
    if( !strcmp( name, "cut" ) ) {
        return cut_main();
    }
    if( !strcmp( name, "index" ) ) {
        return index_main();
    }
    if( !strcmp( name, "diff" ) ) {
        return diff_main();
    }
    if( !strcmp( name, "help" ) ) {
        return help_main();
    }

usage:
    fprintf( stderr, "Usage: %s <convert|hash|strings|split|cut|index|diff|help> [options]\n", argv[0] );
    return 1;
}

