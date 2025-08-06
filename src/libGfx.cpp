#include "../include/libGfx.h"

#include <GL/glew.h>

#include <stdexcept>
#include <unordered_map>
#include <vector>
glm::mat4 s_projection;
GLuint lineVAO = 0, lineVBO = 0;
namespace gfx
{

// ================ 内部全局状态 ================
namespace
{
SDL_Window* s_window = nullptr;
SDL_GLContext s_glContext = nullptr;
std::thread::id s_renderThreadId;
bool s_glewInitialized = false;

// 字体/纹理缓存
std::unordered_map<std::string, Texture*> s_textureCache;
std::unordered_map<FontKey, Font*> s_fontCache;

GLuint quadVAO = 0, quadVBO = 0;
GLuint shaderProgram = 0;
}  // namespace

// ================ 辅助函数 ================
bool IsRenderThread()
{
  return std::this_thread::get_id() == s_renderThreadId;
}

void VerifyRenderThread()
{
  if (!IsRenderThread())
  {
    throw std::runtime_error("GFX operation called from non-render thread");
  }
}

// ================ 初始化实现 ================
bool Renderer::Init(SDL_Window* window)
{
  if (s_window) return true;  // 避免重复初始化

  s_window = window;
  s_renderThreadId = std::this_thread::get_id();

  // 创建OpenGL上下文
  s_glContext = SDL_GL_CreateContext(window);
  if (!s_glContext)
  {
    SDL_Log("Failed to create GL context: %s", SDL_GetError());
    return false;
  }

  // 初始化GLEW
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    SDL_Log("GLEW init failed: %s", glewGetErrorString(err));
    return false;
  }
  s_glewInitialized = true;

  // OpenGL基础配置
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  // 检查OpenGL版本
  if (!GLEW_VERSION_2_1)
  {
    SDL_Log("OpenGL 2.1+ required");
    return false;
  }

  SDL_Log("Renderer: %s", glGetString(GL_RENDERER));
  SDL_Log("OpenGL: %s", glGetString(GL_VERSION));

  // ---------------- 创建 VAO/VBO ----------------
  GLfloat vertices[] = {
      // 位置       // 纹理坐标
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 左下
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 右下
      1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // 右上
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f   // 左上
  };

  // 线段 VAO/VBO
  glGenVertexArrays(1, &lineVAO);
  glGenBuffers(1, &lineVBO);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  // 四边形 VAO/VBO
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);

  // ---------------- 创建着色器 ----------------
  const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 position;

    uniform mat4 projection;
    uniform mat4 model;
    uniform vec4 uvRect;
    out vec2 TexCoord;
    void main() {
        
        gl_Position = projection * model * vec4(position, 1.0);

        vec2 quadUV[4] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
        );

        TexCoord = uvRect.xy + quadUV[gl_VertexID] * uvRect.zw;
        
    })";

  const char* fragmentShaderSource = R"(
        #version 330 core
in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D texture1;
uniform bool useTexture;
uniform vec4 color;

void main() {
    if (useTexture) {
        vec4 texColor = texture(texture1, TexCoord);
        
        if (texColor.a < 0.1) {
            discard;
        }
        
        fragColor = texColor * color;
    } else {
        fragColor = color;
    }
}

    )";

  // 编译顶点着色器
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // 编译片段着色器
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // 着色器错误检查
  GLint success;
  GLchar infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    SDL_Log("Vertex shader compile error: %s", infoLog);
  }
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    SDL_Log("Fragment shader compile error: %s", infoLog);
  }

  // 链接程序
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // 检查链接错误
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    SDL_Log("Shader program link error: %s", infoLog);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // 使用程序并设置投影矩阵
  glUseProgram(shaderProgram);
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  glViewport(0, 0, width, height);  // 设置viewport
  s_projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);

  GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(s_projection));

  return true;
}

// ================ 销毁实现 ================
void Renderer::Shutdown()
{
  // 清理资源缓存
  for (auto& [path, tex] : s_textureCache)
  {
    glDeleteTextures(1, &tex->id);
    delete tex;
  }
  s_textureCache.clear();

  for (auto& [key, font] : s_fontCache)
  {
    delete font;
  }
  s_fontCache.clear();

  // 销毁OpenGL上下文
  if (s_glContext)
  {
    SDL_GL_DeleteContext(s_glContext);
    s_glContext = nullptr;
  }

  // 删除VBO和VAO
  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);

  // 重置状态
  s_window = nullptr;
  s_glewInitialized = false;
}

