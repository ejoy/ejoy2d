#import "EJViewController.h"
//#import "lejoy2d.h"
//#import "ejoy2d.h"
#import "winfw.h"

static EJViewController* _controller = nil;

@interface EJViewController () {
}
@property (strong, nonatomic) EAGLContext *context;

@end

@implementation EJViewController
- (id)init {
  _controller = [super init];
  return _controller;
}

-(void) loadView {
    CGRect bounds = [UIScreen mainScreen].bounds;
    self.view = [[GLKView alloc] initWithFrame:bounds];
}

+(EJViewController*)getLastInstance{
  return _controller;
}

- (BOOL) shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return interfaceOrientation == UIInterfaceOrientationLandscapeLeft || interfaceOrientation == UIInterfaceOrientationLandscapeRight;
}

- (void)dealloc
{
  //lejoy_exit();
  _controller = nil;
  if ([EAGLContext currentContext] == self.context) {
    [EAGLContext setCurrentContext:nil];
  }
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  
  NSLog(@"viewDidLoad");

  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

  if (!self.context) {
    NSLog(@"Failed to create ES context");
  }

  GLKView *view = (GLKView *)self.view;
  view.context = self.context;

  [EAGLContext setCurrentContext:self.context];

  CGFloat screenScale = [[UIScreen mainScreen] scale];
  CGRect bounds = [[UIScreen mainScreen] bounds];

  printf("screenScale: %f\n", screenScale);
  printf("bounds: x:%f y:%f w:%f h:%f\n",
     bounds.origin.x, bounds.origin.y,
     bounds.size.width, bounds.size.height);
  
  NSString *appFolderPath = [[NSBundle mainBundle] resourcePath];
  const char* folder = [appFolderPath UTF8String];

  ejoy2d_win_init(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height, screenScale, folder);
}

-(BOOL)prefersStatusBarHidden
{
  return YES;
}

-(void)viewDidUnload
{
  [super viewDidUnload];

  NSLog(@"viewDidUnload");

//  lejoy_unload();

  if ([self isViewLoaded] && ([[self view] window] == nil)) {
    self.view = nil;

    if ([EAGLContext currentContext] == self.context) {
      [EAGLContext setCurrentContext:nil];
    }
    self.context = nil;
  }
}

- (void)update
{
  ejoy2d_win_update();
/*  lejoy_update(self.timeSinceLastUpdate);
  if(lejoy_check_reload()) {
    lejoy_reload();
  }*/
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
  ejoy2d_win_frame();
//   lejoy_drawframe();
}

// handle Touches

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
    ejoy2d_win_touch(p.x, p.y, TOUCH_BEGIN);
  }
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
    ejoy2d_win_touch(p.x, p.y, TOUCH_MOVE);
  }
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
    ejoy2d_win_touch(p.x, p.y, TOUCH_END);
  }
}
/*
- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
//    lejoy_touch("CANCEL", p.x, p.y);
  }
}
*/
@end

