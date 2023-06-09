#include "platform/platform.h"

// Windows platform layer.
#if SPACE_PLATFORM_WINDOWS

	#include "core/event.h"
	#include "core/input.h"
	#include "core/logger.h"

	#include "containers/darray.h"

	#include <stdio.h>
	#include <stdlib.h>
	#include <windows.h>
	#include <windowsx.h> // param input extraction

	#include "renderer/vulkan/vulkan_types.inl"
	#include <vulkan/vulkan.h>
	#include <vulkan/vulkan_win32.h>

	#define WINDOW_CLASS_NAME "space_window_class"

typedef struct platform_state {
	HINSTANCE h_instance;
	HWND hwnd;
	VkSurfaceKHR surface;
} platform_state;

static platform_state *state_ptr;

// Clock
static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 message, WPARAM w_param, LPARAM l_param);

b8 platform_system_startup(u64 *memory_requirement,
						   void *state,
						   const char *application_name,
						   i32 x,
						   i32 y,
						   i32 width,
						   i32 height) {
	*memory_requirement = sizeof(platform_state);
	if (state == 0) { return true; }

	state_ptr             = state;
	state_ptr->h_instance = GetModuleHandleA(0);

	// Setup and register window class.
	HICON icon = LoadIcon(state_ptr->h_instance, IDI_APPLICATION);
	WNDCLASSA wc;
	memset(&wc, 0, sizeof(wc));
	wc.style         = CS_DBLCLKS; // Get double-clicks.
	wc.lpfnWndProc   = win32_process_message;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = state_ptr->h_instance;
	wc.hIcon         = icon;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // NULL; // Manage the cursor manually
	wc.hbrBackground = NULL;                        // Transparent
	wc.lpszClassName = WINDOW_CLASS_NAME;

	if (!RegisterClassA(&wc)) {
		MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	// Create window
	u32 client_x      = (u32)x;
	u32 client_y      = (u32)y;
	u32 client_width  = (u32)width;
	u32 client_height = (u32)height;

	u32 window_x      = client_x;
	u32 window_y      = client_y;
	u32 window_width  = client_width;
	u32 window_height = client_height;

	u32 window_style    = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
	u32 window_ex_style = WS_EX_APPWINDOW;

	window_style |= WS_MAXIMIZEBOX;
	window_style |= WS_MINIMIZEBOX;
	window_style |= WS_THICKFRAME;

	// Obtain the size of the border
	RECT border_rect = {0, 0, 0, 0};
	AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

	// In this case the border rectangle is negative.
	window_x += (u32)border_rect.left;
	window_y += (u32)border_rect.top;

	// Grow by the size of the OS border.
	window_width += (u32)(border_rect.right - border_rect.left);
	window_height += (u32)(border_rect.bottom - border_rect.top);

	HWND handle = CreateWindowExA(window_ex_style,
								  WINDOW_CLASS_NAME,
								  application_name,
								  window_style,
								  (i32)window_x,
								  (i32)window_y,
								  (i32)window_width,
								  (i32)window_height,
								  0,
								  0,
								  state_ptr->h_instance,
								  0);

	if (handle == 0) {
		MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		SFATAL("Window creation failed!");
		return false;
	} else {
		state_ptr->hwnd = handle;
	}

	// Show the window
	b32 should_activate           = 1; // TODO: if the window should not accept input, this should be false.
	i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
	// If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
	// If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE;
	ShowWindow(state_ptr->hwnd, show_window_command_flags);

	// Clock setup
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	clock_frequency = 1.0 / (f64)frequency.QuadPart;
	QueryPerformanceCounter(&start_time);

	return true;
}

void platform_system_shutdown(void *platform_state) {
	(void)platform_state;

	if (state_ptr && state_ptr->hwnd) {
		DestroyWindow(state_ptr->hwnd);
		state_ptr->hwnd = 0;
	}
}

b8 platform_pump_messages() {
	MSG message;
	while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}

	return true;
}

void *platform_allocate(u64 size, u64 aligned) {
	(void)aligned;

	return malloc(size);
}

