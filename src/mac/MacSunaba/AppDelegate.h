//
//  AppDelegate.h
//  MacSunaba
//
//  Created by library on 2013/11/05.
//  Copyright (c) 2013年 library. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MyGLView.h"
#import "MyView.h"
#import "SunabaWrapper.h"

@interface AppDelegate : NSObject <NSApplicationDelegate, NSDraggingDestination>
{
@private
    SunabaWrapper* sunabaSys;
    NSTimer* timer;
    NSPoint  mousePos;
    char     keys[ 8 ];
    int      viewSize;
    int      viewSizeScale;
}
@property (unsafe_unretained) IBOutlet NSViewController *viewController;
@property (assign) IBOutlet NSWindow *window;
@property (weak) IBOutlet NSButton* button;
@property (weak) IBOutlet NSButton *button2;
@property (weak) IBOutlet MyGLView *glview;
@property (weak) IBOutlet MyView *baseView;
@property (weak) IBOutlet NSTextField *textField;
@property (weak) IBOutlet NSTextField *uiLabel;
@property (unsafe_unretained) IBOutlet NSTextView *textView;
@property (weak) IBOutlet NSScrollView *scrollView;

/* NSDraggingDestination*/
- (NSDragOperation) draggingEntered: (id) sender;
- (BOOL) performDragOperation: (id) sender;
- (BOOL) prepareForDragOperation: (id) sender;
/* Update timer */ 
- (void)updateTimer : (NSTimer*) aTimer;

-(IBAction) pushButton:(id)sender;

- (void) mouseEntered: (NSEvent*) theEvent;
- (void) mouseExited: (NSEvent*) theEvent;
- (void) mouseDown: (NSEvent*) theEvent;
- (void) mouseUp: (NSEvent*) theEvent;
- (void) rightMouseDown: (NSEvent*) theEvent;
- (void) rightMouseUp: (NSEvent*) theEvent;
- (void) mouseMoved: (NSEvent*) theEvent;
- (void) keyDown: (NSEvent*) theEvent;
- (void) keyUp: (NSEvent*) theEvent;

- (void) setSunabaMsg: (const char*) msg;
@end
