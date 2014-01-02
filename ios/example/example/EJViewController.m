#import "EJViewController.h"
#import "lejoy2d.h"
#import "ejoy2d.h"

static EJViewController* _controller = nil;

@interface EJViewController () {
  int disableGesture;
}
@property (strong, nonatomic) EAGLContext *context;

- (void) setGesture;

@end

@implementation EJViewController
- (id)init {
  _controller = [super init];
  return _controller;
}

-(void) loadView {
    self.view = [[GLKView alloc] initWithFrame:[UIScreen mainScreen].bounds];
}

+(EJViewController*)getLastInstance{
  return _controller;
}

- (BOOL) shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return interfaceOrientation == UIInterfaceOrientationLandscapeLeft || interfaceOrientation == UIInterfaceOrientationLandscapeRight;
}

- (void) setGesture
{
  UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePan:)];
  pan.delegate = self;
  [[self view] addGestureRecognizer:pan];

  UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleTap:)];
  tap.delegate = self;
  [[self view] addGestureRecognizer:tap];

  UIPinchGestureRecognizer *pinch = [[UIPinchGestureRecognizer alloc]
    initWithTarget:self action:@selector(handlePinch:)];
  pinch.delegate = self;
  [[self view] addGestureRecognizer:pinch];

  UILongPressGestureRecognizer *press = [[UILongPressGestureRecognizer alloc]
    initWithTarget:self action:@selector(handleLongPress:)];
  press.delegate = self;
  [[self view] addGestureRecognizer:press];
}

- (void)dealloc
{
  lejoy_exit();
  _controller = nil;
  if ([EAGLContext currentContext] == self.context) {
    [EAGLContext setCurrentContext:nil];
  }
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self setGesture];

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

  struct ej_screen scr;
  // swap width and height for horizontal-view
  scr.w = bounds.size.height;
  scr.h = bounds.size.width;
  scr.scale = screenScale;
  ejoy_set_screen(&scr);

  lejoy_init();
}

// iOS 7 隐藏状态栏
-(BOOL)prefersStatusBarHidden
{
  return YES;
}

// iOS 4 或 iOS 5 中, 应用收到 Memory warning 时调用
// iOS 6 中废弃 deprecated, 任何情况下都不会触发
//
// iOS 6 中, 不需要做任何以前 viewDidUnload 的事情,
// 更不需要把以前 viewDidUnload 的代码移动到 didReceiveMemoryWarning 方法中
-(void)viewDidUnload
{
  [super viewDidUnload];

  NSLog(@"viewDidUnload");

  lejoy_unload();

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
  lejoy_update(self.timeSinceLastUpdate);
  if(lejoy_check_reload()) {
    lejoy_reload();
  }
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
  lejoy_drawframe();
}

- (BOOL) gestureRecognizer:(UIGestureRecognizer *) gr shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *) ogr {
  return YES;
}

// handle Gestures

static int
getStateCode(UIGestureRecognizerState state) {
  switch(state) {
    case UIGestureRecognizerStatePossible: return STATE_POSSIBLE;
    case UIGestureRecognizerStateBegan: return STATE_BEGAN;
    case UIGestureRecognizerStateChanged: return STATE_CHANGED;
    case UIGestureRecognizerStateEnded: return STATE_ENDED;
    case UIGestureRecognizerStateCancelled: return STATE_CANCELLED;
    case UIGestureRecognizerStateFailed: return STATE_FAILED;

    // recognized == ended
    // case UIGestureRecognizerStateRecognized: return STATE_RECOGNIZED;

    default: return STATE_POSSIBLE;
  }
}

- (void) handlePan:(UIPanGestureRecognizer *) gr {
  int state = getStateCode(gr.state);
  CGPoint trans = [gr translationInView:self.view];
  // CGPoint p = [gr locationInView:self.view];
  CGPoint v = [gr velocityInView:self.view];
  [gr setTranslation:CGPointMake(0,0) inView:self.view];
  // lejoy_gesture("PAN",(int)trans.x,(int)trans.y,(int)p.x,(int)p.y);
  lejoy_gesture("PAN",trans.x,trans.y,v.x,v.y, state);
}

- (void) handleTap:(UITapGestureRecognizer *) gr {
  int state = getStateCode(gr.state);
  CGPoint p = [gr locationInView:self.view];
  lejoy_gesture("TAP", p.x, p.y, 0.0, 0.0, state);
}

- (void) handlePinch:(UIPinchGestureRecognizer *) gr {
  int state = getStateCode(gr.state);
  CGPoint p = [gr locationInView:self.view];
  lejoy_gesture("PINCH", p.x, p.y, (gr.scale * 1024.0), 0.0, state);
  gr.scale = 1;
}

- (void) handleLongPress:(UILongPressGestureRecognizer *) gr {
  int state = getStateCode(gr.state);
  CGPoint p = [gr locationInView:self.view];
  lejoy_gesture("PRESS", p.x, p.y, 0.0, 0.0, state);
}

// handle Touches

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
    disableGesture = lejoy_touch("BEGIN", p.x, p.y);
  }
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
    lejoy_touch("MOVE", p.x, p.y);
  }
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
    lejoy_touch("END", p.x, p.y);
  }
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
  // UITouch *touch = [touches anyObject];
  for(UITouch *touch in touches) {
    CGPoint p = [touch locationInView:touch.view];
    lejoy_touch("CANCEL", p.x, p.y);
  }
}

- (BOOL) gestureRecognizerShouldBegin:(UIGestureRecognizer *)gr {
  return (disableGesture == 0 ? YES : NO);
}

@end

