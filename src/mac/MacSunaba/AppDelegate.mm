//
//  AppDelegate.m
//  MacSunaba
//
//  Created by library on 2013/11/05.
//  Copyright (c) 2013年 library. All rights reserved.
//

#import "AppDelegate.h"

#define MAX_GLVIEW_SIZE  (300)

static char gProgramFilename[4096] = { 0 };

extern "C"
void setBootProgramPath( const char* path ) {
  strcpy( gProgramFilename, path );
}

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
#ifndef NDEBUG
  // Insert code here to initialize your application
  printf( "didFinishLaunching.\n" );
#endif
}

- (BOOL) application: (NSApplication*) theApplication openFile:(NSString *)filename
{
  // [self.textView setString: filename];
  if( sunabaSys != nil ) {
    [sunabaSys terminate];
    sunabaSys = nil;
  }
  const char* p = [filename UTF8String];
  strcpy( gProgramFilename, p );
  
  sunabaSys = [[SunabaWrapper alloc] init];
  [sunabaSys compileAndExecute: gProgramFilename];
  [self.baseView setNeedsDisplay: YES];
  
  return YES;
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
  if( sunabaSys != nil ) {
    [sunabaSys terminate];
    sunabaSys = nil;
  }
  return YES;
}

- (IBAction)pushRestart:(id)sender {
  int pathLength = (int)strlen( gProgramFilename );
  if( pathLength > 0 ) {
    if( sunabaSys == nil ) {
      sunabaSys = [[SunabaWrapper alloc] init];
    }
    [sunabaSys compileAndExecute: gProgramFilename];
  }
}

- (IBAction) pushScaling:(id)sender
{
#if 0
  viewSize += 100;
  if( viewSize > MAX_GLVIEW_SIZE ) {
    viewSize = 100;
  }
#endif
  ++viewSizeScale;
  if( viewSizeScale > 3 ) {
    viewSizeScale = 1;
  }
  [self locateComponent];
  [self.glview updateContext];
  glViewport( 0, 0, viewSize * viewSizeScale, viewSize * viewSizeScale )
  ;
}


- (NSDragOperation) draggingEntered: (id) sender
{
  // NSLog( @"draggingEnterd" );
  // memset( gProgramFilename, 0, sizeof(gProgramFilename));
  return NSDragOperationCopy;
}

- (BOOL) performDragOperation: (id) sender
{
  NSPasteboard* pboard = [sender draggingPasteboard];
  // NSDragOperation sourceDragMask = [sender draggingSourceOperationMask];
  
  if( [[pboard types] containsObject:NSFilenamesPboardType] ) {
    NSArray* files = [pboard propertyListForType: NSFilenamesPboardType ];
    // NSLog( @"%@", files );
    NSString* file = [files objectAtIndex:0 ];
    const char* p = [file UTF8String];
    strcpy( gProgramFilename, p );
    
    if( sunabaSys == nil ) {
      sunabaSys = [[SunabaWrapper alloc] init];
    }
    [sunabaSys compileAndExecute: gProgramFilename];
  }
  return YES;
}

- (BOOL) prepareForDragOperation: (id) sender
{
  return YES;
}

- (IBAction)pushButton:(id)sender
{
}

- (void) mouseEntered:(NSEvent *)theEvent
{
  
}

-(void) mouseExited:(NSEvent *)theEvent
{
}

- (void) mouseDown: (NSEvent*) event
{
  self->keys[0] = 1;
}

- (void) mouseUp: (NSEvent*) event
{
  self->keys[0] = 0;
}

- (void) rightMouseDown: (NSEvent*) theEvent
{
  self->keys[1] = 1;
}

- (void) rightMouseUp: (NSEvent*) theEvent
{
  self->keys[1] = 0;
}

- (void) mouseMoved:(NSEvent *)theEvent
{
  NSRect contentRect = [self.glview frame];
  NSPoint pt = [self.glview convertPoint:[theEvent locationInWindow] fromView: nil ];
  // NSLog( @"mouseMove: %.2f, %.2f ([%.2f,%.2f])", pt2.x, pt2.y, contentRect.size.width, contentRect.size.height - pt.y);
  mousePos.x = pt.x * 100 / contentRect.size.width;
  mousePos.y = (contentRect.size.height - pt.y) * 100 / contentRect.size.height;
}

- (void) keyDown:(NSEvent *)theEvent
{
  unsigned short keyCode = [theEvent keyCode];
  int index = -1;
  switch (keyCode) {
    case 0x7bU:
      //NSLog( @"left");
      index = 4;
      break;
    case 0x7cU:
      //NSLog( @"right");
      index = 5;
      break;
    case 0x7dU:
      //NSLog( @"down");
      index = 3;
      break;
    case 0x7eU:
      //NSLog( @"up");
      index = 2;
      break;
    case 0x24U:
      //NSLog( @"enter");
      index = 7;
      break;
    case 0x31U:
      //NSLog( @"space");
      index = 6;
      break;
    default:
      break;
  }
  if( index > 0 ) {
    keys[index] = 1;
  }
}

