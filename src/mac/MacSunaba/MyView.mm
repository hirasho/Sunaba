//
//  MyWindow.mm
//  MacSunaba
//
//  Created by library on 2013/11/07.
//  Copyright (c) 2013年 library. All rights reserved.
//
#import "MyView.h"
#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

@implementation MyView

- (id) initWithFrame: (NSRect) rect
{
    self = [super initWithFrame: rect ];
    
    {
        _tracking = [[NSTrackingArea alloc] initWithRect: [self bounds]
                                                 options:(NSTrackingActiveInKeyWindow | NSTrackingMouseMoved | NSTrackingEnabledDuringMouseDrag)
                                                   owner:self
                                                userInfo:nil ];
        [self addTrackingArea: _tracking];
        
    }
    return self;
}
#if 1
- (BOOL) acceptsFirstResponder
{
    return YES;
}
#endif
- (BOOL) canBecomeKeyView
{
    return YES;
}

- (NSDragOperation) draggingEntered: (id<NSDraggingInfo>) sender
{
    NSPasteboard* pboard = [sender draggingPasteboard];
    NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];
    
    if( [[pboard types] containsObject: NSColorPboardType] ) {
        if( sourceDragMask & NSDragOperationGeneric ) {
            return NSDragOperationGeneric;
        }
    }
    // printf( "@MyView::draggingEntered\n" );
    /* 大本のデリゲートクラスへ委譲させる */
    AppDelegate* delegate = (AppDelegate*)[[NSApplication sharedApplication] delegate];
    if( delegate ) {
        [delegate draggingEntered:sender];
    }
    
#if 0 /* 枠を消す作業に失敗するので後回し */
    [[NSColor selectedControlColor] set];
    NSFrameRectWithWidth( [self visibleRect], 2.0 );
    [self displayIfNeeded];
#endif
    return NSDragOperationCopy;
}
- (void) draggingExited:(id<NSDraggingInfo>)sender {
#if 0 /* 枠を消す作業に失敗するので後回し */
    NSEraseRect( [self visibleRect] );
    [[NSColor selectedControlColor] set];
    NSFrameRectWithWidth( [self visibleRect], 0.0f );
    [self displayIfNeeded];
#endif
}

- (BOOL) performDragOperation: (id) sender
{
    /* 大本のデリゲートクラスへ委譲させる */
    AppDelegate* delegate = (AppDelegate*)[[NSApplication sharedApplication] delegate];
    if( delegate ) {
        return [delegate performDragOperation:sender];
    }
    return YES;
}

- (BOOL) prepareForDragOperation: (id) sender
{
    return YES;
}

-(void) updateTrackingAreas
{
    if( _tracking ) {
        [self removeTrackingArea: _tracking];
        _tracking = nil;
    }
    
    _tracking = [[NSTrackingArea alloc] initWithRect: [self bounds]
                                             options:(NSTrackingActiveInKeyWindow | NSTrackingMouseMoved | NSTrackingEnabledDuringMouseDrag)
                                               owner:self
                                            userInfo:nil ];
    [self addTrackingArea: _tracking];
    [super updateTrackingAreas];
}

-(void) mouseDown:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate mouseDown:theEvent];
    }
}

-(void) mouseUp:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate mouseUp: theEvent];
    }
}
- (void) mouseMoved:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate mouseMoved: theEvent];
    }
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate rightMouseDown: theEvent];
    }
}

- (void) rightMouseUp:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate rightMouseUp: theEvent];
    }
}

-(void) mouseDragged:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate mouseMoved: theEvent];
    }
}

-(void) rightMouseDragged:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate mouseMoved: theEvent];
    }
}

-(void) keyDown:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate keyDown: theEvent];
    }
}

-(void) keyUp:(NSEvent *)theEvent
{
    AppDelegate* delegate = [NSApp delegate];
    if( delegate ) {
        [delegate keyUp: theEvent];
    }
}

@end
