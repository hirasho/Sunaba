//
//  MyController.h
//  MacSunaba
//
//  Created by library on 2013/11/08.
//  Copyright (c) 2013年 library. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

@interface MyController : NSObject<NSApplicationDelegate>
@property (assign) IBOutlet NSWindow *window;
@property (weak) IBOutlet NSButton* button;

- (IBAction) onButtonClicked:(id)sender;
@end
