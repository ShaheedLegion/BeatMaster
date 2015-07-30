#include <algorithm>
#include <map>
#include <sstream>
#include "Renderer.hpp"
#include "Math.hpp"

namespace game {

struct texture {
  detail::Uint32 *tex;
  math::vec2i bounds;

  texture(const math::vec2i &size) : bounds(size), tex(nullptr) {
    tex = new detail::Uint32[bounds.v[0] * bounds.v[1]];
  }

  texture(const texture &other) {
    bounds = other.bounds;
    tex = new detail::Uint32[bounds.v[0] * bounds.v[1]];
    memcpy(tex, other.tex,
           (bounds.v[0] * bounds.v[1]) * sizeof(detail::Uint32));
  }

  texture(const std::string &name) {
    // Load a texture resource from a file with the given name. First tries
    // searching in whichever folder is the current working directory, if that
    // fails it will try searching in the module folder (path to .exe file)

    // Open the file here.
    FILE *input(0);

    errno_t err(fopen_s(&input, name.c_str(), "rb"));
    if (err != 0) {
      std::cout << "Could not load texture resource " << name << std::endl;
      char file_name[MAX_PATH];
      memset(file_name, 0, MAX_PATH);
      if (GetModuleFileName(NULL, file_name, MAX_PATH) != 0) {
        std::cout << "Module path[" << file_name << "]" << std::endl;

        // Try to load the absolute path.
        std::string fp(file_name);
        std::string::size_type position(fp.find_last_of("\\"));
        if (position != std::string::npos) {
          // Append the file name to the absolute path.
          std::string path(fp.substr(0, position + 1));
          path.append(name);
          std::cout << "Trying to load from [" << path << "]" << std::endl;
          if (fopen_s(&input, path.c_str(), "rb") != 0) {
            std::cout << "Load failed." << std::endl;
            return; // could not open the file.
          }
        } else
          return; // could not find the slashes.
      } else
        return; // could not get the path to the exe.
    }

    fread(&bounds.v[0], 4, 1, input);
    fread(&bounds.v[1], 4, 1, input);

    std::cout << "Loaded texture resource [" << name << "] w[" << bounds.v[0]
              << "] h[" << bounds.v[1] << "]" << std::endl;
    int len = bounds.v[0] * bounds.v[1];
    tex = new detail::Uint32[len];
    // Now that we have width and height, we can start reading pixels.
    fread(tex, sizeof(detail::Uint32), len, input);

    fclose(input);
  }

  void copy(const texture &other, int rowOffset = 0) {
    if (!bounds.equals(other.bounds)) {
      if (bounds.v[0] > other.bounds.v[0] || bounds.v[1] > other.bounds.v[1]) {
        std::cout << "Cannot copy dissimilar textures." << std::endl;
        return;
      }
    }

    int numRows = bounds.v[1]; // height = numrows
    int otherRows = other.bounds.v[1];

    int finalOffset = rowOffset % otherRows;

    int rowsFromOther =
        (otherRows - finalOffset > numRows ? numRows : otherRows - finalOffset);
    int wrapRows = (rowsFromOther == numRows ? 0 : numRows - rowsFromOther);

    int len(bounds.v[0] * rowsFromOther);
    memcpy(tex, other.tex + (finalOffset * other.bounds.v[0]),
           len * sizeof(detail::Uint32));

    if (wrapRows)
      memcpy(tex + len, other.tex,
             (wrapRows * other.bounds.v[0]) * sizeof(detail::Uint32));
  }

  void clear() {
    int len(bounds.v[0] * bounds.v[1]);
    memset(tex, 0, len * sizeof(detail::Uint32));
  }

  ~texture() {
    delete[] tex;
    tex = nullptr;
  }
};

class BitmapRenderer : public detail::IBitmapRenderer {
public:
  BitmapRenderer()
      : m_direction(-1), m_elapsedFrames(0), m_framesPerSecond(0),
        m_startTime(GetTickCount()), m_fps("FPS: 0") {}
  virtual ~BitmapRenderer() {}

  // Useful for printing stats (like fps/score)
  virtual void RenderToBitmap(HDC screenDC) {
    // Will deal with getting the fps data here later.
    ++m_elapsedFrames;
    if (GetTickCount() - m_startTime > 1000) {
      // Fill our string with the elapsed frames.
      std::stringstream ss;
      ss << "FPS: " << m_elapsedFrames;
      m_fps = ss.str();
      ss.str("");
      ss << "MPF: " << m_currentMillis;
      m_ticks = ss.str();
      m_framesPerSecond = m_elapsedFrames;
      m_elapsedFrames = 0;
      m_startTime = GetTickCount();
    }

    TextOut(screenDC, 0, 0, m_fps.c_str(), m_fps.length());
    TextOut(screenDC, 0, 12, m_ticks.c_str(), m_ticks.length());
  }

