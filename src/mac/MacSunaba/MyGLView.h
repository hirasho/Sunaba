//
//  MyGLView.h
//  test
//
//  Created by library on 2013/10/30.
//  Copyright (c) 2013年 library. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

@interface MyGLView : NSView
{
    NSOpenGLContext* _context;
    NSTrackingArea*  _tracking;
@private
    BOOL isFirst;
}

-(void) updateContext;
@end
