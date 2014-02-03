//
//  EPMAppDelegate.m
//  Uninstall
//
//  Created by Michael Sweet on 2/1/2014.
//  Copyright (c) 2014 Michael Sweet. All rights reserved.
//

#import "EPMAppDelegate.h"

@implementation EPMAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
  [self.indicator setDisplayedWhenStopped:NO];
}

// Close application when the window is closed.
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
  return (YES);
}


- (IBAction)doUninstall:(id)sender
{
  (void)sender;

  NSLog(@"doUninstall");

  [self.indicator startAnimation:sender];
  [self.message setStringValue:@"Uninstalling Foo..."];
  [self.uninstallButton setEnabled:NO];
  [self.cancelButton setEnabled:NO];

  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:5]];

  [self.message setStringValue:@"Uninstalling Bar..."];

  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:5]];

  [self.indicator stopAnimation:sender];
  [self.message setStringValue:@"Completed."];
  [self.cancelButton setTitle:@"Quit"];
  [self.cancelButton setEnabled:YES];
}


@end
