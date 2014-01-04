#import <UIKit/UIKit.h>
#include "label.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#import <sys/utsname.h>

void
font_create(int font_size, struct font_context *ctx) {
  UIFont* font = [UIFont fontWithName:@"Helvetica" size:font_size];
	ctx->font = (__bridge void *)font;
	ctx->dc = NULL;
}

void
font_release(struct font_context *ctx) {
}

void 
font_size(const char *str, int unicode, struct font_context *ctx) {
  NSString * tmp = [NSString stringWithUTF8String: str];
  //TODO handle ios 7 for deprecated method
  CGSize sz = [tmp sizeWithFont:(__bridge UIFont *)(ctx->font)];
  ctx->w = (int)sz.width;
  ctx->h = (int)sz.height;
}

static inline void
_font_glyph_gray(NSString* str, void* buffer, struct font_context* ctx){
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
  CGContextRef context = CGBitmapContextCreate(buffer,
                                              (float)ctx->w,
                                              (float)ctx->h,
                                              8, (float)ctx->w,
                                              colorSpace, kCGImageAlphaNone | kCGBitmapByteOrderDefault);
  CGColorSpaceRelease(colorSpace);
  if(context == NULL){
    NSLog(@"the context is NULL! @ _font_glyph_gray function");
    
  }
  
  assert(context);
  CGContextTranslateCTM(context, 0, (float)ctx->h);
  CGContextScaleCTM(context, 1, -1);
  
   UIGraphicsPushContext(context);
  
  if([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0){
    [str drawAtPoint:CGPointMake(0, 0) withAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                       (__bridge id)(ctx->font), NSFontAttributeName,
                                                       [UIColor colorWithWhite:1.0f alpha:1.0f],NSForegroundColorAttributeName,
                                                       nil]];
  }else{
    CGContextSetGrayFillColor(context, 1.0f, 1.0f);
    [str drawAtPoint:CGPointMake(0, 0) withFont:(__bridge UIFont *)(ctx->font)];
  }
  UIGraphicsPopContext();
  CGContextRelease(context);
}

void 
font_glyph(const char * str, int unicode, void * buffer, struct font_context *ctx){
 
  NSString * tmp = [NSString stringWithUTF8String: str];
  
  _font_glyph_gray(tmp, buffer, ctx);
  
}
