//
//  EPMAppDelegate.h
//  Uninstall
//
//  Created by Michael Sweet on 2/1/2014.
//  Copyright (c) 2014 Michael Sweet. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface EPMAppDelegate : NSObject <NSApplicationDelegate>

@property (assign) IBOutlet NSButton *cancelButton;
@property (assign) IBOutlet NSTextField *message;
@property (assign) IBOutlet NSProgressIndicator *indicator;
@property (assign) IBOutlet NSButton *uninstallButton;
@property (assign) IBOutlet NSWindow *window;

- (IBAction)doUninstall:(id)sender;
@end
