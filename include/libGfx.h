// |  |   |--|   |--|         |---/    |------| \-\   /-/      |
// |  |          |  |        | /       |  |      \ \-/ /       |
// |  |   |--|   |  |/--\   | |      /-------\    |   |        |
// |  |   |  |   |  | - |   |   ---|   |  |      / /-\ \       |
// |____| |__|   |__|___|    |____|    |__|    _/_/   \_\_   MY|

#ifndef NEBULAXLIBGFX_H
#define NEBULAXLIBGFX_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <atomic>
#include <cctype>
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <ft2build.h>
#include FT_FREETYPE_H
extern glm::mat4 s_projection;
extern GLuint lineVAO, lineVBO;
struct FontKey
{
  std::string name;
  int size;

  bool operator==(const FontKey& other) const
  {
    return name == other.name && size == other.size;
  }
};

namespace std
{
template <>
struct hash<FontKey>
{
  size_t operator()(const FontKey& k) const noexcept
  {
    return hash<string>()(k.name) ^ (hash<int>()(k.size)) << 1;
  }
};
}  // namespace std

namespace gfx
{



// ==================== Core Types ====================

/// @brief RGBA color representation (32-bit packed format)
/// @note Supports construction from: 
///       - 32-bit hex (0xRRGGBBAA)
///       - Individual components (r,g,b,a)
///       - CSS-style hex strings (#RRGGBB)
struct Color
{
  uint8_t r, g, b, a;

  // 默认构造函数
  constexpr Color(uint32_t rgba = 0xFFFFFFFF)
      : r((rgba >> 24) & 0xFF),
        g((rgba >> 16) & 0xFF),
        b((rgba >> 8) & 0xFF),
        a(rgba & 0xFF)
  {
  }

  Color(int r, int g, int b) : r(r), g(g), b(b), a(255) {}
  Color(int r, int g, int b, int a) : r(r), g(g), b(b), a(a) {}
  // 支持 #RRGGBB 的构造函数
  Color(const std::string& hex_str)
  {
    if (hex_str[0] == '#')
    {
      unsigned int hex;
      std::stringstream ss;
      ss << std::hex << hex_str.substr(1);
      ss >> hex;
      r = (hex >> 16) & 0xFF;
      g = (hex >> 8) & 0xFF;
      b = hex & 0xFF;
      a = 255;  // 默认不透明
    }
  }

  // 转换为 uint32_t 类型
  operator uint32_t() const
  {
    return (r << 24) | (g << 16) | (b << 8) | a;
  }
};
/// @brief 2D point in screen coordinates
struct Point {
  float x, y;

  Point(float x, float y) : x(x), y(y) {}

  explicit Point(double x, double y)
    : Point(static_cast<float>(x), static_cast<float>(y)) {}

  explicit Point(int x, int y)
    : Point(static_cast<float>(x), static_cast<float>(y)) {}
  Point(){}
};


/// @brief Axis-aligned rectangle with floating-point precision
/// @note Provides constructors for float/double/int types
struct Rect
{
  float x, y, w, h;

  // 原来支持 float 的构造函数
  Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

  // 新增支持 double 的构造函数
  Rect(double x, double y, double w, double h)
      : x(static_cast<float>(x)),
        y(static_cast<float>(y)),
        w(static_cast<float>(w)),
        h(static_cast<float>(h))
  {
  }

  // 新增支持 int 的构造函数
  Rect(int x, int y, int w, int h)
      : x(static_cast<float>(x)),
        y(static_cast<float>(y)),
        w(static_cast<float>(w)),
        h(static_cast<float>(h))
  {
  }
};

struct Texture;
class Font;
class FontA;

// ==================== Rendering Core ====================

/// @brief Main graphics controller (static class)
/// @warning Most methods must be called from render thread

class Renderer
{
 public:
  // 初始化/销毁
  /// @brief Initializes graphics subsystem
    /// @return true if successful
    /// @throws std::runtime_error on OpenGL/GLEW errors
  static bool Init(SDL_Window* window);
  /// @brief Releases all graphics resources
  static void Shutdown();

  // 状态控制
  /// @brief Clears framebuffer with specified color
  static void Clear(Color bg);
  /// @brief Swaps front/back buffers (frame presentation)
  static void Present();
  /// @brief Sets viewport dimensions
    /// @param area Viewport rectangle in screen coordinates
  static void SetViewport(Rect area);

