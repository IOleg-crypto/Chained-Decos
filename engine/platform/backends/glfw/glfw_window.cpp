#include "engine/platform/backends/glfw/glfw_window.h"
#include "engine/common/engine_assert.h"
#include "engine/common/platform_detection.h"
#include "engine/core/events/window_events.h"
#include "engine/core/input.h"

#include "engine/platform/backends/glfw/glfw_input_mapper.h"
#include "imgui_impl_glfw.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <stb_image.h>

namespace Chained
{

	static bool s_GLFWInitialized = false;

	std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
	{

		return std::make_unique<GlfwWindow>(properties);
	}

	static void GLFWErrorCallback(int error, const char* description)
	{
		CH_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	GlfwWindow::GlfwWindow(const WindowProperties& properties)
	{
		Init(properties);
	}

	GlfwWindow::~GlfwWindow()
	{
		Shutdown();
	}

	void GlfwWindow::Init(const WindowProperties& properties)
	{
		int initialWidth = properties.Width;
		int initialHeight = properties.Height;
		m_Title = properties.Title;
		m_VSync = properties.VSync;
		m_TargetFPS = properties.TargetFramesPerSecond;

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			CH_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}

		// Якщо розміри не вказані, беремо робочу область головного монітора
		if (initialWidth <= 0 || initialHeight <= 0)
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			if (monitor)
			{
				int workX = 0, workY = 0, workW = 0, workH = 0;
				glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);
				initialWidth = (workW > 0) ? workW : 1280;
				initialHeight = (workH > 0) ? workH : 720;
			}
			else
			{
				initialWidth = 1280;
				initialHeight = 720;
			}
		}

		constexpr int minimumWidth = 800;
		constexpr int minimumHeight = 600;
		initialWidth = (initialWidth < minimumWidth) ? minimumWidth : initialWidth;
		initialHeight = (initialHeight < minimumHeight) ? minimumHeight : initialHeight;

		m_Width = (uint32_t)initialWidth;
		m_Height = (uint32_t)initialHeight;
		CH_CORE_INFO("Initializing Glfw Window: {} ({}x{})", m_Title, m_Width, m_Height);

		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_Samples = properties.Samples;
		glfwWindowHint(GLFW_SAMPLES, m_Samples > 0 ? m_Samples : 0);

