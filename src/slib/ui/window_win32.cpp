/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/core.h"
#include "slib/ui/screen.h"
#include "slib/ui/window.h"
#include "slib/ui/platform.h"

#include "view_win32.h"

namespace slib
{

	namespace priv
	{

		namespace ui_core
		{
			void RunLoop(HWND hWndModalDialog);
		}

		namespace window
		{

			class Win32_WindowInstance : public WindowInstance
			{
			public:
				HWND m_handle;

				sl_bool m_flagMinimized;
				sl_bool m_flagMaximized;

				AtomicRef<ViewInstance> m_viewContent;
				sl_bool m_flagDestroyOnRelease;

				Color m_backgroundColor;

			public:
				Win32_WindowInstance()
				{
					m_flagMinimized = sl_false;
					m_flagMaximized = sl_false;

					m_backgroundColor = Color::zero();
				}

				~Win32_WindowInstance()
				{
					close();
				}

			public:
				static Ref<Win32_WindowInstance> create(HWND hWnd, sl_bool flagDestroyOnRelease)
				{
					Ref<Win32_WindowInstance> ret;
					if (hWnd) {
						ret = new Win32_WindowInstance();
						if (ret.isNotNull()) {
							ret->m_handle = hWnd;
							ret->m_flagDestroyOnRelease = flagDestroyOnRelease;
							Ref<ViewInstance> content = UIPlatform::createViewInstance(hWnd, sl_false);
							if (content.isNotNull()) {
								content->setWindowContent(sl_true);
							}
							ret->m_viewContent = content;
							UIPlatform::registerWindowInstance(hWnd, ret.get());
							return ret;
						}
						if (flagDestroyOnRelease) {
							PostMessageW(hWnd, SLIB_UI_MESSAGE_CLOSE, 0, 0);
						}
					}
					return ret;
				}

				static HWND createHandle(const WindowInstanceParam& param)
				{
					Win32_UI_Shared* shared = Win32_UI_Shared::get();
					if (!shared) {
						return NULL;
					}

					HINSTANCE hInst = GetModuleHandleW(NULL);

					HWND hParent = NULL;
					if (param.parent.isNotNull()) {
						Win32_WindowInstance* w = static_cast<Win32_WindowInstance*>(param.parent.get());
						hParent = w->m_handle;
					}

					// create handle
					HWND hWnd;
					{
						DWORD style = WS_CLIPCHILDREN;
						DWORD styleEx = WS_EX_CONTROLPARENT;
						if (param.flagBorderless || param.flagFullScreen) {
							style |= WS_POPUP;
						} else {
							if (param.flagShowTitleBar){
								if (param.flagDialog) {
									style |= (WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_BORDER);
									styleEx |= WS_EX_DLGMODALFRAME;
								} else {
									style |= (WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION);
								}
							} else {
								style |= (WS_POPUP | WS_BORDER);
							}
						}

						UIRect frameScreen;
						if (param.screen.isNotNull()) {
							frameScreen = param.screen->getRegion();
						} else {
							frameScreen = UI::getScreenRegion();
						}
						UIRect frameWindow = param.calculateRegion(frameScreen);
						String16 title = param.title;
						hWnd = CreateWindowExW(
							styleEx, // ex-style
							(LPCWSTR)((LONG_PTR)(shared->wndClassForWindow)),
							(LPCWSTR)(title.getData()),
							style,
							(int)(frameWindow.left), (int)(frameWindow.top),
							(int)(frameWindow.getWidth()), (int)(frameWindow.getHeight()),
							hParent, // parent
							UIPlatform::getMenuHandle(param.menu), // menu
							hInst,
							NULL);						
					}
					return hWnd;
				}

				void close() override
				{
					ObjectLocker lock(this);
					HWND hWnd = m_handle;
					if (hWnd) {
						m_handle = NULL;
						UIPlatform::removeWindowInstance(hWnd);
						m_viewContent.setNull();
						if (m_flagDestroyOnRelease) {
							PostMessageW(hWnd, SLIB_UI_MESSAGE_CLOSE, 0, 0);
						}
					}
				}

				sl_bool isClosed() override
				{
					return m_handle == NULL;
				}

				sl_bool setParent(const Ref<WindowInstance>& window) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						if (window.isNotNull()) {
							Win32_WindowInstance* w = static_cast<Win32_WindowInstance*>(window.get());
							HWND hWndParent = w->m_handle;
							if (hWndParent) {
								SetWindowLongPtrW(hWnd, GWLP_HWNDPARENT, (LONG_PTR)hWndParent);
								return sl_true;
							}
						} else {
							SetWindowLongPtrW(hWnd, GWLP_HWNDPARENT, (LONG_PTR)NULL);
							return sl_true;
						}
					}
					return sl_false;
				}

