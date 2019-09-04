#include "hzpch.h"
#include "Texture.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Hazel {
	std::string CorrectFilePath(const std::string&);

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture2D>(CorrectFilePath(path));
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	std::string CorrectFilePath(const std::string& path)
	{
		#if defined(HZ_DEBUG) || defined(HZ_RELEASE)
			struct stat buffer;
			int status;

			std::vector<std::string>prepath_string_vector = {"", "./Sandbox/", "../../../Sandbox/"};

			#ifdef HZ_PLATFORM_WINDOWS
				for (auto string_iter : prepath_string_vector)
				{
					for (int char_check = 0; char_check < string_iter.size(); char_check++)
					{
						if (string_iter[char_check] == '/')
							string_iter[char_check] = '\\';
					}
				}
			#endif

			for (auto path_iter : prepath_string_vector)
			{
				std::string modified_path = path_iter + path;
				status = stat(modified_path.c_str(), &buffer);

				if (status == 0)
					return modified_path;
			}
		#endif

		return path;
	}

}