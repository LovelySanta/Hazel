#include "hzpch.h"
#include "Application.h"

#include "Hazel/Log.h"

#include "Hazel/Renderer/Renderer.h"

#include "Input.h"

#include <GLFW/glfw3.h>

namespace Hazel {

	Application* Application::s_Instance = nullptr;
	std::string Application::s_AbsoluteDirectoryPath;

	Application::Application()
	{
		HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		HZ_CORE_TRACE("Running application from '{0}'", GetApplicationDirectoryPath().c_str());

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::OnWindowClose));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate(timestep);

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	void Application::CreateAbsoluteDirectoryPath(const char* argv0, const bool useFallback)
	{
	#ifdef HZ_PLATFORM_WINDOWS
		// When NULL is passed to GetModuleHandle, the handle of the exe itself is returned
		HMODULE hModule = GetModuleHandle(NULL);
		HZ_CORE_ASSERT(hModule, "Module handle is NULL");

		// Extract the path to this
		TCHAR ownPathT[MAX_PATH];
		GetModuleFileName(hModule, ownPathT, sizeof(ownPathT));

		// Convert TCHAR to std::string
		#ifndef UNICODE
			std::string ownPath(ownPathT);
		#else
			std::wstring ownPathW(ownPathT);
			std::string ownPath(ownPathW.begin(), ownPathW.end());
		#endif

		// ownPath could contain \.. in its path, we remove it
		// example: "C:\subDir\subsubDir\..\Sandbox" => "C:\subDir\Sandbox"
		size_t foundIndex = ownPath.find("\\..");
		while (foundIndex < ownPath.length())
		{
			size_t prevIndex = ownPath.rfind("\\", foundIndex - 1);
			ownPath.erase(prevIndex, foundIndex - prevIndex + 3);

			// search again
			foundIndex = ownPath.find("\\..");
		}

		// ownPath contains for example "C:\Sandbox\Sandbox.exe"
		// Remove the Sandbox.exe part from the end
		size_t absOwnDirIndex = ownPath.rfind("\\");
		if (absOwnDirIndex < ownPath.length())
			s_AbsoluteDirectoryPath = ownPath.substr(0, absOwnDirIndex + 1);
		else
			s_AbsoluteDirectoryPath = ownPath;
		return;

	#elif defined(HZ_PLATFORM_LINUX)
		// absolute pathname to current working directory of calling process
		char save_pwd[PATH_MAX];
		getcwd(save_pwd, sizeof(save_pwd));

		// create a working copy of argv0
		char save_argv0[PATH_MAX];
		strncpy(save_argv0, argv0, sizeof(save_argv0));
		save_argv0[sizeof(save_argv0) - 1] = 0;

		// get the path variable
		char save_path[PATH_MAX];
		strncpy(save_path, getenv("PATH"), sizeof(save_path));
		save_path[sizeof(save_path) - 1] = 0;

		// Now let us try and resolve any of these paths
		char save_realpath[PATH_MAX];
		char newpath[PATH_MAX + 256], newpath2[PATH_MAX + 256];
		char pathSeperator = '/';
		char pathSeperatorString[2] = "/";
		char pathSeperatorList[8] = ":"; // could be ":; "
		save_realpath[0] = 0;

		// Option 1: argv[0] contains an absolute path
		if (save_argv0[0] == pathSeperator) {
			realpath(save_argv0, newpath);
			HZ_CORE_ASSERT(access(newpath, F_OK), "Couldn't access path of running executable (option 1)");
			strncpy(save_realpath, newpath, sizeof(save_realpath));
			save_realpath[sizeof(save_realpath) - 1] = 0;
			s_AbsoluteDirectoryPath = save_realpath;
			return;

		} // Option 2: argv[0] contains a relative path to pwd
		else if (strchr(save_argv0, pathSeperator)) {
			strncpy(newpath2, save_pwd, sizeof(newpath2));
			newpath2[sizeof(newpath2) - 1] = 0;
			strncat(newpath2, pathSeperatorString, sizeof(newpath2));
			newpath2[sizeof(newpath2) - 1] = 0;
			strncat(newpath2, save_argv0, sizeof(newpath2));
			newpath2[sizeof(newpath2) - 1] = 0;

			realpath(newpath2, newpath);
			HZ_CORE_ASSERT(access(newpath, F_OK), "Couldn't access path of running executable (option 2)");
			strncpy(save_realpath, newpath, sizeof(save_realpath));
			save_realpath[sizeof(save_realpath) - 1] = 0;
			s_AbsoluteDirectoryPath = save_realpath;
			return;

		} // Option 3: searching $PATH for any possible relative path location...
		else {
			char* saveptr;
			char* pathitem;
			for (pathitem = strtok_r(save_path, pathSeperatorList, &saveptr);
			     pathitem; pathitem = strtok_r(NULL, pathSeperatorList, &saveptr)) {
				strncpy(newpath2, pathitem, sizeof(newpath2));
				newpath2[sizeof(newpath2) - 1] = 0;
				strncat(newpath2, pathSeperatorString, sizeof(newpath2));
				newpath2[sizeof(newpath2) - 1] = 0;
				strncat(newpath2, save_argv0, sizeof(newpath2));
				newpath2[sizeof(newpath2) - 1] = 0;

				realpath(newpath2, newpath);
				if (!access(newpath, F_OK)) {
					strncpy(save_realpath, newpath, sizeof(save_realpath));
					save_realpath[sizeof(save_realpath) - 1] = 0;
					s_AbsoluteDirectoryPath = save_realpath;
					return;
				}
			}
			HZ_CORE_ASSERT(useFallback, "Couldn't determine path of running executable ($PATH didn't have an accessable location)");
		}

		// if we get here, we have tried all three methods on argv[0]
		// and still haven't succeeded. Our last resort is reading
		// to use "/proc/self/exe", but this will fail if has a hard
		// link to the program, returning the wrong location.
		ssize_t written = ::readlink("proc/self/exe", save_realpath, sizeof(save_realpath) - 1);
		HZ_CORE_ASSERT(written != -1, "Couldn't determine path of running executable (readlink failed)");
		save_realpath[written] = 0;
		s_AbsoluteDirectoryPath = save_realpath;
		return;

	#else
		#error Unsupported platform!
	#endif
	}

	std::string Application::ResolvePath(const std::string& path)
	{
		// This will always return an absolute path
		std::filesystem::path fsPath(path);
		if (fsPath.is_absolute())
			return path;
		return GetApplicationDirectoryPath() + path;
	}

}
