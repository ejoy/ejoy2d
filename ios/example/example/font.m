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
    if(ctx->edge == 1) {
        ctx->w = (int)sz.width+4;
        ctx->h = (int)sz.height+4;
    } else {
        ctx->w = (int)sz.width;
        ctx->h = (int)sz.height;
    }
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

//static inline void
//_test_write_ppm3(unsigned char* buffer, int w, int h){
//    static int count = 0;
//    NSMutableString* str = [[NSMutableString alloc] initWithFormat:@"P3\n%d %d\n255\n", w, h];
//    for(int i=0; i<w*h*4; i+=4){
//        [str appendFormat:@"%d ", buffer[i]];   // r
//        [str appendFormat:@"%d ", buffer[i+1]]; // g
//        [str appendFormat:@"%d ", buffer[i+2]];
//    }
//    
//    NSString* file = [NSString stringWithFormat:@"%@/Documents/font_%d.ppm", NSHomeDirectory(), count++];
//    [str writeToFile:file atomically:YES encoding:NSUTF8StringEncoding error:NULL];
//}

static inline void
_font_glyph_rgba(NSString* str, void* buffer, struct font_context* ctx){
    float w = (float)ctx->w;
    float h = (float)ctx->h;
    UIFont* font = (__bridge id)(ctx->font);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(buffer, w, h, 8, w*4,
                                                 colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault);
    if(context == NULL){
        NSLog(@"the context is NULL! @ _font_glyph_rgba function ");
        CGColorSpaceRelease(colorSpace);
        return;
    }
    
    CGContextTranslateCTM(context, 0, h);
    CGContextScaleCTM(context, 1, -1);
    
    UIGraphicsPushContext(context);

    if([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0){
        if (ctx->edge) {
            CGContextSetLineWidth(context, 4);
            CGContextSetTextDrawingMode(context, kCGTextStroke);
            NSDictionary* _attr =[NSDictionary dictionaryWithObjectsAndKeys:
                              font, NSFontAttributeName,
                              [UIColor whiteColor], NSForegroundColorAttributeName,
                              [UIColor blackColor], NSStrokeColorAttributeName,
                              nil];
            [str drawAtPoint:CGPointMake(2, 2) withAttributes:_attr];
        
            CGContextSetTextDrawingMode(context, kCGTextFill);
            [str drawAtPoint:CGPointMake(2, 2) withAttributes:_attr];
        } else {
            NSDictionary* _attr =[NSDictionary dictionaryWithObjectsAndKeys:
                                  font, NSFontAttributeName,
                                  [UIColor whiteColor], NSForegroundColorAttributeName,
                                  nil];
            [str drawAtPoint:CGPointMake(0, 0) withAttributes:_attr];
        }
    }
    else {
        
        if (ctx->edge) {
            CGContextSetRGBFillColor(context, 1, 1, 1, 1);
        
            CGContextSetLineWidth(context, 4);
            CGContextSetLineJoin(context, kCGLineJoinRound);
            CGContextSetRGBStrokeColor(context, 0, 0, 0, 1);
            CGContextSetTextDrawingMode(context, kCGTextStroke);
            [str drawAtPoint:CGPointMake(2,2) withFont:font];
        
            CGContextSetTextDrawingMode(context, kCGTextFill);
            [str drawAtPoint:CGPointMake(2,2) withFont:font];
        } else {
            [str drawAtPoint:CGPointMake(0,0) withFont:font];
        }
    }
    
    UIGraphicsPopContext();
    CGContextRelease(context);
    
//    _test_write_ppm3(buffer, ctx->w, ctx->h);
}


void
font_glyph(const char * str, int unicode, void * buffer, struct font_context *ctx){
 
    NSString * tmp = [NSString stringWithUTF8String: str];
    _font_glyph_rgba(tmp, buffer, ctx);
//    _font_glyph_gray(tmp, buffer, ctx);
}


//void log_output(const char *format)
//{
//    NSString *str = [NSString stringWithUTF8String:format];
//    NSLog(@"%@", str);
//}