// ================ 状态控制 ================
void Renderer::Clear(Color bg)
{
  VerifyRenderThread();  // 确保在渲染线程

  glClearColor(bg.r / 255.0f, bg.g / 255.0f, bg.b / 255.0f, bg.a / 255.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Present()
{
  VerifyRenderThread();
  SDL_GL_SwapWindow(s_window);
}

void Renderer::SetViewport(Rect area)
{
  VerifyRenderThread();
  glViewport(static_cast<GLint>(area.x), static_cast<GLint>(area.y),
             static_cast<GLsizei>(area.w), static_cast<GLsizei>(area.h));
}

// ================ 绘图指令 ================
void Renderer::DrawRect(Rect rect, Color fill)
{
  glm::mat4 model =
      glm::translate(glm::mat4(1.0f), glm::vec3(rect.x, rect.y, 0.0f));
  model = glm::scale(model, glm::vec3(rect.w, rect.h, 1.0f));

  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE,
                     &model[0][0]);
  glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_FALSE);
  glUniform4f(glGetUniformLocation(shaderProgram, "color"), fill.r / 255.0f,
              fill.g / 255.0f, fill.b / 255.0f, fill.a / 255.0f);

  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
void Renderer::DrawLine(Point p1, Point p2, Color color, float width)
{
  glUseProgram(shaderProgram);
  glm::vec2 points[2] = {glm::vec2(p1.x, p1.y), glm::vec2(p2.x, p2.y)};

  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STREAM_DRAW);

  glBindVertexArray(lineVAO);
  glEnableVertexAttribArray(0);

  glm::mat4 model(1.0f);  // 单位矩阵
  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE,
                     glm::value_ptr(model));
  glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_FALSE);
  glUniform4f(glGetUniformLocation(shaderProgram, "color"), color.r / 255.0f,
              color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

  glLineWidth(width);
  glDrawArrays(GL_LINES, 0, 2);
}

void Renderer::DrawTexture(Texture* tex, Rect dest, float rotation)
{
  DrawTexture(tex->id, dest,rotation);
}
void Renderer::DrawTexture(GLuint tex, Rect dest, float rotation)
{
    if (tex == 0) {
        std::cerr << "Invalid texture ID!" << std::endl;
        return;
    }

    glUseProgram(shaderProgram);
    glBindVertexArray(quadVAO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(dest.x, dest.y, 0.0f));

    if (rotation != 0.0f) {
        model = glm::translate(model, glm::vec3(dest.w / 2, dest.h / 2, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-dest.w / 2, -dest.h / 2, 0.0f));
    }

    model = glm::scale(model, glm::vec3(dest.w, dest.h, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_TRUE);
    glUniform4f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 1.0f, 1.0f);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    glUniform4f(glGetUniformLocation(shaderProgram, "uvRect"), 0.0f, 0.0f, 1.0f, 1.0f);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisable(GL_BLEND);
}


void Renderer::DrawText(const std::string& text, Point pos, Font* font)
{
    if (!font) return;

    for (size_t i = 0; i < text.size(); ++i)
    {
        char32_t codepoint = static_cast<unsigned char>(text[i]); // 仅支持 ASCII
        const Font::Glyph* glyph = font->GetGlyph(codepoint);
        if (!glyph) continue;

        // 计算字符纹理左上角位置（相对于基线 pos）
        gfx::Rect dest(
            static_cast<float>(pos.x + glyph->bitmap_left),
            static_cast<float>(pos.y - glyph->bitmap_top),
            static_cast<float>(glyph->w),
            static_cast<float>(glyph->h)
        );



        DrawTexture(glyph->texture, dest);

        // 移动到下一个字符位置
        pos.x += glyph->advance_x;
    }
}


