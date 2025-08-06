#include "../include/libGfx.h"
namespace gfx
{
FontA* FontA::Load(const std::string& path, int size)
{
  TTF_Font* font = TTF_OpenFont(path.c_str(), size);
  if (!font)
  {
    std::cerr << "TTF_OpenFont Failed: " << TTF_GetError() << std::endl;
    return nullptr;
  }
  FontA* fonta = new FontA();
  fonta->font_ = font;
  return fonta;
}

void FontA::Release(FontA* font)
{
  if (font && font->font_)
  {
    TTF_CloseFont(font->font_);
    delete font;
  }
}

Texture* FontA::GetTextTexture(FontA* font, const char* text, Color color)
{
  if (!font || !text) return {};
  if (font->TextCache == text) return font->TextureCached;

  SDL_Color sdlColor = {color.r, color.g, color.b, color.a};
  SDL_Surface* surface = TTF_RenderText_Blended(font->font_, text, sdlColor);
  if (!surface)
  {
    std::cerr << "TTF_RenderText_Blended Failed: " << TTF_GetError()
              << std::endl;
    return {};
  }

  SDL_Surface* converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(surface);

  if (!converted)
  {
    std::cerr << "Failed to convert image" << std::endl;
    return {};
  }

  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, converted->w, converted->h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
  glGenerateMipmap(GL_TEXTURE_2D);

  SDL_FreeSurface(converted);

  GLenum err = glGetError();
  if (err != GL_NO_ERROR)
  {
    std::cerr << "Failed to upload texture: " << err << std::endl;
    glDeleteTextures(1, &textureID);
    return {};
  }

  // 释放旧纹理和旧对象
  if (font->TextureCached)
  {
    glDeleteTextures(1, &font->TextureCached->id);
    delete font->TextureCached;
  }

  font->TextureCached = new Texture{textureID, converted->w, converted->h};
  return font->TextureCached;
}

void Renderer::DrawText(const std::string& text, Point pos, FontA* font,
                        Color color, float scale, float rotation)
{
    Texture* texture = font->GetTextTexture(font, text.c_str(), color);  // 保持为指针类型
    if (texture->id == 0)
    {
        return;
    }

    // 计算左上角为原点的绘制区域
    Rect dest(static_cast<float>(pos.x), static_cast<float>(pos.y),
              static_cast<float>(texture->width) * scale,
              static_cast<float>(texture->height) * scale);
    // 绘制纹理
    DrawTexture(texture, dest, rotation);

}


}  // namespace gfx
