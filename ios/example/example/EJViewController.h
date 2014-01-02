//
//  EJViewController.h
//  coco
//
//  Created by Cloud on 13-3-14.
//  Copyright (c) 2013年 Cloud. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@class EJViewController;

@interface EJViewController : GLKViewController <UIGestureRecognizerDelegate>

+(EJViewController*)getLastInstance;

@end