- (void) keyUp:(NSEvent *)theEvent
{
  unsigned short keyCode = [theEvent keyCode];
  int index = -1;
  switch (keyCode) {
    case 0x7bU:
      //NSLog( @"left");
      index = 4;
      break;
    case 0x7cU:
      //NSLog( @"right");
      index = 5;
      break;
    case 0x7dU:
      //NSLog( @"down");
      index = 3;
      break;
    case 0x7eU:
      //NSLog( @"up");
      index = 2;
      break;
    case 0x24U:
      //NSLog( @"enter");
      index = 7;
      break;
    case 0x31U:
      //NSLog( @"space");
      index = 6;
      break;
    default:
      break;
  }
  if( index > 0 ) {
    keys[index] = 0;
  }
}

-(void) awakeFromNib
{
  float interval = 1.0f / 60.0f;
  
  [self.baseView registerForDraggedTypes: [NSArray arrayWithObjects: NSFilenamesPboardType, nil]];
  
  timer = [NSTimer scheduledTimerWithTimeInterval: interval target:self selector:@selector(updateTimer:) userInfo:nil repeats:YES ];
  
  memset( self->keys, 0, sizeof(keys) );
  
  //NSNotificationCenter* aCenter = [NSNotificationCenter defaultCenter];
  //[aCenter addObserver: self selector: @selector(applicationWillTerminate:) name:@"NSApplicationWillTerminateNotification" object: NSApp ];
  NSNotificationCenter* aCenter = [NSNotificationCenter defaultCenter];
  [aCenter addObserver:self selector:@selector(windowResized:) name:NSWindowDidResizeNotification object:[self  window]];
  self->viewSize = 100;
  self->viewSizeScale = 3;
  [self locateComponent];
}

- (void) locateComponent
{
  NSRect frame = [self.baseView frame];
  NSPoint glviewOrigin;
  const int MARGIN = 10;
  // 最大300x300 の中で横方向センタリングする。
  int ofs = 0.5 * (MAX_GLVIEW_SIZE - viewSize * viewSizeScale);
  glviewOrigin.x = frame.size.width - MARGIN - (viewSize * viewSizeScale);
  glviewOrigin.y = MARGIN;
  //[self.glview setFrameOrigin:glviewOrigin];
  [self.glview setFrame: CGRectMake(glviewOrigin.x, glviewOrigin.y, viewSize * viewSizeScale, viewSize * viewSizeScale)];
  
  if( sunabaSys ) {
    int width = [sunabaSys viewWidth] * viewSizeScale;
    int height = [sunabaSys viewHeight] * viewSizeScale;
    [sunabaSys setViewSize: width height: height ];
  }
#if 0
  NSPoint logOrigin;
  logOrigin.x = MARGIN;
  logOrigin.y = MARGIN;
  [self.scrollView setFrameOrigin: logOrigin ];
  
  CGSize logSize;
  logSize.width = frame.size.width - viewSize - MARGIN*2 - MARGIN;
  logSize.height = frame.size.height - MARGIN - 100;
  [self.scrollView setFrameSize: logSize];
  [self.textView setFrameSize: self.scrollView.frame.size ];
#endif
}

- (void) windowResized : (NSNotificationCenter*) aNotif
{
  [self locateComponent];
}

-(void)updateTimer:(NSTimer *)aTimer
{
  if( sunabaSys != nil ) {
    int mx = (int) mousePos.x;
    int my = (int) mousePos.y;
    
    NSString* sunabaVMstatus = [sunabaSys update: mx mouseY: my keyStatus: keys ];
    [self.uiLabel setStringValue: sunabaVMstatus ];
    
    if( viewSize != [sunabaSys viewWidth] || viewSize != [sunabaSys viewHeight] ) {
      viewSize = [sunabaSys viewWidth];
      [self locateComponent];
    }
  }
  if( self.baseView != nil ) {
    [self.glview setNeedsDisplay:YES];
    [self.glview display];
  }
}

-(void) setSunabaMsg:(const char *)msg
{
  NSString* curr = [self.textField stringValue];
  NSString* buf = [curr stringByAppendingString: [NSString stringWithUTF8String:msg]];
  [self.textField setStringValue: buf ];
  [self.textField autoscroll: nil];
  
  
  [self.textView.textStorage beginEditing];
  // テキストを追加する。
  NSAttributedString* str = [[NSAttributedString alloc] initWithString: [NSString stringWithUTF8String:msg] ];
  [self.textView.textStorage appendAttributedString: str];
  [self.textView.textStorage endEditing];
  
  // 最終行の付近にスクロール.
  [self.textView autoscroll: nil ];
}
@end