  virtual void HandleOutput(VOID *output) {}
  virtual void HandleDirection(int direction) { m_direction = direction; }
  void SetTicks(double millis) { m_currentMillis = millis; }
  int GetDirection() const { return m_direction; }
  double GetFPS() const { return m_framesPerSecond; }

protected:
  volatile int m_direction;
  double m_elapsedFrames;
  double m_framesPerSecond;
  double m_currentMillis;
  DWORD m_startTime;
  std::string m_fps;
  std::string m_ticks;
};

void handle_player_movement(math::vec8 &player, int dir, double millis,
                            double fps) {
  double xpf = math::compute_units(320.0, millis, fps);
  double ypf = math::compute_units(240.0, millis, fps);
  if (dir == -1) {
    dir = -1;
    player.v[2] = 0;
    player.v[3] = 0;
  }

  if (dir == 0)
    player.v[2] = -xpf;
  if (dir == 2)
    player.v[2] = xpf;
  if (dir == 1)
    player.v[3] = ypf;
  if (dir == 3)
    player.v[3] = -ypf;
}

void handle_enemy_movement(math::vec8 &enemy, double millis, double fps) {}

void draw_units(const std::vector<texture> &tex, texture &fg,
                std::vector<math::vec8> &units, double millis, double fps,
                int dir) {
  math::vec4 clip{8.0, 8.0, fg.bounds.v[0] - 8.0, fg.bounds.v[1] - 8.0};

  if (units.empty()) {

    srand(2635);
    // Define what we need in each unit.
    // 0 = x pos, 1 = y pos, 2 = delta x, 3 = delta y, 4 = alive, 5 = firing
    // rate, 6 = cooldown, 7 = type
    for (int i = 0; i < 10; ++i)
      units.push_back(
          math::vec8((rand() % static_cast<int>(clip.v[2])) + clip.v[0],
                     (rand() % static_cast<int>(clip.v[3])) + clip.v[1], 0, 0,
                     1, 1, 0, 1));

    for (int i = 0; i < 10; ++i)
      units.push_back(
          math::vec8((rand() % static_cast<int>(clip.v[2])) + clip.v[0],
                     (rand() % static_cast<int>(clip.v[3])) + clip.v[1], 0, 0,
                     1, 0, 0, 2));

    units.push_back(
        math::vec8(fg.bounds.v[0] / 2, fg.bounds.v[1] / 2, 0, 0, 1, 3, 0, 0));
  }

  math::vec8 &player = units[units.size() - 1];
  handle_player_movement(player, dir, millis, fps);

  for (auto &i : units) {
    i.v[0] += i.v[2];
    i.v[1] += i.v[3];

    if (i.v[7] == 1.0)
      handle_enemy_movement(i, millis, fps);
#if 0
    if (i.v[0] < 1 || i.v[0] > (fg.bounds.v[0] - 1) || i.v[1] < 1 ||
        i.v[1] > (fg.bounds.v[1] - 1) || i.v[2] == 0 || i.v[3] == 0) {
      i.v[0] = rand() % fg.bounds.v[0];
      i.v[1] = rand() % fg.bounds.v[1];
      i.v[2] = rand() % 4 - 2;
      i.v[3] = rand() % 4 - 2;
      continue;
    }
#endif // 0

    // For all items, we perform clipping.
    if (i.v[0] < clip.v[0])
      i.v[0] = clip.v[0];
    if (i.v[0] > clip.v[2])
      i.v[0] = clip.v[2];
    if (i.v[1] < clip.v[1])
      i.v[1] = clip.v[1];
    if (i.v[1] > clip.v[3])
      i.v[1] = clip.v[3];

    const texture &item = tex[static_cast<int>(i.v[7])];
    int _y = 0;
    for (int y = static_cast<int>(i.v[1] - (item.bounds.v[1] / 2));
         y < static_cast<int>(i.v[1] + (item.bounds.v[1] / 2)); ++y) {
      int _x = 0;
      for (int x = static_cast<int>(i.v[0] - (item.bounds.v[0] / 2));
           x < static_cast<int>(i.v[0] + (item.bounds.v[0] / 2)); ++x) {
        detail::Uint32 col = item.tex[_y * item.bounds.v[0] + _x];
        if (col)
          fg.tex[y * fg.bounds.v[0] + x] = col;
        ++_x;
      }
      ++_y;
    }
  }
}

inline detail::Uint32 blend_color(detail::Uint32 a, detail::Uint32 b) {
  // Blend colors on a per-component basis.
  if (a == 0)
    return b; // shortcut

  unsigned char aa = (unsigned char)(a >> 24);
  unsigned char ar = (unsigned char)((a & 0x00ff0000) >> 16);
  unsigned char ag = (unsigned char)((a & 0x0000ff00) >> 8);
  unsigned char ab = (unsigned char)((a & 0x000000ff));

  unsigned char ba = (unsigned char)(b >> 24);
  unsigned char br = (unsigned char)((b & 0x00ff0000) >> 16);
  unsigned char bg = (unsigned char)((b & 0x0000ff00) >> 8);
  unsigned char bb = (unsigned char)((b & 0x000000ff));

  unsigned char fa = (aa >> 1) + (ba >> 1);
  unsigned char fr = (ar >> 1) + (br >> 1);
  unsigned char fg = (ag >> 1) + (bg >> 1);
  unsigned char fb = (ab >> 1) + (bb >> 1);

  return static_cast<detail::Uint32>(
      static_cast<detail::Uint32>(static_cast<unsigned char>(fa)) << 24 |
      static_cast<detail::Uint32>(static_cast<unsigned char>(fr)) << 16 |
      static_cast<detail::Uint32>(static_cast<unsigned char>(fg)) << 8 |
      static_cast<detail::Uint32>(static_cast<unsigned char>(fb)));
}

void blur_texture(texture &t) {
  // Blend horizontally
  for (int y = 0; y < t.bounds.v[1]; ++y) {
    for (int x = 0; x < t.bounds.v[0] - 4; ++x) {
      t.tex[y * t.bounds.v[0] + x] =
          blend_color(blend_color(t.tex[y * t.bounds.v[0] + x + 0],
                                  t.tex[y * t.bounds.v[0] + x + 1]),
                      blend_color(t.tex[y * t.bounds.v[0] + x + 2],
                                  t.tex[y * t.bounds.v[0] + x + 3]));
    }
  }

  // Blend vertically
  for (int x = 0; x < t.bounds.v[0]; ++x) {
    for (int y = 0; y < t.bounds.v[1] - 4; ++y) {
      t.tex[y * t.bounds.v[0] + x] =
          blend_color(blend_color(t.tex[(y + 0) * t.bounds.v[0] + x],
                                  t.tex[(y + 1) * t.bounds.v[0] + x]),
                      blend_color(t.tex[(y + 2) * t.bounds.v[0] + x],
                                  t.tex[(y + 3) * t.bounds.v[0] + x]));
    }
  }
}

void compute_shadows(texture &fg, texture &sg, const math::vec3 &light) {
  // place the fg somewhere between the 'origin' and the light source.
  double fg_z = 40.0;
  double start_x = light.v[0];
  double start_y = light.v[1];
  double delta_z = light.v[2] - fg_z;

  for (int y = 0; y < fg.bounds.v[1]; ++y) {
    for (int x = 0; x < fg.bounds.v[0]; ++x) {
      // compute the gradients here.
      // First check if we are going to hit something on the image buffer.
      if (fg.tex[(y * fg.bounds.v[0]) + x]) {
        double delta_x = static_cast<double>(x) - light.v[0];
        double delta_y = static_cast<double>(y) - light.v[1];

        // Now get number of steps
        double x_step = delta_x / delta_z;
        double y_step = delta_y / delta_z;

        double end_x = start_x + (x_step * (delta_z + fg_z));
        double end_y = start_y + (y_step * (delta_z + fg_z));

        int x_idx = static_cast<int>(end_x);
        int y_idx = static_cast<int>(end_y);
        if (x_idx >= 0 && x_idx < fg.bounds.v[0] && y_idx >= 0 &&
            y_idx < fg.bounds.v[1]) {
          sg.tex[y_idx * fg.bounds.v[0] + x_idx] = 0xff222222;
        }
      }
    }
  }

  // Naively blur the shadow map to remove artifacts
  blur_texture(sg);
}

void draw_stage(detail::Uint32 *buffer, const math::vec2 &iResolution,
                texture &bg, texture &sg, texture &fg, double millis, int dir) {
  double ratio_x = static_cast<double>(bg.bounds.v[0]) / iResolution.v[0];
  double ratio_y = static_cast<double>(bg.bounds.v[1]) / iResolution.v[1];

  double current_x = 0;
  double current_y = 0;

  int width = static_cast<int>(iResolution.v[0]);
  int height = static_cast<int>(iResolution.v[1]);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx = static_cast<int>(current_y) * bg.bounds.v[0] +
                static_cast<int>(current_x);
      detail::Uint32 bgi = bg.tex[idx]; // background (stage)
      detail::Uint32 sgi = sg.tex[idx]; // shadow map (fg)
      detail::Uint32 fgi = fg.tex[idx];
      detail::Uint32 result = bgi;
      result = blend_color(sgi, result);
      result = (fgi ? fgi : result);
      buffer[y * width + x] = result;
      current_x += ratio_x;
    }
    current_y += ratio_y;
    current_x = 0;
  }
}

} // namespace game