#include "main.h"
#include "option.h"

int     cmd_argc;
char    **cmd_argv;
#if 0
int     cmd_optind;
char    *cmd_optarg;
char    *cmd_optopt;

int parse_options( const option_t *opt ) {
    const option_t *o;
    char *s, *p;

    cmd_optopt = "";

    if( cmd_optind == cmd_argc ) {
        cmd_optarg = "";
        return -1; // no more arguments
    }

    s = cmd_argv[cmd_optind];
    if( *s != '-' ) {
        cmd_optarg = s;
        return -1; // non-option argument
    }
    cmd_optopt = s++;

    if( *s == '-' ) {
        s++;
        if( *s == 0 ) {
            if( ++cmd_optind < cmd_argc ) {
                cmd_optarg = cmd_argv[cmd_optind];
            } else {
                cmd_optarg = "";
            }
            return -1; // special terminator
        }
        if( ( p = strchr( s, '=' ) ) != NULL ) {
            *p = 0;
        }
        for( o = opt; o->sh; o++ ) {
            if( !strcmp( o->lo, s ) ) {
                break;
            }
        }
        if( p ) {
            if( o->sh[1] == ':' ) {
                cmd_optarg = p + 1;
            } else {
                fprintf( stderr, "%s does not take an argument.\n", o->lo );
            }
            *p = 0;
        }
    } else {
        p = NULL;
        if( s[1] ) {
            goto unknown;
        }
        for( o = opt; o->sh; o++ ) {
            if( o->sh[0] == *s ) {
                break;
            }
        }
    }

    if( !o->sh ) {
unknown:
        fprintf( stderr, "Unknown option: %s.\n", cmd_argv[cmd_optind] );
        return '?';
    }

    // parse option argument
    if( !p && o->sh[1] == ':' ) {
        if( cmd_optind + 1 == cmd_argc ) {
            fprintf( stderr, "Missing argument to %s.\n", cmd_argv[cmd_optind] );
            return ':';
        }
        cmd_optarg = cmd_argv[++cmd_optind];
    }

    cmd_optind++;

    return o->sh[0];
}

#endif
