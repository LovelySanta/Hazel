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
		struct stat buffer;
		int status;

		std::vector<std::string>prepath_string_vector = {"", "./Sandbox/", "../../../Sandbox/"};
			
		for (auto path_iter = prepath_string_vector.cbegin(); path_iter != prepath_string_vector.cend(); path_iter++)
		{
			std::string modified_path = *path_iter + path;
			status = stat(modified_path.c_str(), &buffer);

			if (status == 0)
				return modified_path;
		}
		return path;
	}

}