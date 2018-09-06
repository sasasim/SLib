/*
 *  Copyright (c) 2008-2017 SLIBIO. All Rights Reserved.
 *
 *  This file is part of the SLib.io project.
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/window.h"

#include "slib/ui/screen.h"
#include "slib/ui/core.h"
#include "slib/ui/platform.h"

#include "view_macos.h"

namespace slib
{
	class _priv_macOS_Window;
}

@interface _priv_Slib_macOS_Window : NSWindow<NSWindowDelegate> {
@public slib::WeakRef<slib::_priv_macOS_Window> m_window;
	
@public sl_bool m_flagClosing;
@public sl_bool m_flagModal;

}
@end

namespace slib
{

	class _priv_macOS_Window : public WindowInstance
	{
	public:
		NSWindow* m_window;
		__weak NSWindow* m_parent;
		
		sl_ui_len m_heightScreen;
		
		AtomicRef<ViewInstance> m_viewContent;
		
	public:
		_priv_macOS_Window()
		{
		}
		
		~_priv_macOS_Window()
		{
			release();
		}

	public:
		static Ref<_priv_macOS_Window> create(NSWindow* window)
		{
			if (window != nil) {
				sl_ui_pos heightScreen;
				NSScreen* screen = [window screen];
				if (screen != nil) {
					NSRect rc = [screen frame];
					heightScreen = (sl_ui_pos)(rc.size.height);
					if (heightScreen < 0) {
						heightScreen = 0;
					}
				} else {
					heightScreen = 0;
				}
				Ref<_priv_macOS_Window> ret = new _priv_macOS_Window();
				if (ret.isNotNull()) {
					ret->m_window = window;
					ret->m_heightScreen = heightScreen;
					NSView* view = [window contentView];
					if (view != nil) {
						Ref<ViewInstance> content = UIPlatform::createViewInstance(view);
						if (content.isNotNull()) {
							content->setWindowContent(sl_true);
							ret->m_viewContent = content;
							if ([view isKindOfClass:[_priv_Slib_macOS_ViewHandle class]]) {
								((_priv_Slib_macOS_ViewHandle*)view)->m_viewInstance = Ref<macOS_ViewInstance>::from(content);
							}
						}
					}
					UIPlatform::registerWindowInstance(window, ret.get());
					return ret;
				}
			}
			return sl_null;
		}
		
		static Ref<WindowInstance> create(const WindowInstanceParam& param)
		{
			
			NSWindow* parent = nil;
			if (param.parent.isNotNull()) {
				_priv_macOS_Window* w = static_cast<_priv_macOS_Window*>(param.parent.get());
				parent = w->m_window;
			}
			
			sl_bool flagModal = param.flagModal && parent != nil;
			
			NSScreen* screen = nil;
			int styleMask = 0;
			NSRect rect;
			
			if (flagModal) {
				
				rect.origin.x = 0;
				rect.origin.y = 0;
				rect.size.width = param.size.x;
				rect.size.height = param.size.y;
				
				styleMask = NSTitledWindowMask;
				
			} else {
				
				if (param.flagBorderless || param.flagFullScreen) {
					styleMask = NSBorderlessWindowMask;
				} else {
					if (param.flagShowTitleBar){
						styleMask = NSTitledWindowMask | NSClosableWindowMask;
					} else {
						styleMask = NSClosableWindowMask;
					}
				}
				
				screen = UIPlatform::getScreenHandle(param.screen.get());
				
				UIRect screenFrame;
				Ref<Screen> _screen = param.screen;
				if (_screen.isNotNull()) {
					screenFrame = _screen->getRegion();
				} else {
					_screen = UI::getPrimaryScreen();
					if (_screen.isNotNull()) {
						screenFrame = _screen->getRegion();
					} else {
						screenFrame = UIRect::zero();
					}
				}
				
				_getNSRect(rect, param.calculateRegion(screenFrame), screenFrame.getHeight());
				
				rect = [NSWindow contentRectForFrameRect:rect styleMask:styleMask];
			}

			_priv_Slib_macOS_Window* window = [[_priv_Slib_macOS_Window alloc] initWithContentRect:rect styleMask:styleMask backing:NSBackingStoreBuffered defer:YES screen:screen];
			
			if (window != nil) {
			
				window->m_flagClosing = sl_false;
				window->m_flagModal = flagModal;
				[window setReleasedWhenClosed:NO];
				[window setContentView:[[_priv_Slib_macOS_ViewHandle alloc] init]];

				Ref<_priv_macOS_Window> ret = create(window);
				
				if (ret.isNotNull()) {
					
					ret->m_parent = parent;
					
					window->m_window = ret;
					
					{
						NSString* title = Apple::getNSStringFromString(param.title);
						[window setTitle: title];
					}
					
					if (flagModal) {
						window->m_flagModal = sl_true;
						[parent beginSheet:window completionHandler:^(NSModalResponse returnCode) {
							window->m_flagModal = sl_false;
						}];
					} else {
						if (parent != nil) {
							[parent addChildWindow:window ordered:NSWindowAbove];
						}
						[window makeKeyAndOrderFront:NSApp];
						[window setDelegate: window];
					}
					
					return ret;
				}
			}
			return sl_null;
		}
		
		void release()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				UIPlatform::removeWindowInstance(window);
			}
			m_viewContent.setNull();
			m_window = nil;
		}
		
		Ref<ViewInstance> getContentView() override
		{
			return m_viewContent;
		}
		
		void close() override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				if ([window isKindOfClass:[_priv_Slib_macOS_Window class]]) {
					_priv_Slib_macOS_Window* w = (_priv_Slib_macOS_Window*)window;
					if (w->m_flagModal) {
						w->m_flagModal = sl_false;
						NSWindow* parent = m_parent;
						if (parent != nil) {
							[parent endSheet: window];
							return;
						}
					}
					if (!(w->m_flagClosing)) {
						[w close];
					}
				}
			}
		}
		
		sl_bool isClosed() override
		{
			return m_window == nil;
		}
		
		Ref<WindowInstance> getParent()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSWindow* parent = m_parent;
				if (parent != nil) {
					return UIPlatform::createWindowInstance(parent);
				}
			}
			return sl_null;
		}
		
		sl_bool setParent(const Ref<WindowInstance>& windowInst) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				if (windowInst.isNotNull()) {
					_priv_macOS_Window* w = static_cast<_priv_macOS_Window*>(windowInst.get());
					NSWindow* p = w->m_window;
					m_parent = p;
					if (p != nil) {
						[p addChildWindow:window ordered:NSWindowAbove];
						return sl_true;
					}
				} else {
					NSWindow* p = [window parentWindow];
					if (p != nil) {
						[p removeChildWindow:window];
					}
					m_parent = nil;
					return sl_true;
				}
			}
			return sl_false;
		}
		
		sl_bool setFocus() override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				[window makeKeyAndOrderFront:NSApp];
				return sl_true;
			}
			return sl_false;
		}
		
		UIRect getFrame() override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect rect = [window frame];
				UIRect frame;
				_getFrame(frame, rect, m_heightScreen);
				return frame;
			} else {
				return UIRect::zero();
			}
		}
		
		sl_bool setFrame(const UIRect& frame) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect rect;
				_getNSRect(rect, frame, m_heightScreen);
				if ([NSThread isMainThread]) {
					[window setFrame:rect display:TRUE];
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						[window setFrame:rect display:TRUE];
					});
				}
				return sl_true;
			}
			return sl_false;
		}
		
		UIRect getClientFrame() override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect fr = [window frame];
				NSRect cr = [window contentRectForFrameRect:fr];
				UIRect frame;
				_getFrame(frame, cr, m_heightScreen);
				return frame;
			} else {
				return UIRect::zero();
			}
		}
		
		UISize getClientSize() override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect rect = [window contentRectForFrameRect:[window frame]];
				return UISize((sl_ui_pos)(rect.size.width), (sl_ui_pos)(rect.size.height));
			} else {
				return UISize::zero();
			}
		}
		
		sl_bool setClientSize(const UISize& _size) override
		{
			UISize size = _size;
			NSWindow* window = m_window;
			if (window != nil) {
				NSSize s;
				s.width = (CGFloat)(size.x);
				s.height = (CGFloat)(size.y);
				if ([NSThread isMainThread]) {
					[window setContentSize:s];
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						[window setContentSize:s];
					});
				}
				return sl_true;
			}
			return sl_false;
		}
		
		String getTitle()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSString* title = [window title];
				return Apple::getStringFromNSString(title);
			}
			return sl_null;
		}
		
		sl_bool setTitle(const String& title) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSString* ntitle = Apple::getNSStringFromString(title);
				[window setTitle: ntitle];
				return sl_true;
			}
			return sl_false;
		}
		
		Color getBackgroundColor()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSColor* color = [window backgroundColor];
				if (color == [NSColor windowBackgroundColor]) {
					return Color::zero();
				}
				return GraphicsPlatform::getColorFromNSColor(color);
			}
			return Color::Transparent;
		}
		
		sl_bool setBackgroundColor(const Color& _color) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSColor* color;
				if (_color.isZero()) {
					color = [NSColor windowBackgroundColor];
				} else {
					color = GraphicsPlatform::getNSColorFromColor(_color);
				}
				if ([NSThread isMainThread]) {
					[window setBackgroundColor:color];
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						[window setBackgroundColor:color];
					});
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool isMinimized() override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				BOOL flag = [window isMiniaturized];
				if (flag) {
					return sl_true;
				} else {
					return sl_false;
				}
			} else {
				return sl_false;
			}
		}
		
		sl_bool setMinimized(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				if ([NSThread isMainThread]) {
					sl_bool f1 = [window isMiniaturized] ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						if (f2) {
							[window miniaturize:nil];
						} else {
							[window deminiaturize:nil];
						}
					}
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						sl_bool f1 = [window isMiniaturized] ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							if (f2) {
								[window miniaturize:nil];
							} else {
								[window deminiaturize:nil];
							}
						}
					});
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool isMaximized() override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				BOOL flag = [window isZoomed];
				if (flag) {
					return sl_true;
				} else {
					return sl_false;
				}
			} else {
				return sl_false;
			}
		}
		
		sl_bool setMaximized(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				if ([NSThread isMainThread]) {
					sl_bool f1 = [window isZoomed] ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						[window zoom:nil];
					}
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						sl_bool f1 = [window isZoomed] ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							[window zoom:nil];
						}
					});
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool isVisible()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				BOOL flag = [window isVisible];
				if (flag) {
					return sl_true;
				} else {
					return sl_false;
				}
			} else {
				return sl_false;
			}
		}
		
		sl_bool setVisible(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				if ([NSThread isMainThread]) {
					if (flag) {
						NSWindow* parent = m_parent;
						if (parent != nil) {
							[parent addChildWindow:window ordered:NSWindowAbove];
						} else {
							[window orderFrontRegardless];
						}
					} else {
						[window orderOut:nil];
					}
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						if (flag) {
							NSWindow* parent = m_parent;
							if (parent != nil) {
								[parent addChildWindow:window ordered:NSWindowAbove];
							} else {
								[window orderFrontRegardless];
							}
						} else {
							[window orderOut:nil];
						}
					});
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool isAlwaysOnTop()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSInteger level = [window level];
				if (level == NSNormalWindowLevel) {
					return sl_false;
				} else {
					return sl_true;
				}
			}
			return sl_false;
		}
		
		sl_bool setAlwaysOnTop(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				if ([NSThread isMainThread]) {
					if (flag) {
						[window setLevel:NSFloatingWindowLevel];
					} else {
						[window setLevel:NSNormalWindowLevel];
					}
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						if (flag) {
							[window setLevel:NSFloatingWindowLevel];
						} else {
							[window setLevel:NSNormalWindowLevel];
						}
					});
				}
			}
			return sl_false;
		}
		
		sl_bool isCloseButtonEnabled()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				if (!(style & NSTitledWindowMask)) {
					return sl_false;
				}
				sl_bool f = (style & NSClosableWindowMask) ? sl_true : sl_false;
				return f;
			}
			return sl_false;
		}
		
		sl_bool setCloseButtonEnabled(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				if (!(style & NSTitledWindowMask)) {
					return sl_false;
				}
				sl_bool f1 = (style & NSClosableWindowMask) ? sl_true : sl_false;
				sl_bool f2 = flag ? sl_true : sl_false;
				if (f1 != f2) {
					if (flag) {
						style = style | NSClosableWindowMask;
					} else {
						style = style & (~NSClosableWindowMask);
					}
					if ([NSThread isMainThread]) {
						[window setStyleMask:style];
					} else {
						dispatch_async(dispatch_get_main_queue(), ^{
							[window setStyleMask:style];
						});
					}
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool isMinimizeButtonEnabled()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				if (!(style & NSTitledWindowMask)) {
					return sl_false;
				}
				sl_bool f = (style & NSMiniaturizableWindowMask) ? sl_true : sl_false;
				return f;
			}
			return sl_false;
		}
		
		sl_bool setMinimizeButtonEnabled(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				if (!(style & NSTitledWindowMask)) {
					return sl_false;
				}
				sl_bool f1 = (style & NSMiniaturizableWindowMask) ? sl_true : sl_false;
				sl_bool f2 = flag ? sl_true : sl_false;
				if (f1 != f2) {
					if (flag) {
						style = style | NSMiniaturizableWindowMask;
					} else {
						style = style & (~NSMiniaturizableWindowMask);
					}
					if ([NSThread isMainThread]) {
						[window setStyleMask:style];
					} else {
						dispatch_async(dispatch_get_main_queue(), ^{
							[window setStyleMask:style];
						});
					}
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool isMaximizeButtonEnabled()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				NSButton* buttonZoom = [window standardWindowButton:NSWindowZoomButton];
				sl_bool f = [buttonZoom isEnabled] ? sl_true : sl_false;
				return f;
			}
			return sl_false;
		}
		
		sl_bool setMaximizeButtonEnabled(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				NSButton* buttonZoom = [window standardWindowButton:NSWindowZoomButton];
				sl_bool f1 = [buttonZoom isEnabled] ? sl_true : sl_false;
				sl_bool f2 = flag ? sl_true : sl_false;
				if (f1 != f2) {
					if ([NSThread isMainThread]) {
						[buttonZoom setEnabled:flag];
					} else {
						dispatch_async(dispatch_get_main_queue(), ^{
							[buttonZoom setEnabled:flag];
						});
					}
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool isResizable()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				sl_bool f = (style & NSResizableWindowMask) ? sl_true : sl_false;
				return f;
			}
			return sl_false;
		}

		sl_bool setResizable(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSUInteger style = [window styleMask];
				if (style & NSBorderlessWindowMask) {
					return sl_false;
				}
				sl_bool f1 = (style & NSResizableWindowMask) ? sl_true : sl_false;
				sl_bool f2 = flag ? sl_true : sl_false;
				if (f1 != f2) {
					if (flag) {
						style = style | NSResizableWindowMask;
					} else {
						style = style & (~NSResizableWindowMask);
						if (style == 0) {
							style = NSClosableWindowMask;
						}
					}
					if ([NSThread isMainThread]) {
						[window setStyleMask:style];
					} else {
						dispatch_async(dispatch_get_main_queue(), ^{
							[window setStyleMask:style];
						});
					}
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_real getAlpha()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				sl_real alpha = (sl_real)([window alphaValue]);
				return alpha;
			}
			return 1;
		}
		
		sl_bool setAlpha(sl_real _alpha) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				sl_real alpha = _alpha;
				if (alpha < 0) {
					alpha = 0;
				}
				if (alpha > 1) {
					alpha = 1;
				}
				if ([NSThread isMainThread]) {
					[window setAlphaValue:alpha];
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						[window setAlphaValue:alpha];
					});
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool isTransparent()
		{
			NSWindow* window = m_window;
			if (window != nil) {
				BOOL f = [window ignoresMouseEvents];
				return f ? sl_true : sl_false;
			}
			return sl_false;
		}
		
		sl_bool setTransparent(sl_bool flag) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				[window setIgnoresMouseEvents:(flag?TRUE:FALSE)];
				return sl_true;
			}
			return sl_false;
		}
		
		
		UIPointf convertCoordinateFromScreenToWindow(const UIPointf& ptScreen) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect rect;
				rect.origin.x = (CGFloat)(ptScreen.x);
				rect.origin.y = (CGFloat)(m_heightScreen - ptScreen.y);
				rect.size.width = 0;
				rect.size.height = 0;
				rect = [window convertRectFromScreen:rect];
				NSRect frame = [window frame];
				UIPointf ret;
				ret.x = (sl_ui_pos)(rect.origin.x);
				ret.y = (sl_ui_pos)(frame.size.height - rect.origin.y);
				return ret;
			} else {
				return ptScreen;
			}
		}
		
		UIPointf convertCoordinateFromWindowToScreen(const UIPointf& ptWindow) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect frame = [window frame];
				NSRect rect;
				rect.origin.x = (CGFloat)(ptWindow.x);
				rect.origin.y = (CGFloat)(frame.size.height - ptWindow.y);
				rect.size.width = 0;
				rect.size.height = 0;
				rect = [window convertRectToScreen:rect];
				UIPointf ret;
				ret.x = (sl_ui_posf)(rect.origin.x);
				ret.y = (sl_ui_posf)(m_heightScreen - rect.origin.y);
				return ret;
			} else {
				return ptWindow;
			}
		}
		
		UIPointf convertCoordinateFromScreenToClient(const UIPointf& ptScreen) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect rect;
				rect.origin.x = (CGFloat)(ptScreen.x);
				rect.origin.y = (CGFloat)(m_heightScreen - ptScreen.y);
				rect.size.width = 0;
				rect.size.height = 0;
				rect = [window convertRectFromScreen:rect];
				NSRect client = [window contentRectForFrameRect:[window frame]];
				UIPointf ret;
				ret.x = (sl_ui_posf)(rect.origin.x);
				ret.y = (sl_ui_posf)(client.size.height - rect.origin.y);
				return ret;
			} else {
				return ptScreen;
			}
		}
		
		UIPointf convertCoordinateFromClientToScreen(const UIPointf& ptClient) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect client = [window contentRectForFrameRect:[window frame]];
				NSRect rect;
				rect.origin.x = (CGFloat)(ptClient.x);
				rect.origin.y = (CGFloat)(client.size.height - ptClient.y);
				rect.size.width = 0;
				rect.size.height = 0;
				rect = [window convertRectToScreen:rect];
				UIPointf ret;
				ret.x = (sl_ui_posf)(rect.origin.x);
				ret.y = (sl_ui_posf)(m_heightScreen - rect.origin.y);
				return ret;
			} else {
				return ptClient;
			}
		}
		
		UIPointf convertCoordinateFromWindowToClient(const UIPointf& ptWindow) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect frame = [window frame];
				NSRect client = [window contentRectForFrameRect:frame];
				UIPointf ret;
				ret.x = (sl_ui_posf)(ptWindow.x);
				ret.y = (sl_ui_posf)(client.size.height - (frame.size.height - ptWindow.y));
				return ret;
			} else {
				return ptWindow;
			}
		}
		
		UIPointf convertCoordinateFromClientToWindow(const UIPointf& ptClient) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect frame = [window frame];
				NSRect client = [window contentRectForFrameRect:frame];
				UIPointf ret;
				ret.x = (sl_ui_posf)(ptClient.x);
				ret.y = (sl_ui_posf)(frame.size.height - (client.size.height - ptClient.y));
				return ret;
			} else {
				return ptClient;
			}
		}
		
		UISize getWindowSizeFromClientSize(const UISize& sizeClient) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect frame = [window frame];
				NSRect client = [window contentRectForFrameRect:frame];
				UISize ret;
				ret.x = sizeClient.x + (sl_ui_pos)(frame.size.width - client.size.width);
				ret.y = sizeClient.y + (sl_ui_pos)(frame.size.height - client.size.height);
				if (ret.x < 0) {
					ret.x = 0;
				}
				if (ret.y < 0) {
					ret.y = 0;
				}
				return ret;
			} else {
				return sizeClient;
			}
		}
		
		UISize getClientSizeFromWindowSize(const UISize& sizeWindow) override
		{
			NSWindow* window = m_window;
			if (window != nil) {
				NSRect frame = [window frame];
				NSRect client = [window contentRectForFrameRect:frame];
				UISize ret;
				ret.x = sizeWindow.x - (sl_ui_pos)(frame.size.width - client.size.width);
				ret.y = sizeWindow.y - (sl_ui_pos)(frame.size.height - client.size.height);
				if (ret.x < 0) {
					ret.x = 0;
				}
				if (ret.y < 0) {
					ret.y = 0;
				}
				return ret;
			} else {
				return sizeWindow;
			}
		}
		
	private:
		static void _applyRectLimit(NSRect& rect)
		{
			if (rect.origin.x < -16000.0f) {
				rect.origin.x = -16000.0f;
			}
			if (rect.origin.x > 16000.0f) {
				rect.origin.x = 16000.0f;
			}
			if (rect.origin.y < -16000.0f) {
				rect.origin.y = -16000.0f;
			}
			if (rect.origin.y > 16000.0f) {
				rect.origin.y = 16000.0f;
			}
			if (rect.size.width < 0.0f) {
				rect.size.width = 0.0f;
			}
			if (rect.size.width > 10000.0f) {
				rect.size.width = 10000.0f;
			}
			if (rect.size.height < 0.0f) {
				rect.size.height = 0.0f;
			}
			if (rect.size.height > 10000.0f) {
				rect.size.height = 10000.0f;
			}
		}
		
		static void _getNSRect(NSRect& rectOut, const UIRect& rectIn, sl_ui_len heightScreen)
		{
			rectOut.origin.x = (int)(rectIn.left);
			rectOut.origin.y = (int)(heightScreen - rectIn.bottom);
			rectOut.size.width = (int)(rectIn.getWidth());
			rectOut.size.height = (int)(rectIn.getHeight());
			_applyRectLimit(rectOut);
		}
		
		static void _getFrame(UIRect& rectOut, const NSRect& rectIn, sl_ui_len heightScreen)
		{
			rectOut.left = (sl_ui_pos)(rectIn.origin.x);
			rectOut.top = heightScreen - (sl_ui_pos)(rectIn.origin.y + rectIn.size.height);
			rectOut.setWidth((sl_ui_pos)(rectIn.size.width));
			rectOut.setHeight((sl_ui_pos)(rectIn.size.height));
			rectOut.fixSizeError();
		}
		
	};

	Ref<WindowInstance> Window::createWindowInstance(const WindowInstanceParam& param)
	{
		return _priv_macOS_Window::create(param);
	}

}

@implementation _priv_Slib_macOS_Window

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)windowShouldClose:(id)sender
{
	BOOL ret = YES;
	m_flagClosing = sl_true;
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		if (window->onClose()) {
			window->close();
			ret = YES;
		} else {
			ret = NO;
		}
	}
	m_flagClosing = sl_false;
	return ret;
}

- (void)windowWillClose:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		slib::UI::dispatchToUiThread(SLIB_FUNCTION_REF(slib::_priv_macOS_Window, release, window));
	}
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		window->onActivate();
	}
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		window->onDeactivate();
	}
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		slib::UISize size((sl_ui_pos)(frameSize.width), (sl_ui_pos)(frameSize.height));
		if (size.x < 0) {
			size.x = 0;
		}
		if (size.y < 0) {
			size.y = 0;
		}
		window->onResizing(size);
		frameSize.width = (CGFloat)(size.x);
		frameSize.height = (CGFloat)(size.y);
	}
	return frameSize;
}

- (void)windowDidResize:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		NSRect fr = [self frame];
		NSRect cr = [self contentRectForFrameRect:fr];
		NSSize size = cr.size;
		window->onResize((sl_ui_len)(size.width), (sl_ui_len)(size.height));
	}
}

- (void)windowDidMove:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		window->onMove();
	}
}

- (void)windowWillMiniaturize:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		window->onMinimize();
	}
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		window->onDeminimize();
	}
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		window->onMaximize();
	}
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	slib::Ref<slib::_priv_macOS_Window> window = m_window;
	if (window.isNotNull()) {
		window->onDemaximize();
	}
}

- (BOOL)acceptsFirstResponder
{
	return TRUE;
}

- (void)sendEvent:(NSEvent *)event
{
	if (event.type == NSEventTypeKeyDown) {
		id view = [self firstResponder];
		if ([view isKindOfClass:[NSTextView class]]) {
			// Find NSTextField
			NSView* t = view;
			t = t.superview;
			if (!([t isKindOfClass:[NSTextField class]])) {
				t = t.superview;
			}
			if ([t isKindOfClass:[NSTextField class]]) {
				int c = event.keyCode;
				// Tab, Return, Escape
				if (c == 0x30 || c == 0x24 || c == 0x35) {
					// NSTextField can't get keyDown event, so we manually invoke this event
					[t keyDown:event];
					return;
				}
			}
		}
	}
	[super sendEvent:event];
}

@end

namespace slib
{

	Ref<WindowInstance> UIPlatform::createWindowInstance(NSWindow* window)
	{
		Ref<WindowInstance> ret = UIPlatform::_getWindowInstance((__bridge void*)window);
		if (ret.isNotNull()) {
			return ret;
		}
		return _priv_macOS_Window::create(window);
	}

	void UIPlatform::registerWindowInstance(NSWindow* window, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance((__bridge void*)window, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(NSWindow* window)
	{
		return UIPlatform::_getWindowInstance((__bridge void*)window);
	}

	void UIPlatform::removeWindowInstance(NSWindow* window)
	{
		UIPlatform::_removeWindowInstance((__bridge void*)window);
	}

	NSWindow* UIPlatform::getWindowHandle(WindowInstance* instance)
	{
		_priv_macOS_Window* window = (_priv_macOS_Window*)instance;
		if (window) {
			return window->m_window;
		} else {
			return nil;
		}
	}

	NSWindow* UIPlatform::getWindowHandle(Window* window)
	{
		if (window) {
			Ref<WindowInstance> instance = window->getWindowInstance();
			if (instance.isNotNull()) {
				_priv_macOS_Window* _instance = (_priv_macOS_Window*)(instance.get());
				if (_instance) {
					return _instance->m_window;
				}
			}
		}
		return nil;
	}

}

#endif
