#pragma once
#include <Platform.h>

#if VTS_PLATFORM_OSX
// NOTE: This header uses Objective-C types; include only from .mm files.
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

// ---------------------------------------------------------------------------
// Window delegate: record close events
// ---------------------------------------------------------------------------
@interface MacWindowDelegate : NSObject<NSWindowDelegate>
@property (nonatomic, assign) bool isClosed;
@end

@class MacApplicationDelegate;

// ---------------------------------------------------------------------------
// WindowMac – wraps NSWindow + CAMetalLayer
// ---------------------------------------------------------------------------
struct WindowMac
{
    NSWindow*            window     = nil;
    CAMetalLayer*        metalLayer = nil;
    MacWindowDelegate*   delegate   = nil;
    MacApplicationDelegate* applicationDelegate = nil;
    int width  = 0;
    int height = 0;

    WindowMac(const char* title, int w, int h);
    ~WindowMac();

    bool IsClosed() const { return delegate.isClosed; }

    void PumpEvents();
};

#endif // VTS_PLATFORM_OSX
