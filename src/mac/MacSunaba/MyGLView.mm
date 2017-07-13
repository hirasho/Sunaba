//
//  MyGLView.m
//  test
//
//  Created by library on 2013/10/30.
//  Copyright (c) 2013年 library. All rights reserved.
//

#include <iostream>
#import "AppDelegate.h"
#import "MyGLView.h"

static NSOpenGLContext* pContextGL;

@implementation MyGLView

- (id)initWithFrame:(NSRect)frame
{
  self = [super initWithFrame: frame];
  if( self ) {
    if( self == nil ) {
      return nil;
    }

    NSOpenGLPixelFormatAttribute attribs[] =
    {
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFADepthSize, 16,
		0
    };
	
    NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attribs ];
    _context = [[NSOpenGLContext alloc] initWithFormat: fmt shareContext:nil ];
    pContextGL = _context;
	  
    if( pContextGL ) {
      [_context makeCurrentContext];
      glClearColor( 0.0f,0.0f,0.0f,1.0f);
      glClearDepth( 1.0f );
      isFirst = YES;
      glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	  glFlush();
      [pContextGL flushBuffer];
	}
  }
  return self;
}

- (void)lockFocus
{
  [super lockFocus];
  if( [_context view] != self ) {
    [_context setView: self ];
  }
  [_context makeCurrentContext];
}

-(void) drawRect:(NSRect)dirtyRect
{
  if( isFirst ) {
    glClearColor( 0.0f,0.0f,0.0f,1.0f);
    glClearDepth( 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glFlush();
    [pContextGL flushBuffer];
    isFirst = NO;
  }
}

-(void) dealloc
{
}

-(void) updateContext
{
  if( pContextGL ) {
    [pContextGL update];
  }
}
@end

extern "C"
void setContextMyGLView()
{
  [pContextGL makeCurrentContext];
}

extern "C"
void flushContentMyGLView()
{
  [pContextGL flushBuffer];
}
