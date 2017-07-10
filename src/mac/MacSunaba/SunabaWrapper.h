//
//  SunabaWrapper.h
//  MacSunaba
//
//  Created by library on 2013/11/08.
//  Copyright (c) 2013年 library. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SunabaWrapper : NSObject
- (BOOL) compileAndExecute: (const char*) filePath;
- (NSString*) update: (int) mouseX mouseY:(int)mY keyStatus: (const char*) keys;
- (void) terminate;
- (int) viewWidth;
- (int) viewHeight;
- (void) setViewSize: (int) w height: (int) h;
@end
