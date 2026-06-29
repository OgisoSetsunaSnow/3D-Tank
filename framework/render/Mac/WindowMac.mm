#include <Platform.h>

#if VTS_PLATFORM_OSX
#include "render/Mac/WindowMac.h"

@interface MacApplicationDelegate : NSObject<NSApplicationDelegate>
@end

static void InstallApplicationMenu(NSString* appName)
{
	if ([NSApp mainMenu] != nil)
		return;

	NSMenu* menuBar = [[NSMenu alloc] initWithTitle:@"MenuBar"];
	NSMenuItem* appMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
	[menuBar addItem:appMenuItem];
	[NSApp setMainMenu:menuBar];

	NSMenu* appMenu = [[NSMenu alloc] initWithTitle:appName ?: @"App"];
	NSString* quitTitle = [@"Quit " stringByAppendingString:(appName ?: @"App")];
	NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:quitTitle
													  action:@selector(terminate:)
											   keyEquivalent:@"q"];
	[appMenu addItem:quitItem];
	[appMenuItem setSubmenu:appMenu];

#if !__has_feature(objc_arc)
	[quitItem release];
	[appMenu release];
	[appMenuItem release];
	[menuBar release];
#endif
}

static MacApplicationDelegate* EnsureApplicationInitialized(const char* title)
{
	[NSApplication sharedApplication];
	[[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"NSQuitAlwaysKeepsWindows": @NO }];

	MacApplicationDelegate* appDelegate = (MacApplicationDelegate*)[NSApp delegate];
	if (!appDelegate)
	{
		appDelegate = [[MacApplicationDelegate alloc] init];
		[NSApp setDelegate:appDelegate];
	}

	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	NSString* appName = title ? [NSString stringWithUTF8String:title] : [[NSProcessInfo processInfo] processName];
	InstallApplicationMenu(appName);

	if (![NSApp isRunning])
		[NSApp finishLaunching];

	return appDelegate;
}

@implementation MacWindowDelegate
- (void)windowWillClose:(NSNotification*)notification { (void)notification; _isClosed = true; }
@end

@implementation MacApplicationDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
	(void)sender;
	return YES;
}

- (BOOL)applicationShouldSaveApplicationState:(NSApplication*)app
{
	(void)app;
	return NO;
}

- (BOOL)applicationShouldRestoreApplicationState:(NSApplication*)app
{
	(void)app;
	return NO;
}

@end

WindowMac::WindowMac(const char* title, int w, int h) : width(w), height(h)
{
	applicationDelegate = EnsureApplicationInitialized(title);

	NSRect frame = NSMakeRect(0, 0, (CGFloat)w, (CGFloat)h);
	NSWindowStyleMask style = NSWindowStyleMaskTitled
							| NSWindowStyleMaskClosable
							| NSWindowStyleMaskMiniaturizable;
	window = [[NSWindow alloc] initWithContentRect:frame
										 styleMask:style
										   backing:NSBackingStoreBuffered
											 defer:NO];
	[window setTitle:[NSString stringWithUTF8String:title]];
	if ([window respondsToSelector:@selector(setRestorable:)])
		[window setRestorable:NO];
	[window center];

	delegate = [[MacWindowDelegate alloc] init];
	[window setDelegate:delegate];

	NSView* view = [[NSView alloc] initWithFrame:frame];
	[view setWantsLayer:YES];
	metalLayer = [CAMetalLayer layer];
	metalLayer.frame = view.bounds;
	[view setLayer:metalLayer];
	[window setContentView:view];
#if !__has_feature(objc_arc)
	[view release];
#endif

	[window makeKeyAndOrderFront:nil];
}

WindowMac::~WindowMac()
{
	if (window)
		[window orderOut:nil];
	if (window)
		[window setDelegate:nil];
	if ([NSApp delegate] == applicationDelegate)
		[NSApp setDelegate:nil];
#if !__has_feature(objc_arc)
	if (applicationDelegate)
		[applicationDelegate release];
	if (delegate)
		[delegate release];
	if (window)
		[window release];
#endif
	applicationDelegate = nil;
	delegate = nil;
	metalLayer = nil;
	window = nil;
}

void WindowMac::PumpEvents()
{
	@autoreleasepool {
		NSEvent* evt;
		NSDate* pollTime = [NSDate distantPast];
		while ((evt = [NSApp nextEventMatchingMask:NSEventMaskAny
										 untilDate:pollTime
											inMode:NSDefaultRunLoopMode
										   dequeue:YES]) != nil)
		{
			[NSApp sendEvent:evt];
		}
		[NSApp updateWindows];
	}
}

#endif // VTS_PLATFORM_OSX

