//
//  SunabaWrapper.m
//  MacSunaba
//
//  Created by library on 2013/11/08.
//  Copyright (c) 2013年 library. All rights reserved.
//

#import "AppDelegate.h"
#import "SunabaWrapper.h"

#include "System.h"

static Sunaba::System* gpSystem = NULL;

bool compileFromPath(
    const char* filePath,
    Sunaba::Array<unsigned int>* instructions,
    const Sunaba::Localization&,
    std::string& resultMsg );

@implementation SunabaWrapper

-(id) init
{
    if( gpSystem == NULL ) {
        gpSystem = new Sunaba::System( NULL, L"japanese" );
    }
    return self;
}

-(BOOL) compileAndExecute:(const char *)filePath
{
    if( gpSystem == NULL ) {
        gpSystem = new Sunaba::System( NULL, L"japanese" );
    }
    std::string msg;
    Sunaba::Array<unsigned int> sunabaInstructions;
    BOOL ret = ( compileFromPath(filePath, &sunabaInstructions, *(gpSystem->localization()), msg) ? YES : NO );
    
    AppDelegate* delegate = (AppDelegate*)[[NSApplication sharedApplication] delegate];
    if( delegate ) {
        [delegate setSunabaMsg: msg.c_str() ];
    }
    
    if( ret ) {
        /* 実行処理 */
        const int instructionSize = sunabaInstructions.size();
        int objectCodeSize = ( sizeof(unsigned int) * instructionSize );
        unsigned char* objectCode = new unsigned char[  objectCodeSize ];
        for( int i = 0; i < instructionSize; ++i ) {
            objectCode[ i*4 + 0 ] = static_cast<unsigned char>( (sunabaInstructions[i] >> 24) & 0xFFu );
            objectCode[ i*4 + 1 ] = static_cast<unsigned char>( (sunabaInstructions[i] >> 16) & 0xFFu );
            objectCode[ i*4 + 2 ] = static_cast<unsigned char>( (sunabaInstructions[i] >> 8) & 0xFFu );
            objectCode[ i*4 + 3 ] = static_cast<unsigned char>( (sunabaInstructions[i] >> 0) & 0xFFu );
            
        }
        ret = gpSystem->bootProgram( objectCode, objectCodeSize );
        delete[] objectCode;
        
        gpSystem->restartGraphics();
    }
    
    return ret;
}

-(BOOL) bootProgramByPath:(NSString *)filePath
{
    return YES;
}

-(NSString*) update: (int) mX mouseY:(int) mY keyStatus:(const char*) keys
{
    Sunaba::Array<unsigned char> msgBuf;
    gpSystem->update( &msgBuf, mX, mY, keys );

    if( msgBuf.size() > 0 ) {
        /* msgBuf に入ってくるデータは UCS2 を想定. 文字数に直すために /2 を実行 */
        // BE -> LE に直して UTF8 変換をする.
        Sunaba::Array<wchar_t> wcharStr;
        int ucs2count = msgBuf.size() / 2;
        wcharStr.setSize( ucs2count );
        
        for( int i = 0; i < ucs2count; ++i ) {
            unsigned char b1 = msgBuf[2*i+0];
            unsigned char b2 = msgBuf[2*i+1];
            wcharStr[i] = (b1 << 8) | b2;
        }
        
        Sunaba::Array<char> convMsg;
        Sunaba::convertUnicodeToUtf8( &convMsg, (wchar_t*)wcharStr.pointer(), (int)wcharStr.size() );

        AppDelegate* delegate = (AppDelegate*)[[NSApplication sharedApplication] delegate];
        if( delegate ) {
            [delegate setSunabaMsg: convMsg.pointer() ];
        }
    }

    char statusBuf[256] = { 0 };
    sprintf(statusBuf, "秒間%d回表示 負荷%d％", gpSystem->framePerSecond(), gpSystem->calculationTimePercent() );
    return [NSString stringWithUTF8String: statusBuf];
}

- (void) terminate
{
    if( gpSystem ) {
        delete gpSystem;
        gpSystem = NULL;
    }
}

- (int) viewWidth
{
    if( gpSystem ) {
        return gpSystem->screenWidth();
    }
    return 0;
}
- (int) viewHeight
{
    if( gpSystem ) {
        return gpSystem->screenHeight();
    }
    return 0;
}
- (void) setViewSize:(int)w height:(int)h
{
    if( gpSystem  ) {
        gpSystem->setPictureBoxHandle( NULL, w, h );
    }
}
@end

bool compileFromPath(
const char* filePath,
Sunaba::Array<unsigned int>* instructions,
const Sunaba::Localization& loc,
std::string& msg ) {
    Sunaba::Array<wchar_t> tmpPath;
    Sunaba::Array<wchar_t> compiled;
    std::wostringstream messageOut;
    int len = (int)strlen(filePath)+1;
    Sunaba::convertToUnicode( &tmpPath, filePath, len);
    
    bool bRet = Sunaba::Compiler::process( &compiled, &messageOut, tmpPath.pointer(), loc);
    if( bRet ) {
        
        bRet = Sunaba::Assembler::process( instructions,  &messageOut, compiled, loc );
        if( bRet ) {
            
        }
    }
    
    Sunaba::Array<char> convMsg;
    Sunaba::convertUnicodeToUtf8( &convMsg, messageOut.str().c_str(), (int)messageOut.str().length() );
    
    // printf( "Wrapper:compileFromPath:\n\t%s\n\n", convMsg.pointer() );
    msg = std::string(convMsg.pointer());
    return bRet;
}