				Ref<ViewInstance> getContentView() override
				{
					return m_viewContent;
				}

				void setMenu(const Ref<Menu>& menu) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						SetMenu(hWnd, UIPlatform::getMenuHandle(menu));
					}
				}

				sl_bool isActive() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						return hWnd == GetActiveWindow();
					}
					return sl_false;
				}

				sl_bool activate() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						SetForegroundWindow(hWnd);
						return sl_true;
					}
					return sl_false;
				}

				UIRect getFrame() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rect;
						GetWindowRect(hWnd, &rect);
						return UIRect((sl_ui_pos)(rect.left), (sl_ui_pos)(rect.top), (sl_ui_pos)(rect.right), (sl_ui_pos)(rect.bottom));
					} else {
						return UIRect::zero();
					}
				}

				sl_bool setFrame(const UIRect& frame) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						SetWindowPos(hWnd, NULL,
							(int)(frame.left), (int)(frame.top),
							(int)(frame.getWidth()), (int)(frame.getHeight()),
							SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						return sl_true;
					}
					return sl_false;
				}

				UIRect getClientFrame() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						UIRect ret;
						RECT rect;
						GetClientRect(hWnd, &rect);
						POINT pt;
						pt.x = 0;
						pt.y = 0;
						ClientToScreen(hWnd, &pt);
						ret.left = (sl_ui_pos)(pt.x);
						ret.top = (sl_ui_pos)(pt.y);
						pt.x = rect.right;
						pt.y = rect.bottom;
						ClientToScreen(hWnd, &pt);
						ret.right = (sl_ui_pos)(pt.x);
						ret.bottom = (sl_ui_pos)(pt.y);
						return ret;
					} else {
						return UIRect::zero();
					}
				}

				UISize getClientSize() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rect;
						GetClientRect(hWnd, &rect);
						return UISize((sl_ui_pos)(rect.right), (sl_ui_pos)(rect.bottom));
					} else {
						return UISize::zero();
					}
				}

				sl_bool setClientSize(const UISize& size) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rectClient;
						GetClientRect(hWnd, &rectClient);
						RECT rectWindow;
						GetWindowRect(hWnd, &rectWindow);
						int dx = rectWindow.right - rectWindow.left - rectClient.right;
						int dy = rectWindow.bottom - rectWindow.top - rectClient.bottom;
						SetWindowPos(hWnd, NULL,
							0, 0,
							dx + (int)(size.x), dy + (int)(size.y),
							SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setTitle(const String& title) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						Windows::setWindowText(hWnd, title);
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setBackgroundColor(const Color& color) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						m_backgroundColor = color;
						InvalidateRect(hWnd, NULL, TRUE);
					}
					return sl_false;
				}

				sl_bool isMinimized() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						BOOL flag = IsIconic(hWnd);
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
					HWND hWnd = m_handle;
					if (hWnd) {
						sl_bool f1 = IsIconic(hWnd) ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							if (f2) {
								ShowWindowAsync(hWnd, SW_FORCEMINIMIZE);
							} else {
								ShowWindowAsync(hWnd, SW_RESTORE);
							}
						}
						return sl_true;
					}
					return sl_false;
				}

				sl_bool isMaximized() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						BOOL flag = IsZoomed(hWnd);
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
					HWND hWnd = m_handle;
					if (hWnd) {
						sl_bool f1 = IsZoomed(hWnd) ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							if (f2) {
								ShowWindowAsync(hWnd, SW_MAXIMIZE);
							} else {
								ShowWindowAsync(hWnd, SW_RESTORE);
							}
						}
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setVisible(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						sl_bool f1 = IsWindowVisible(hWnd) ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							if (f2) {
								ShowWindowAsync(hWnd, SW_SHOW);
							} else {
								ShowWindowAsync(hWnd, SW_HIDE);
							}
						}
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setAlwaysOnTop(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						if (flag) {
							SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
								SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						} else {
							SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
								SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						}
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setCloseButtonEnabled(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						LONG old = GetClassLongW(hWnd, GCL_STYLE);
						if (flag) {
							SetClassLongW(hWnd, GCL_STYLE, old & (~CS_NOCLOSE));
						} else {
							SetClassLongW(hWnd, GCL_STYLE, old | CS_NOCLOSE);
						}
						SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
							SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setMinimizeButtonEnabled(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						LONG old = GetWindowLongW(hWnd, GWL_STYLE);
						if (flag) {
							SetWindowLongW(hWnd, GWL_STYLE, old | WS_MINIMIZEBOX);
						} else {
							SetWindowLongW(hWnd, GWL_STYLE, old & (~WS_MINIMIZEBOX));
						}
						SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
							SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setMaximizeButtonEnabled(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						LONG old = GetWindowLongW(hWnd, GWL_STYLE);
						if (flag) {
							SetWindowLongW(hWnd, GWL_STYLE, old | WS_MAXIMIZEBOX);
						} else {
							SetWindowLongW(hWnd, GWL_STYLE, old & (~WS_MAXIMIZEBOX));
						}
						SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
							SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setResizable(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						LONG old = GetWindowLongW(hWnd, GWL_STYLE);
						if (!(old & WS_BORDER)) {
							return sl_false;
						}
						if (flag) {
							SetWindowLongW(hWnd, GWL_STYLE, old | WS_THICKFRAME);
						} else {
							SetWindowLongW(hWnd, GWL_STYLE, old & (~WS_THICKFRAME));
						}
						SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
							SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setAlpha(sl_real _alpha) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						sl_real alpha = _alpha * 255;
						if (alpha < 0) {
							alpha = 0;
						}
						if (alpha > 255) {
							alpha = 255;
						}
						int a = (int)alpha;
						LONG old = GetWindowLongW(hWnd, GWL_EXSTYLE);
						sl_bool flagChangeStyle = sl_false;
						if (a >= 255) {
							if (old & WS_EX_LAYERED) {
								SetWindowLongW(hWnd, GWL_EXSTYLE, old & (~WS_EX_LAYERED));
								flagChangeStyle = sl_true;
							}
						} else {
							if (!(old & WS_EX_LAYERED)) {
								SetWindowLongW(hWnd, GWL_EXSTYLE, old | WS_EX_LAYERED);
								flagChangeStyle = sl_true;
							}
							SetLayeredWindowAttributes(hWnd, 0, a, LWA_ALPHA);
						}
						if (flagChangeStyle) {
							RedrawWindow(hWnd,
								NULL,
								NULL,
								RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
						}
						return sl_true;
					}
					return sl_false;
				}

				sl_bool setTransparent(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						LONG old = GetWindowLongW(hWnd, GWL_EXSTYLE);
						if (flag) {
							SetWindowLongW(hWnd, GWL_EXSTYLE, old | WS_EX_TRANSPARENT);
						} else {
							SetWindowLongW(hWnd, GWL_EXSTYLE, old & (~WS_EX_TRANSPARENT));
						}
						SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
							SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
						return sl_true;
					}
					return sl_false;
				}

				UIPointf convertCoordinateFromScreenToWindow(const UIPointf& ptScreen) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rect;
						GetWindowRect(hWnd, &rect);
						return UIPointf(ptScreen.x - (sl_ui_posf)(rect.left), ptScreen.y - (sl_ui_posf)(rect.top));
					} else {
						return ptScreen;
					}
				}

				UIPointf convertCoordinateFromWindowToScreen(const UIPointf& ptWindow) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rect;
						GetWindowRect(hWnd, &rect);
						return UIPointf(ptWindow.x + (sl_ui_posf)(rect.left), ptWindow.y + (sl_ui_posf)(rect.top));
					} else {
						return ptWindow;
					}
				}

				UIPointf convertCoordinateFromScreenToClient(const UIPointf& ptScreen) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						POINT pt;
						pt.x = (int)(ptScreen.x);
						pt.y = (int)(ptScreen.y);
						ScreenToClient(hWnd, &pt);
						return UIPointf((sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y));
					} else {
						return ptScreen;
					}
				}

				UIPointf convertCoordinateFromClientToScreen(const UIPointf& ptClient) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						POINT pt;
						pt.x = (int)(ptClient.x);
						pt.y = (int)(ptClient.y);
						ClientToScreen(hWnd, &pt);
						return UIPointf((sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y));
					} else {
						return ptClient;
					}
				}

				UIPointf convertCoordinateFromWindowToClient(const UIPointf& ptWindow) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rect;
						GetWindowRect(hWnd, &rect);
						POINT pt;
						pt.x = (int)(ptWindow.x) + rect.left;
						pt.y = (int)(ptWindow.y) + rect.top;
						ScreenToClient(hWnd, &pt);
						return UIPointf((sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y));
					} else {
						return ptWindow;
					}
				}

				UIPointf convertCoordinateFromClientToWindow(const UIPointf& ptClient) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						POINT pt;
						pt.x = (int)(ptClient.x);
						pt.y = (int)(ptClient.y);
						ClientToScreen(hWnd, &pt);
						RECT rect;
						GetWindowRect(hWnd, &rect);
						return UIPointf((sl_ui_posf)(pt.x - rect.left), (sl_ui_posf)(pt.y - rect.top));
					} else {
						return ptClient;
					}
				}

				UISize getWindowSizeFromClientSize(const UISize& size) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rectClient;
						GetClientRect(hWnd, &rectClient);
						RECT rectWindow;
						GetWindowRect(hWnd, &rectWindow);
						UISize ret((sl_ui_pos)(rectWindow.right - rectWindow.left - rectClient.right + size.x),
							(sl_ui_pos)(rectWindow.bottom - rectWindow.top - rectClient.bottom + size.y));
						if (ret.x < 0) {
							ret.x = 0;
						}
						if (ret.y < 0) {
							ret.y = 0;
						}
						return ret;
					} else {
						return size;
					}
				}

				UISize getClientSizeFromWindowSize(const UISize& size) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rectClient;
						GetClientRect(hWnd, &rectClient);
						RECT rectWindow;
						GetWindowRect(hWnd, &rectWindow);
						UISize ret(size.x - (sl_ui_pos)(rectWindow.right - rectWindow.left - rectClient.right),
							size.y - (sl_ui_pos)(rectWindow.bottom - rectWindow.top - rectClient.bottom));
						if (ret.x < 0) {
							ret.x = 0;
						}
						if (ret.y < 0) {
							ret.y = 0;
						}
						return ret;
					} else {
						return size;
					}
				}

				sl_bool doModal() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						HWND hWndParent = Windows::getOwnerWindow(hWnd);
						sl_bool flagEnableParent = sl_false;
						if (hWndParent) {
							if (hWndParent != GetDesktopWindow() && IsWindowEnabled(hWndParent)) {
								EnableWindow(hWndParent, FALSE);
								flagEnableParent = sl_true;
							}
						}
						ui_core::RunLoop(hWnd);
						if (flagEnableParent) {
							EnableWindow(hWndParent, TRUE);
							SetForegroundWindow(hWndParent);
						}
					}
					return sl_true;
				}

			};


			LRESULT CALLBACK WindowInstanceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				Ref<WindowInstance> _window = UIPlatform::getWindowInstance(hWnd);
				Win32_WindowInstance* window = (Win32_WindowInstance*)(_window.get());
				if (window && window->m_handle) {
					switch (uMsg) {
					case WM_CLOSE:
						{
							if (window->onClose()) {
								window->close();
							}
							return 1;
						}
					case WM_ACTIVATE:
						{
							if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE) {
								window->onActivate();
							} else {
								window->onDeactivate();
							}
							break;
						}
					case WM_SIZE:
						{
							int width = LOWORD(lParam);
							int height = HIWORD(lParam);
							if (width < 0) {
								width = 0;
							}
							if (height < 0) {
								height = 0;
							}
							if (width > 60000) {
								width = 60000;
							}
							if (height > 60000) {
								height = 60000;
							}
							if (wParam == SIZE_MAXIMIZED) {
								window->m_flagMaximized = sl_true;
								window->onMaximize();
								window->onResize((sl_ui_pos)width, (sl_ui_pos)height);
							} else if (wParam == SIZE_MINIMIZED) {
								window->m_flagMinimized = sl_true;
								window->onMinimize();
							} else if (wParam == SIZE_RESTORED) {
								if (window->m_flagMinimized) {
									window->m_flagMinimized = sl_false;
									window->onDeminimize();
								} else if (window->m_flagMaximized) {
									window->m_flagMaximized = sl_false;
									window->onDemaximize();
									window->onResize((sl_ui_pos)width, (sl_ui_pos)height);
								} else {
									window->onResize((sl_ui_pos)width, (sl_ui_pos)height);
								}
							}
							break;
						}
					case WM_SIZING:
						{
							RECT& rect = *(RECT*)(lParam);
							UISize size((sl_ui_pos)(rect.right - rect.left), (sl_ui_pos)(rect.bottom - rect.top));
							window->onResizing(size, wParam != WMSZ_TOP && wParam != WMSZ_BOTTOM);
							if (size.x < 0) {
								size.x = 0;
							}
							if (size.y < 0) {
								size.y = 0;
							}
							if (size.x > 60000) {
								size.x = 60000;
							}
							if (size.y > 60000) {
								size.y = 60000;
							}
							switch (wParam) {
							case WMSZ_TOPLEFT:
								rect.left = (int)(rect.right - size.x);
								rect.top = (int)(rect.bottom - size.y);
								break;
							case WMSZ_TOP:
								rect.right = (int)(rect.left + size.x);
								rect.top = (int)(rect.bottom - size.y);
								break;
							case WMSZ_TOPRIGHT:
								rect.right = (int)(rect.left + size.x);
								rect.top = (int)(rect.bottom - size.y);
								break;
							case WMSZ_LEFT:
								rect.left = (int)(rect.right - size.x);
								rect.bottom = (int)(rect.top + size.y);
								break;
							case WMSZ_RIGHT:
								rect.right = (int)(rect.left + size.x);
								rect.bottom = (int)(rect.top + size.y);
								break;
							case WMSZ_BOTTOMLEFT:
								rect.left = (int)(rect.right - size.x);
								rect.bottom = (int)(rect.top + size.y);
								break;
							case WMSZ_BOTTOM:
								rect.right = (int)(rect.left + size.x);
								rect.bottom = (int)(rect.top + size.y);
								break;
							case WMSZ_BOTTOMRIGHT:
								rect.right = (int)(rect.left + size.x);
								rect.bottom = (int)(rect.top + size.y);
								break;
							}
							break;
						}
						break;
					case WM_MOVE:
						{
							window->onMove();
							break;
						}
					case WM_ERASEBKGND:
						{
							Ref<ViewInstance> contentInst = window->getContentView();
							if (contentInst.isNotNull()) {
								return TRUE;
							}
							HDC hDC = (HDC)(wParam);
							Color color = window->m_backgroundColor;
							if (color.a != 0) {
								HBRUSH hBrush = CreateSolidBrush(GraphicsPlatform::getColorRef(color));
								if (hBrush) {
									RECT rect;
									GetClientRect(hWnd, &rect);
									FillRect(hDC, &rect, hBrush);
									DeleteObject(hBrush);
									return TRUE;
								}
							}
							return DefWindowProcW(hWnd, uMsg, wParam, lParam);
						}
					}
				}
				return priv::view::ViewInstanceProc(hWnd, uMsg, wParam, lParam);
			}

		}
	}

	using namespace priv::window;

	Ref<WindowInstance> Window::createWindowInstance(const WindowInstanceParam& param)
	{
		HWND hWnd = Win32_WindowInstance::createHandle(param);
		if (hWnd) {
			return Win32_WindowInstance::create(hWnd, sl_true);
		}
		return sl_null;
	}

	Ref<Window> Window::getActiveWindow()
	{
		HWND hWnd = GetActiveWindow();
		if (hWnd) {
			Ref<WindowInstance> instance = UIPlatform::getWindowInstance(hWnd);
			if (instance.isNotNull()) {
				return instance->getWindow();
			}
		}
		return sl_null;
	}


	Ref<WindowInstance> UIPlatform::createWindowInstance(HWND hWnd, sl_bool flagDestroyOnRelease)
	{
		Ref<WindowInstance> ret = UIPlatform::_getWindowInstance((void*)hWnd);
		if (ret.isNotNull()) {
			return ret;
		}
		return Win32_WindowInstance::create(hWnd, flagDestroyOnRelease);
	}

	void UIPlatform::registerWindowInstance(HWND hWnd, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance((void*)hWnd, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(HWND hWnd)
	{
		return UIPlatform::_getWindowInstance((void*)hWnd);
	}

	void UIPlatform::removeWindowInstance(HWND hWnd)
	{
		UIPlatform::_removeWindowInstance((void*)hWnd);
	}

	HWND UIPlatform::getWindowHandle(WindowInstance* instance)
	{
		Win32_WindowInstance* window = (Win32_WindowInstance*)instance;
		if (window) {
			return window->m_handle;
		} else {
			return 0;
		}
	}

	HWND UIPlatform::getWindowHandle(Window* window)
	{
		if (window) {
			Ref<WindowInstance> _instance = window->getWindowInstance();
			Win32_WindowInstance* instance = (Win32_WindowInstance*)(_instance.get());
			if (instance) {
				return instance->m_handle;
			}
		}
		return 0;
	}

}

#endif
