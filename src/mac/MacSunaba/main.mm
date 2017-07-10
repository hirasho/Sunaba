//
//  main.m
//  MacSunaba
//
//  Created by library on 2013/11/05.
//  Copyright (c) 2013年 library. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

extern "C"
void setBootProgramPath( const char* path );

int main(int argc, const char * argv[])
{    
#if 0
    {
        FILE* fp = NULL;
        fp = fopen( "/Users/library/log-args.txt", "wb" );
        if( fp ) {
            for( int i = 0; i < argc; ++i ) {
                fprintf(fp, "(%d) %s\n", i, argv[i]);
                printf( "(%d) %s\n", i, argv[i] );
            }
            fclose(fp);
        }
    }
#endif
    
    if( argc > 2 ) {
        const char* path = argv[1];
        if( path[0] == '/' ) {
            /* フルパス始まりならプログラムファイルと判定 */
            setBootProgramPath( path );
        }
    }
    
    return NSApplicationMain(argc, argv);
}
