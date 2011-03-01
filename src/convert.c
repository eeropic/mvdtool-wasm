#include "main.h"
#include "proto.h"
#include "node.h"
#include "bin.h"
#include "txt.h"
#include "swap.h"
#include "option.h"

#ifdef _DEBUG
extern size_t trap_pos;
#endif

int convert_main( void ) {
    uint32_t magic;
    node_t *n;
    unsigned blocknum;

    FILE *ifp = stdin;
    FILE *ofp = stdout;

    if( cmd_argc > 1 ) {
        ifp = fopen( cmd_argv[1], "rb" );
        if( !ifp ) {
            perror( "fopen" );
            return 1;
        }
        if( cmd_argc > 2 ) {
            ofp = fopen( cmd_argv[2], "w" );
            if( !ofp ) {
                perror( "fopen" );
                return 1;
            }
        }
    }

    read_raw( &magic, sizeof( magic ), ifp );
    blocknum = 0;
    if( magic == MVD_MAGIC ) {
        magic = TXT_MAGIC;
        write_raw( &magic, sizeof( magic ), ofp );
        write_raw( "\n", 1, ofp );
        while( !feof( ifp ) ) {
            n = read_bin( ifp );
            if( !n ) {
                break;
            }
            write_txt( ofp, n, blocknum );
            free_nodes( n );
            blocknum++;
        }
        write_txt( ofp, NULL, 0 );
    } else if( magic == TXT_MAGIC ) {
        magic = MVD_MAGIC;
        write_raw( &magic, sizeof( magic ), ofp );
        while( !feof( ifp ) ) {
            n = read_txt( ifp );
            if( !n ) {
                break;
            }
#ifdef _DEBUG
            trap_pos = SIZE_MAX;
            if( blocknum == 8999 ) trap_pos = 133;
#endif
            write_bin( ofp, n );
            free_nodes( n );
            blocknum++;
        }
        write_bin( ofp, NULL );
    }

    return 0;
}

