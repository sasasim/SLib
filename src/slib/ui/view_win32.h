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

#ifndef CHECKHEADER_SLIB_UI_VIEW_WIN32
#define CHECKHEADER_SLIB_UI_VIEW_WIN32

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/view.h"

#include "ui_core_win32.h"

namespace slib
{

	class Win32_ViewInstance : public ViewInstance
	{
	public:
		Win32_ViewInstance();
		
		~Win32_ViewInstance();

	public:
		template <class T>
		static Ref<T> create(HWND hWnd, sl_bool flagDestroyOnRelease)
		{
			Ref<T> ret;
			if (hWnd) {
				ret = new T;
				if (ret.isNotNull()) {
					ret->m_handle = hWnd;
					ret->m_flagDestroyOnRelease = flagDestroyOnRelease;
					UIPlatform::registerViewInstance(hWnd, ret.get());
				} else {
					if (flagDestroyOnRelease) {
						PostMessageW(hWnd, SLIB_UI_MESSAGE_CLOSE, 0, 0);
					}
				}
			}
			return ret;
		}

		template <class T>
		static Ref<T> create(
			View* view, ViewInstance* parent
			, LPCWSTR wndClass, LPCWSTR text
			, int style, int styleEx)
		{
			UIRect frame = view->getFrameInInstance();
			Matrix3 transform = view->getFinalTransformInInstance();
			HWND handle = createHandle(view, parent, wndClass, text, style, styleEx, frame, transform);
			if (handle) {
				Ref<T> instance = create<T>(handle, sl_true);
				if (instance.isNotNull()) {
					instance->initialize(view, frame, transform);
					return instance;
				}
			}
			return sl_null;
		}

		static HWND createHandle(View* view, ViewInstance* parent
			, LPCWSTR wndClass, LPCWSTR text
			, int style, int styleEx, const UIRect& rect, const Matrix3& transform);

		void initialize(View* view, const UIRect& rect, const Matrix3& transform);

		HWND getHandle();

	public:
		virtual sl_bool preprocessWindowMessage(MSG& msg);

		virtual sl_bool processWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result);

		virtual sl_bool processCommand(SHORT code, LRESULT& result);

		virtual sl_bool processNotify(NMHDR* nmhdr, LRESULT& result);

		virtual sl_bool processControlColor(UINT msg, HDC hDC, HBRUSH& result);

		virtual void processPostControlColor(UINT msg, HDC hDC, HBRUSH& result);

	public:
		sl_bool isValid() override;

		void setFocus(sl_bool flag) override;

		void invalidate() override;

		void invalidate(const UIRect& rect) override;

		UIRect getFrame() override;

		void setFrame(const UIRect& frame) override;

		void setTransform(const Matrix3& transform) override;

		void setVisible(sl_bool flag) override;

		void setEnabled(sl_bool flag) override;

		void setOpaque(sl_bool flag) override;

		void setAlpha(sl_real alpha) override;

		void setClipping(sl_bool flag) override;

		void setDrawing(sl_bool flag) override;

		UIPointf convertCoordinateFromScreenToView(const UIPointf& ptScreen) override;

		UIPointf convertCoordinateFromViewToScreen(const UIPointf& ptView) override;

		void addChildInstance(const Ref<ViewInstance>& instance) override;

		void removeChildInstance(const Ref<ViewInstance>& instance) override;

		void bringToFront() override;

	public:
		sl_bool onEventKey(sl_bool flagDown, WPARAM wParam, LPARAM lParam);

		sl_bool onEventMouse(UIAction action, WPARAM wParam, LPARAM lParam);

		sl_bool onEventMouseWheel(sl_bool flagVertical, WPARAM wParam, LPARAM lParam);

		sl_bool onEventSetCursor();

		void applyModifiers(UIEvent* ev);

		void setGenericView(sl_bool flag);

	protected:
		HWND m_handle;
		sl_bool m_flagGenericView;
		sl_bool m_flagDestroyOnRelease;
		UIAction m_actionMouseCapture;
		
		UIRect m_frame;
		Vector2 m_translation;

	};

	namespace priv
	{
		namespace view
		{
			LRESULT CALLBACK ViewInstanceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		}
	}

}

#endif

#endif