void platform_free(void *block, b8 aligned) {
	(void)aligned;

	free(block);
}

void *platform_zero_memory(void *block, u64 size) { return memset(block, 0, size); }

void *platform_copy_memory(void *dest, const void *source, u64 size) { return memcpy(dest, source, size); }

void *platform_set_memory(void *dest, i32 value, u64 size) { return memset(dest, value, size); }

void print_with_colour(const char *message, u8 colour, DWORD output_handle) {
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	// FATAL, ERROR, WARN, INFO, DEBUG, TRACE
	static u8 levels[6] = {64, 4, 6, 2, 1, 8};
	SetConsoleTextAttribute(console_handle, levels[colour]);
	u64 length = strlen(message) + 3;
	char output[32002];
	sprintf(output, "%s\r\n", message);
	OutputDebugStringA(output);
	LPDWORD number_written = 0;
	WriteConsoleA(GetStdHandle(output_handle), output, (DWORD)length, number_written, 0);
}

void platform_console_write(const char *message, u8 colour) { print_with_colour(message, colour, STD_OUTPUT_HANDLE); }

void platform_console_write_error(const char *message, u8 colour) {
	print_with_colour(message, colour, STD_ERROR_HANDLE);
}

f64 platform_get_absolute_time() {
	LARGE_INTEGER now_time;
	QueryPerformanceCounter(&now_time);
	return (f64)now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms) { Sleep((u32)ms); }

void platform_get_required_extension_names(const char ***names_darray) {
	darray_push(*names_darray, &"VK_KHR_win32_surface");
}

b8 platform_create_vulkan_surface(vulkan_context *context) {
	if (!state_ptr) { return false; }

	VkWin32SurfaceCreateInfoKHR create_info = {
		.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = state_ptr->h_instance,
		.hwnd      = state_ptr->hwnd,
	};

	VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &state_ptr->surface);
	if (result != VK_SUCCESS) {
		SFATAL("Vulkan surface creation failed.");
		return false;
	}

	context->surface = state_ptr->surface;
	return true;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 message, WPARAM w_param, LPARAM l_param) {
	switch (message) {
		case WM_ERASEBKGND:
			// Notify the OS that erasing the screen will be handled by the application
			// to prevent flicker.
			return 1;

		case WM_CLOSE:
			event_context data = {};
			event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
			return true;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SIZE: {
			// Get the updated size.
			RECT r;
			GetClientRect(hwnd, &r);
			u32 width  = (u32)(r.right - r.left);
			u32 height = (u32)(r.bottom - r.top);

			event_context context;
			context.data.u16[0] = (u16)width;
			context.data.u16[1] = (u16)height;
			event_fire(EVENT_CODE_RESIZED, 0, context);
		} break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP: {
			// Key pressed/released
			b8 pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
			keys key   = (u16)w_param;

			input_process_key(key, pressed);
		} break;

		case WM_MOUSEMOVE: {
			// Mouse move
			i32 x_position = GET_X_LPARAM(l_param);
			i32 y_position = GET_Y_LPARAM(l_param);

			input_process_mouse_move((i16)x_position, (i16)y_position);
		} break;

		case WM_MOUSEHWHEEL: {
			i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
			if (z_delta != 0) {
				// Flatten the input to an OS-independent(-1, 1)
				z_delta = (z_delta < 0) ? -1 : 1;
				input_process_mouse_wheel((i8)z_delta);
			}
		} break;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP: {
			b8 pressed = message == WM_LBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_RBUTTONDOWN;

			buttons mouse_button = BUTTON_MAX_BUTTONS;
			switch (message) {
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
					mouse_button = BUTTON_LEFT;
					break;

				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
					mouse_button = BUTTON_MIDDLE;
					break;

				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
					mouse_button = BUTTON_RIGHT;
					break;

				default:
					break;
			}

			if (mouse_button != BUTTON_MAX_BUTTONS) { input_process_button(mouse_button, pressed); }
		} break;

		default:
			break;
	}

	return DefWindowProc(hwnd, message, w_param, l_param);
}

#endif // SPACE_PLATFORM_WINDOWS