#if CH_PLATFORM_MACOS
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

		m_WindowHandle = glfwCreateWindow((int)m_Width, (int)m_Height, m_Title.c_str(), nullptr, nullptr);
		CH_CORE_ASSERT(m_WindowHandle, "Failed to create GLFW window!");
		glfwSetWindowSizeLimits(m_WindowHandle, minimumWidth, minimumHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);

		glfwMakeContextCurrent(m_WindowHandle);
		glfwSetWindowUserPointer(m_WindowHandle, this);

		int fbWidth, fbHeight;
		glfwGetFramebufferSize(m_WindowHandle, &fbWidth, &fbHeight);
		m_FramebufferWidth = (uint32_t)fbWidth;
		m_FramebufferHeight = (uint32_t)fbHeight;

		glfwSetFramebufferSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height) {
			auto& glWindow = *(GlfwWindow*)glfwGetWindowUserPointer(window);

			glWindow.m_FramebufferWidth = (uint32_t)width;
			glWindow.m_FramebufferHeight = (uint32_t)height;

			int winWidth, winHeight;
			glfwGetWindowSize(window, &winWidth, &winHeight);
			glWindow.SetSizeDirect(winWidth, winHeight);

			WindowResizeEvent event(width, height);
			if (glWindow.m_EventCallback)
			{
				glWindow.m_EventCallback(event);
			}

			glViewport(0, 0, width, height);
		});
		glfwSetWindowCloseCallback(m_WindowHandle, [](GLFWwindow* window) {
			auto& glWindow = *(GlfwWindow*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			if (glWindow.m_EventCallback)
			{
				glWindow.m_EventCallback(event);
			}
		});

		glfwSetScrollCallback(m_WindowHandle, [](GLFWwindow* window, double xOffset, double yOffset) {
			Core::Input::OnMouseScroll((float)xOffset, (float)yOffset);
			ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
		});

		glfwSetKeyCallback(m_WindowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			Core::Input::OnKey(GlfwInputMapper::MapKey(key), action != GLFW_RELEASE);
			ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
		});

		glfwSetMouseButtonCallback(m_WindowHandle, [](GLFWwindow* window, int button, int action, int mods) {
			Core::Input::OnMouseButton(GlfwInputMapper::MapMouseButton(button), action != GLFW_RELEASE);
			ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
		});

		glfwSetCursorPosCallback(m_WindowHandle, [](GLFWwindow* window, double xpos, double ypos) {
			Core::Input::OnMouseMove((float)xpos, (float)ypos);
			ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
		});

		glfwSetCharCallback(m_WindowHandle,
							[](GLFWwindow* window, unsigned int c) { ImGui_ImplGlfw_CharCallback(window, c); });

		glfwSetWindowFocusCallback(m_WindowHandle, [](GLFWwindow* window, int focused) {
			if (!focused)
			{
				Core::Input::ResetAll();
			}
		});

		int status = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
		CH_CORE_ASSERT(status, "Failed to initialize Glad!");

		CH_CORE_INFO("OpenGL Info:");
		CH_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		CH_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		CH_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));

		SetVSync(m_VSync);
	}

	void GlfwWindow::Shutdown()
	{
		if (m_WindowHandle)
		{
			glfwSetScrollCallback(m_WindowHandle, nullptr);
			glfwSetKeyCallback(m_WindowHandle, nullptr);
			glfwSetMouseButtonCallback(m_WindowHandle, nullptr);
			glfwSetCursorPosCallback(m_WindowHandle, nullptr);
			glfwSetCharCallback(m_WindowHandle, nullptr);
			glfwSetWindowFocusCallback(m_WindowHandle, nullptr);
			glfwSetFramebufferSizeCallback(m_WindowHandle, nullptr);
			glfwSetWindowCloseCallback(m_WindowHandle, nullptr);

			glfwMakeContextCurrent(nullptr);
			glfwDestroyWindow(m_WindowHandle);
			m_WindowHandle = nullptr;
		}

		CH_CORE_INFO("Glfw Window Closed");
	}

	void GlfwWindow::BeginFrame()
	{
		glfwPollEvents();
	}

	void GlfwWindow::EndFrame()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

	bool GlfwWindow::ShouldClose() const
	{
		return glfwWindowShouldClose(m_WindowHandle);
	}

	void GlfwWindow::SetTitle(const std::string& title)
	{
		m_Title = title;
		glfwSetWindowTitle(m_WindowHandle, m_Title.c_str());
	}

	void GlfwWindow::SetSize(int width, int height)
	{
		m_Width = (uint32_t)width;
		m_Height = (uint32_t)height;
		glfwSetWindowSize(m_WindowHandle, (int)m_Width, (int)m_Height);
	}

	void GlfwWindow::SetSizeDirect(int width, int height)
	{
		m_Width = (uint32_t)width;
		m_Height = (uint32_t)height;
	}

	void GlfwWindow::ToggleFullscreen()
	{
		SetFullscreen(!m_IsFullscreen);
	}

	void GlfwWindow::SetFullscreen(bool enabled)
	{
		if (m_IsFullscreen == enabled)
		{
			return;
		}

		if (enabled)
		{
			glfwGetWindowPos(m_WindowHandle, &m_WindowedX, &m_WindowedY);
			glfwGetWindowSize(m_WindowHandle, &m_WindowedWidth, &m_WindowedHeight);

			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(m_WindowHandle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			m_IsFullscreen = true;
		}
		else
		{
			glfwSetWindowMonitor(m_WindowHandle, nullptr, m_WindowedX, m_WindowedY, m_WindowedWidth, m_WindowedHeight,
								 GLFW_DONT_CARE);
			m_IsFullscreen = false;
		}
	}

	void GlfwWindow::SetWindowIcon(const std::string& path)
	{
		GLFWimage image{};

		image.pixels = stbi_load(path.c_str(), &image.width, &image.height, nullptr, 4);

		if (image.pixels)
		{
			glfwSetWindowIcon(m_WindowHandle, 1, &image);
			stbi_image_free(image.pixels);
		}
		else
		{
			CH_CORE_WARN("Failed to load window icon from {}", path);
		}
	}

	void GlfwWindow::SetWindowIconFromMemory(const uint8_t* data, size_t size)
	{
		GLFWimage image{};

		image.pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data), static_cast<int>(size),
											 &image.width, &image.height, nullptr, 4);

		if (image.pixels)
		{
			glfwSetWindowIcon(m_WindowHandle, 1, &image);
			stbi_image_free(image.pixels);
		}
		else
		{
			CH_CORE_WARN("Failed to decode window icon from memory");
		}
	}

	void GlfwWindow::SetVSync(bool enabled)
	{
		m_VSync = enabled;
		glfwSwapInterval(m_VSync ? 1 : 0);
	}

	void GlfwWindow::SetAntialiasing(bool enabled)
	{
		// GLFW_SAMPLES only takes effect on the *next* glfwCreateWindow call, so calling
		// glfwWindowHint here would be a silent no-op for the window that's already open.
		// If the framebuffer wasn't created with samples, GL_MULTISAMPLE has nothing to
		// multisample and this call is harmless but ineffective - warn so it's not a silent no-op.
		if (enabled && m_Samples <= 0)
		{
			CH_CORE_WARN("SetAntialiasing(true) requested but the window was created with "
						 "Samples = 0. MSAA sample count can only be changed by "
						 "recreating the window; toggling GL_MULTISAMPLE will have no visible effect.");
		}

		if (enabled)
		{
			glEnable(GL_MULTISAMPLE);
		}
		else
		{
			glDisable(GL_MULTISAMPLE);
		}
	}

	void GlfwWindow::SetTargetFramesPerSecond(int framesPerSecond)
	{
		m_TargetFPS = framesPerSecond;
	}

	void GlfwWindow::SetCursorMode(CursorMode mode)
	{
		switch (mode)
		{
		case CursorMode::Normal:
			glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			break;
		case CursorMode::Hidden:
			glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			break;
		case CursorMode::Locked:
			glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			break;
		}
	}

	bool GlfwWindow::IsFocused() const
	{
		return glfwGetWindowAttrib(m_WindowHandle, GLFW_FOCUSED) == GLFW_TRUE;
	}

} // namespace Chained