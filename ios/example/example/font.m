#import <UIKit/UIKit.h>
#include "font.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#import <sys/utsname.h>

// static const struct dfont_rect *
// gen_char(int unicode, const char * utf8, int size) {
//     NSString * tmp = [NSString stringWithUTF8String: utf8];
//     UIFont * font = [UIFont systemFontOfSize:size];
//     CGSize sz = [tmp sizeWithFont:font];
// 
//     const struct dfont_rect * rect = dfont_insert(Dfont, unicode, size, (int)sz.width, (int)sz.height);
//     if (rect == NULL)
//         return NULL;
//     
//     int buffer_sz = rect->w * rect->h;
//     uint8_t buffer[buffer_sz];
//     memset(buffer,0,buffer_sz);
//     
//     CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
//     CGContextRef context = CGBitmapContextCreate(buffer, sz.width, sz.height,8,sz.width,colorSpace,kCGImageAlphaNone);
//     CGColorSpaceRelease(colorSpace);
//     CGContextSetGrayFillColor(context, 1,1);
//     CGContextTranslateCTM(context, 0, sz.height);
//     CGContextScaleCTM(context, 1, -1);
//     UIGraphicsPushContext(context);
//     [tmp drawAtPoint:CGPointMake(0,0) withFont:font];
//     UIGraphicsPopContext();
//     CGContextRelease(context);
//     
//     glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->w, rect->h, GL_ALPHA, GL_UNSIGNED_BYTE, buffer);
//     
//     return rect;
// }

void
font_create(int font_size, struct font_context *ctx) {
//  UIFont * font = [UIFont boldSystemFontOfSize:font_size];
//  UIFont* font = [UIFont fontWithName:@"Supercell-Magic" size:font_size];
//  UIFont* font = [UIFont fontWithName:@"MicrosoftYaHei" size:font_size];
//    UIFont* font = [UIFont fontWithName:@"FZY4JW--GB1-0" size:font_size];
  UIFont* font = [UIFont fontWithName:@"FZCuYuan-M03" size:font_size];
	ctx->font = (void *)font;
	ctx->dc = NULL;
  ctx->size = font_size;
}

void
font_release(struct font_context *ctx) {
}

void 
font_size(const char *str, int unicode, struct font_context *ctx) {
    NSString * tmp = [NSString stringWithUTF8String: str];
  CGSize sz = [tmp sizeWithAttributes:@{NSFontAttributeName:[UIFont systemFontOfSize:ctx->size]}];
  if(!(ctx->size & 0x01)){
    ctx->w = (int)sz.width+4;
    ctx->h = (int)sz.height+4;
  }else{
    ctx->w = (int)sz.width;
    ctx->h = (int)sz.height;
  }
}

static inline void
_edge_scale(unsigned char* buffer, int width, int height){
  int pos = 0;
  for(int i=0; i<width*height*4; i+=4){
    int r = buffer[i];
    int g = buffer[i+1];
    int b = buffer[i+2];
    int a = buffer[i+3];
    
    float gray_scale = r*0.22 + g*0.707 + b*0.071;
    if(a>254){
      buffer[pos] = gray_scale*0.5f + 128;
    }else{
      buffer[pos] = a *0.5f;
    }
    pos++;
  }
}

/*
static void
_test_write_ppm3(unsigned char* buffer, int w, int h){
  static int count = 0;
  NSMutableString* str = [[[NSMutableString alloc] initWithFormat:@"P3\n%d %d\n255\n", w, h] autorelease];
  for(int i=0; i<w*h*4; i+=4){
    [str appendFormat:@"%d ", buffer[i]]; // r
    [str appendFormat:@"%d ", buffer[i+1]]; // g
    [str appendFormat:@"%d ", buffer[i+2]];
  }
  
  NSString* file = [NSString stringWithFormat:@"%@/Documents/font_%d.ppm", NSHomeDirectory(), count++];
  [str writeToFile:file atomically:YES encoding:NSUTF8StringEncoding error:NULL];
}
 
 static void
 _test_write_ppm2(unsigned char* buffer, int w, int h){
 static int count = 0;
 NSMutableString* str = [[[NSMutableString alloc] initWithFormat:@"P2\n%d %d\n255\n", w, h] autorelease];
 for(int i=0; i<w*h; i++){
 [str appendFormat:@"%d ", buffer[i]];
 }
 
 NSString* file = [NSString stringWithFormat:@"%@/Documents/font_%d.ppm", NSHomeDirectory(), count++];
 [str writeToFile:file atomically:YES encoding:NSUTF8StringEncoding error:NULL];
 }

*/

