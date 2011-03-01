#include "main.h"
#include "proto.h"
#include "node.h"

static void free_list( void *n ) {
    void *next;

    for( ; n; n = next ) {
        next = ((node_t *)n)->next;
        free( n );
    }
}

static void free_frame( frame_t *f ) {
    free( f->portalbits );
    free_list( f->players );
    free_list( f->entities );
    free( f );
}

static void free_gamestate( game_state_t *g ) {
    free_list( g->configstrings );
    free_frame( ( frame_t * )g->baseframe );
    free( g );
}

static void free_multicast( multicast_t *m ) {
    free_list( m->data );
    free( m );
}

static void free_unicast( unicast_t *u ) {
    free_list( u->data );
    free( u );
}

void free_node( void *n ) {
    switch( ((node_t *)n)->type ) {
    case NODE_GAMESTATE:
        free_gamestate( n );
        break;
    case NODE_MULTICAST:
        free_multicast( n );
        break;
    case NODE_UNICAST:
        free_unicast( n );
        break;
    case NODE_FRAME:
        free_frame( n );
        break;
    case NODE_CONFIGSTRING:
    case NODE_SOUND:
    case NODE_PRINT:
    case NODE_NOP:
        free( n );
        break;
    default:
        fatal( "bad node type" );
    }
}

void free_nodes( void *n ) {
    void *next;

    for( ; n; n = next ) {
        next = ((node_t *)n)->next;
        free_node( n );
    }
}

void *alloc_node( unsigned type, size_t size ) {
    node_t *n;

    n = malloc( size );
    if( !n ) {
        fatal( "out of memory" );
    }
    //memset(n,0,size);
    //{
    //    size_t i;
    //    for(i=0;i<size;i++) ((uint8_t *)n)[i]=rand()&0xff;
    //}

    n->type = type;
    n->next = NULL;
    return n;
}

node_t *build_list( build_func_t build ) {
    node_t *ret, *n, **next_p;

    next_p = &ret;
    while( 1 ) {
        n = build();
        if( !n ) {
            break;
        }
        *next_p = n;
        next_p = &n->next;
    }
    *next_p = NULL;

    return ret;
}

void _iter_list( void *n, iter_func_t iter ) {
    for( ; n; n = ((node_t *)n)->next ) {
        iter( n );
    }
}