Texture* Renderer::CreateTexture(int width, int height)
{
  if (width <= 0 || height <= 0)
  {
    std::cerr << "Invalid texture dimensions: " << width << "x" << height
              << std::endl;
    return nullptr;
  }

  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  // 设置默认纹理参数
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // 创建空纹理
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);

  // 检查错误
  GLenum err = glGetError();
  if (err != GL_NO_ERROR)
  {
    std::cerr << "Failed to create texture: " << err << std::endl;
    glDeleteTextures(1, &textureID);
    return nullptr;
  }

  return new Texture{textureID, width, height};
}
Texture* Renderer::LoadTexture(const std::string& path)
{
  // 使用SDL_image加载图像
  SDL_Surface* surface = IMG_Load(path.c_str());
  if (!surface)
  {
    std::cerr << "Failed to load image: " << path << " - " << IMG_GetError()
              << std::endl;
    return nullptr;
  }

  // 转换为RGBA格式（如果还不是）
  SDL_Surface* converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(surface);

  if (!converted)
  {
    std::cerr << "Failed to convert image: " << path << std::endl;
    return nullptr;
  }

  // 创建OpenGL纹理
  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  // 设置纹理参数
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // 上传纹理数据
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, converted->w, converted->h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
  glGenerateMipmap(GL_TEXTURE_2D);

  // 清理SDL表面
  SDL_FreeSurface(converted);

  // 检查错误
  GLenum err = glGetError();
  if (err != GL_NO_ERROR)
  {
    std::cerr << "Failed to upload texture: " << err << std::endl;
    glDeleteTextures(1, &textureID);
    return nullptr;
  }

  return new Texture{textureID, converted->w, converted->h};
}
void Renderer::ReleaseTexture(Texture* tex)
{
  if (!tex) return;

  if (tex->id)
  {
    glDeleteTextures(1, &tex->id);
  }
  delete tex;
}
Font* Font::Load(const std::string& path, int size,Color color)
{
    FT_Library library;
    FT_Face face;
    FT_Error error;

    // 1. 初始化 FreeType
    error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "INFO :Unable to initialize FreeType library\n");
        return nullptr;
    }

    // 2. 加载字体文件
    error = FT_New_Face(library, path.c_str(), 0, &face);
    if (error) {
        fprintf(stderr, "INFO :Unable to load font file: %s\n", path.c_str());
        FT_Done_FreeType(library);
        return nullptr;
    }

    // 3. 设置字体大小
    error = FT_Set_Pixel_Sizes(face, 0, size);
    if (error) {
        fprintf(stderr, "INFO :Unable to set font size\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return nullptr;
    }

    // 4. 创建 Font 对象
    Font* font = new Font();
    font->size=size;
    // 5. 加载 ASCII 字符
    for (int i = 0; i < 128; ++i) {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
            fprintf(stderr, "INFO :Unable to load character: %c (ASCII %d)\n", (char)i, i);
            continue;
        }

        FT_GlyphSlot slot = face->glyph;
        FT_Bitmap bitmap = slot->bitmap;

        int width = bitmap.width;
        int height = bitmap.rows;
        uint8_t* grayBuffer = bitmap.buffer;

        // 转换成 RGBA：白色字体 + 灰度做透明度
        std::vector<uint8_t> rgbaBuffer(width * height * 4);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int i = y * width + x;
                uint8_t alpha = grayBuffer[i];

                rgbaBuffer[i * 4 + 0] = color.r; // R（白色）
                rgbaBuffer[i * 4 + 1] = color.g; // G
                rgbaBuffer[i * 4 + 2] = color.b; // B
                rgbaBuffer[i * 4 + 3] = alpha; // A
            }
        }

        // 创建 OpenGL 纹理
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            rgbaBuffer.data()
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap(GL_TEXTURE_2D);

        // 存储字符信息
        font->glyphs[(char32_t)i] = Glyph{
            static_cast<int>(slot->bitmap_left),
            static_cast<int>(slot->bitmap_top),
            static_cast<int>(slot->advance.x >> 6),
            texture,
            width,
            height
        };
    }

    // 6. 清理
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    // 7. 返回字体
    return font;
}




void Font::Release(Font* font)
{
    if (font)
    {
        // 释放纹理资源
        for (auto& pair : font->glyphs)
        {
            GLuint texture = pair.second.texture;
            glDeleteTextures(1, &texture); // 删除 OpenGL 纹理
        }

        // 删除 Font 对象
        delete font;
    }
}

const Font::Glyph* Font::GetGlyph(char32_t codepoint) const
{
  auto it = glyphs.find(codepoint);
  if (it != glyphs.end()) return &it->second;
  else{

  }
  return nullptr;
}

}  // namespace gfx