  // 绘图指令
  // Primitive drawing commands
  static void DrawRect(Rect rect, Color fill);
  static void DrawLine(Point p1, Point p2, Color color, float width = 1.0f);
  static void DrawTexture(Texture* tex, Rect dest, float rotation = 0.0f);
  static void DrawTexture(GLuint tex, Rect dest, float rotation = 0.0f);

  static void DrawText(const std::string& text, Point pos, Font* font);
  static void DrawText(const std::string& text, Point pos, FontA* font,Color color,float scale= 1.0f, float rotation = 0.0f);

  // 资源管理
  /**
   * 创建一个空白的纹理
   * @param width 纹理宽度
   * @param height 纹理高度
   * @return 新创建的纹理对象指针，失败返回nullptr
   */
  static Texture* CreateTexture(int width, int height);

  /**
   * 从文件加载纹理
   * @param path 图像文件路径
   * @return 新创建的纹理对象指针，失败返回nullptr
   */
  static Texture* LoadTexture(const std::string& path);

  /**
   * 释放纹理资源
   * @param tex 要释放的纹理对象
   */
  static void ReleaseTexture(Texture* tex);
  // 窗口大小变化处理
  static void HandleWindowResize(int width, int height)
  {
    s_projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
    glViewport(0, 0, width, height);
  }
};

// ==================== Resource Management ====================

/// @brief GPU texture resource
/// @note Managed via reference counting
struct Texture
{
  GLuint id;
  int width, height;
  ~Texture()
  {
    if (id) glDeleteTextures(1, &id);
  }

  void Bind(GLuint unit = 0) const
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);

  }
};

// ==================== 字体系统 ====================
class FontA {
  public:
      static FontA* Load(const std::string& path, int size);
      static void Release(FontA* font);
      static Texture* GetTextTexture(FontA* font, const char* text, Color color);
  
      TTF_Font* GetRaw() const { return font_; }
  
  private:
      FontA() = default;
      TTF_Font* font_ = nullptr;
      std::string TextCache="";
      Texture* TextureCached;
  };
  
/// @brief Font resource with glyph cache
class Font
{
 public:
 // @brief Loads font face with specific size
    /// @param color Glyph atlas tint color
    /// @return New font instance (managed)
  static Font* Load(const std::string& path, int size,Color color);
  /// @brief Releases font resources
    /// @note Invalidates all associated glyph textures
  static void Release(Font* font);
  int size=0;
  struct Glyph
{
    int bitmap_left;
    int bitmap_top;
    int advance_x;
    GLuint texture;
    int w;
    int h;
    Glyph() : bitmap_left(0), bitmap_top(0), advance_x(0), texture(0),w(0),h(0) {}
    Glyph(int left, int top, int advance, GLuint tex,int mw,int mh)
        : bitmap_left(left), bitmap_top(top), advance_x(advance), texture(tex),w(mw),h(mh) {}

};


  const Glyph* GetGlyph(char32_t codepoint) const;
  
 private:
  std::unordered_map<char32_t, Glyph> glyphs;

};

// ==================== 高级功能 ====================
namespace advanced{
// 离屏渲染
class Framebuffer
{
 public:
  // Framebuffer(int width, int height);
  //~Framebuffer();

  // void Bind();
  // void Unbind();
  Texture* GetColorTexture() const
  {
    return colorTex.get();
  }

 private:
  GLuint fbo;
  std::unique_ptr<Texture> colorTex;
  GLuint depthBuffer;
};

// 着色器系统
class Shader
{
 public:
  // static Shader* Create(const char* vsSrc, const char* fsSrc);
  // void Use();
  // void SetUniform(const char* name, float value);
  // void SetUniform(const char* name, const Color& color);
};
}  // namespace advanced

// ==================== 线程安全渲染 ====================
namespace async
{
// void RunOnRenderThread(std::function<void()> task);
// void ProcessTasks(); // 每帧调用
}

// ==================== Preset Colors ====================
const Color Black(0x000000FF);
const Color White(0xFFFFFFFF);
const Color Red(0xFF0000FF);
const Color Green(0x00FF00FF);
const Color Blue(0x0000FFFF);
const Color Yellow(0xFFFF00FF);
const Color Cyan(0x00FFFF00);
const Color Magenta(0xFF00FFFF);

}  // namespace gfx
inline int GFX_INIT(){
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    std::cerr << "SDL_Init Failed: " << SDL_GetError() << std::endl;
    return -1;
  }
  TTF_Init();
  glewInit();
  return 0;
}
#endif  // NEBULAXLIBGFX_H