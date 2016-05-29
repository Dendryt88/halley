#include "halley/concurrency/concurrent.h"
#include "halley/support/exception.h"
#include "halley_gl.h"
#include "texture_opengl.h"
#include "halley/core/graphics/texture_descriptor.h"

using namespace Halley;

TextureOpenGL::TextureOpenGL(const TextureDescriptor& d)
{
	textureId = create(d.size.x, d.size.y, d.format, d.useMipMap, d.useFiltering);
	if (d.pixelData != nullptr) {
		loadImage(reinterpret_cast<const char*>(d.pixelData), d.size.x, d.size.y, d.size.x, d.format, d.useMipMap);
	}
}

void TextureOpenGL::bind(int textureUnit)
{
	GLUtils glUtils;
	glUtils.setTextureUnit(textureUnit);
	glUtils.bindTexture(textureId);
}

unsigned int TextureOpenGL::create(size_t w, size_t h, TextureFormat format, bool useMipMap, bool useFiltering)
{
	glCheckError();
	assert(w > 0);
	assert(h > 0);
	assert(w <= 4096);
	assert(h <= 4096);

	size = Vector2i(static_cast<int>(w), static_cast<int>(h));

	int filtering = useFiltering ? GL_LINEAR : GL_NEAREST;

	unsigned int id;
	glGenTextures(1, &id);
	
	GLUtils glUtils;
	glUtils.bindTexture(id);

	//loader = TextureLoadQueue::getCurrent();

#ifdef WITH_OPENGL_ES
	GLuint pixFormat = GL_UNSIGNED_BYTE;
#else
	GLuint pixFormat = GL_UNSIGNED_BYTE;

	if (useMipMap) glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glCheckError();
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useMipMap ? GL_LINEAR_MIPMAP_LINEAR : filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

	if (format == TextureFormat::DEPTH) {
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	std::vector<char> blank;
	blank.resize(w * h * TextureDescriptor::getBitsPerPixel(format));
	GLuint glFormat = getGLFormat(format);
	GLuint format2 = glFormat;
	if (format2 == GL_RGBA16F || format2 == GL_RGBA16) format2 = GL_RGBA;
	if (format2 == GL_DEPTH_COMPONENT24) format2 = GL_DEPTH_COMPONENT;
	glTexImage2D(GL_TEXTURE_2D, 0, glFormat, size.x, size.y, 0, format2, pixFormat, blank.data());
	glCheckError();

	return id;
}

void TextureOpenGL::loadImage(const char* px, size_t w, size_t h, size_t stride, TextureFormat format, bool useMipMap)
{
	glPixelStorei(GL_PACK_ROW_LENGTH, int(stride));
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, int(w), int(h), getGLFormat(format), GL_UNSIGNED_BYTE, px);
	glCheckError();

#ifndef WITH_OPENGL_ES

	// Generate mipmap
	if (useMipMap) {
		glGenerateMipmap(GL_TEXTURE_2D);
		glCheckError();
	}

#endif

	if (Concurrent::getThreadName() != "main") {
		glFinish();
	}
}

unsigned TextureOpenGL::getGLFormat(TextureFormat format)
{
	switch (format) {
	case TextureFormat::RGB:
		return GL_RGB;
	case TextureFormat::RGBA:
		return GL_RGBA;
	case TextureFormat::DEPTH:
		return GL_DEPTH_COMPONENT24;
	}
	throw Exception("Unknown texture format: " + String::integerToString(static_cast<int>(format)));
}