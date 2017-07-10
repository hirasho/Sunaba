#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

@interface MyView : NSView <NSDraggingDestination>
{
@private
    NSTrackingArea* _tracking;
}
- (NSDragOperation) draggingEntered: (id<NSDraggingInfo>) sender;
- (void) draggingExited:(id<NSDraggingInfo>)sender;
- (BOOL) performDragOperation: (id) sender;
- (BOOL) prepareForDragOperation: (id) sender;
@end
