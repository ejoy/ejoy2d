#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@class EJViewController;

@interface EJViewController : GLKViewController <UIGestureRecognizerDelegate>

+(EJViewController*)getLastInstance;

@end