static  void* _gen_glyph(NSString* str, UIFont* font, int is_edge, UIColor* color, int* w, int *h, void* buffer){
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGSize sz = [str sizeWithAttributes:@{NSFontAttributeName:[NSDictionary dictionaryWithObject:font forKey: NSFontAttributeName]}];
  
  if(is_edge){
    sz.width  +=4;
    sz.height +=4;
  }
  
  *w = sz.width;
  *h = sz.height;
  
  int flag = 0;
  if(!buffer){
    flag = 1;
    buffer = calloc(1, 4*sz.height*sz.width);
  }
  
  CGContextRef context = CGBitmapContextCreate(buffer,
                                               (float)sz.width, (float)sz.height, 8, (float)sz.width*4,
                                               colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault);
  if(context == NULL){
    if(flag) free(buffer);
    NSLog(@"the context is NULL! @ _font_glyph_rgba function ");
    return NULL;
  }
  assert(context);
  CGColorSpaceRelease(colorSpace);
  CGContextTranslateCTM(context, 0, (float)sz.height);
  CGContextScaleCTM(context, 1, -1);
  
  UIGraphicsPushContext(context);
  
  if([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0){
    if (is_edge){
      CGContextSetLineWidth(context, 4);
      CGContextSetTextDrawingMode(context, kCGTextStroke);
      NSDictionary* _attr =[NSDictionary dictionaryWithObjectsAndKeys:
                            font,NSFontAttributeName,
                            color, NSForegroundColorAttributeName,
                            [UIColor blackColor], NSStrokeColorAttributeName,
                            nil];
      [str drawAtPoint:CGPointMake(2, 2) withAttributes:_attr];
      
      CGContextSetTextDrawingMode(context, kCGTextFill);
      [str drawAtPoint:CGPointMake(2, 2) withAttributes:_attr];
    }else{
      NSDictionary* _attr =[NSDictionary dictionaryWithObjectsAndKeys:
                            font,NSFontAttributeName,
                            color, NSForegroundColorAttributeName,
                            nil];
      [str drawAtPoint:CGPointMake(0, 0) withAttributes:_attr];
    }
  }else{
    CGFloat r,g,b,a;
    [color getRed:&r green:&g blue:&b alpha:&a];
    
    CGContextSetRGBFillColor(context, r, g, b, a);
    if(is_edge){
      CGContextSetLineWidth(context, 4);
      CGContextSetLineJoin(context, kCGLineJoinRound);
      CGContextSetRGBStrokeColor(context, 0, 0, 0, 1);
      CGContextSetTextDrawingMode(context, kCGTextStroke);
      [str drawAtPoint:CGPointMake(2,2) withAttributes:@{NSFontAttributeName:[NSDictionary dictionaryWithObject:font forKey: NSFontAttributeName]}];
      
      CGContextSetTextDrawingMode(context, kCGTextFill);
      [str drawAtPoint:CGPointMake(2,2) withAttributes:@{NSFontAttributeName:[NSDictionary dictionaryWithObject:font forKey: NSFontAttributeName]}];
    }else{
      [str drawAtPoint:CGPointMake(0,0) withAttributes:@{NSFontAttributeName:[NSDictionary dictionaryWithObject:font forKey: NSFontAttributeName]}];
    }
  }
  
  UIGraphicsPopContext();
  CGContextRelease(context);
  
  return buffer;
}

void* font_write_glyph(const char* str, int size, uint32_t color, int is_edge,
                       int* width, int* height){
  NSString* _str = [NSString stringWithUTF8String:str];
  if(!_str) return NULL;
  
  UIFont* font = [UIFont fontWithName:@"FZCuYuan-M03" size:size];
  CGFloat b, g, r, a;
  b = (color & 0xff)/255.0;
  g = ((color >> 8) & 0xff)/255.0;
  r = ((color >> 16) & 0xff)/255.0;
  a = (color >> 24)/255.0;
  
  UIColor* _color = [UIColor colorWithRed:r green:g blue:b alpha:a];
  void* buffer = _gen_glyph(_str, font, is_edge, _color, width, height, NULL);

  return buffer;
}

static inline void
_font_glyph_rgba(NSString* str, void* buffer, struct font_context* ctx){
  _gen_glyph(str, ctx->font, 1, [UIColor whiteColor], NULL, NULL, buffer);
  
//  _test_write_ppm3(buffer, ctx->w, ctx->h);
  _edge_scale(buffer, ctx->w, ctx->h);
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
                                                       ctx->font, NSFontAttributeName,
                                                       [UIColor colorWithWhite:1.0f alpha:1.0f],NSForegroundColorAttributeName,
                                                       nil]];
  }else{
    CGContextSetGrayFillColor(context, 1.0f, 1.0f);
    [str drawAtPoint:CGPointMake(0, 0) withAttributes:@{NSFontAttributeName:[NSDictionary dictionaryWithObject:ctx->font forKey: NSFontAttributeName]}];
  }
  UIGraphicsPopContext();
  CGContextRelease(context);
  
//  _test_write_ppm2(buffer, ctx->w, ctx->h);
}

void 
font_glyph(const char * str, int unicode, void * buffer, struct font_context *ctx){
  BOOL is_edge = !(ctx->size & 0x01);
  NSString * tmp = [NSString stringWithUTF8String: str];
  if(is_edge){
    _font_glyph_rgba(tmp, buffer, ctx);
  }else{
    _font_glyph_gray(tmp, buffer, ctx);
  }
}
