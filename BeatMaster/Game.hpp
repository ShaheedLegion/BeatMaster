#include <algorithm>
#include <map>
#include <sstream>
#include "Renderer.hpp"
#include "Math.hpp"
#include "util.hpp"

namespace game {

static const int _width = 320;
static const int _height = 240;

enum UnitTypes { PLAYER, ENEMY, PROJECTILE };
enum FieldTypes {
  x_pos,
  y_pos,
  delta_x,
  delta_y,
  life,
  firing_rate,
  cooldown,
  type
};
struct texture {
  detail::Uint32 *tex;
  math::vec2i bounds;
  util::mem_pool &m_allocator;

  texture(const math::vec2i &size, util::mem_pool &allocator)
      : bounds(size), tex(nullptr), m_allocator(allocator) {
    tex = reinterpret_cast<detail::Uint32 *>(m_allocator.alloc(
        bounds.v[x_pos] * bounds.v[y_pos] *
        sizeof(detail::Uint32))); // new detail::Uint32[bounds.v[x_pos] *
                                  // bounds.v[y_pos]];
  }

  texture(const texture &other) : m_allocator(other.m_allocator) {
    bounds = other.bounds;
    tex = other.tex;
  }

  texture(const std::string &name, util::mem_pool &allocator)
      : m_allocator(allocator) {
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

    fread(&bounds.v[x_pos], 4, 1, input);
    fread(&bounds.v[y_pos], 4, 1, input);

    std::cout << "Loaded texture resource [" << name << "] w["
              << bounds.v[x_pos] << "] h[" << bounds.v[y_pos] << "]"
              << std::endl;
    int len = bounds.v[x_pos] * bounds.v[y_pos];
    tex = reinterpret_cast<detail::Uint32 *>(
        m_allocator.alloc(len * sizeof(detail::Uint32)));
    // tex = new detail::Uint32[len];
    // Now that we have width and height, we can start reading pixels.
    fread(tex, sizeof(detail::Uint32), len, input);

    fclose(input);
  }

  void copy(const texture &other, int rowOffset = 0) {
    if (!bounds.equals(other.bounds)) {
      if (bounds.v[x_pos] > other.bounds.v[x_pos] ||
          bounds.v[y_pos] > other.bounds.v[y_pos]) {
        std::cout << "Cannot copy dissimilar textures." << std::endl;
        return;
      }
    }

    int numRows = bounds.v[y_pos]; // height = numrows
    int otherRows = other.bounds.v[y_pos];

    int finalOffset = rowOffset % otherRows;

    int rowsFromOther =
        (otherRows - finalOffset > numRows ? numRows : otherRows - finalOffset);
    int wrapRows = (rowsFromOther == numRows ? 0 : numRows - rowsFromOther);

    int len(bounds.v[x_pos] * rowsFromOther);
    util::memcpy(tex, other.tex + (finalOffset * other.bounds.v[x_pos]), len);
    // memcpy(tex, other.tex + (finalOffset * other.bounds.v[x_pos]),
    //       len * sizeof(detail::Uint32));

    if (wrapRows)
      util::memcpy(tex + len, other.tex, (wrapRows * other.bounds.v[x_pos]));
    // memcpy(tex + len, other.tex,
    //       (wrapRows * other.bounds.v[x_pos]) * sizeof(detail::Uint32));
  }

  void clear() {
    int len(bounds.v[x_pos] * bounds.v[y_pos]);
    util::memset(tex, 0, len);
    // memset(tex, 0, len * sizeof(detail::Uint32));
  }

  ~texture() {
    // delete[] tex;
    // Never delete tex - we don't own the memory.
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
  virtual void RenderToBitmap(HDC screenDC, int w, int h) {
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

    TextOut(screenDC, 0, h - 32, m_fps.c_str(), m_fps.length());
    TextOut(screenDC, 0, h - 16, m_ticks.c_str(), m_ticks.length());
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
  double xpf = math::compute_units(_width * 2.0, millis, fps);
  double ypf = math::compute_units(_height * 2.0, millis, fps);
  player.v[firing_rate] -= math::compute_units(400.0, millis, fps);
  if (dir == 0)
    player.v[delta_x] = -xpf;
  if (dir == 2)
    player.v[delta_x] = xpf;
  if (dir == 1)
    player.v[delta_y] = ypf;
  if (dir == 3)
    player.v[delta_y] = -ypf;

  if (dir == -1) {
    dir = -1;
    player.v[delta_x] = 0;
    player.v[delta_y] = 0;
  }
  if (player.v[y_pos] < 32)
    player.v[delta_y] = 1;
}

void handle_enemy_movement(math::vec8 &enemy, const math::vec4 &clip,
                           double millis, double fps) {
  enemy.v[firing_rate] -= math::compute_units(200.0, millis, fps);
  double ypf = math::compute_units(600.0, millis, fps);
  if (enemy.v[life] != 0 && enemy.v[delta_y] == 0)
    enemy.v[delta_y] = -ypf;
  else if (enemy.v[life] == 0) {
    enemy.v[x_pos] =
        (rand() % static_cast<int>(clip.v[delta_x])) + clip.v[x_pos];
    enemy.v[life] = 1;
  } else if (enemy.v[life] != 0 && enemy.v[y_pos] < 16) {
    enemy.v[y_pos] =
        (rand() % static_cast<int>(clip.v[delta_y])) + (clip.v[delta_y]);
  }
}

void handle_projectile_movement(math::vec8 &projectile, const math::vec4 &clip,
                                double millis, double fps) {
  double ypf = math::compute_units(_height, millis, fps);

  projectile.v[life] -=
      math::compute_units(projectile.v[cooldown], millis,
                          fps); // subtract cooldown from life.

  if (projectile.v[life] > 0 && projectile.v[delta_y] == 0)
    projectile.v[delta_y] = -ypf;
}

void fire_projectiles(std::vector<math::vec8> &units, double millis,
                      double fps) {

  double player_x = units[units.size() - 1].v[x_pos];
  // Get list of all references to 'free' projectiles which can be fired. This
  // could be optimized by generating this list during movement update.
  for (int i = 0; i < units.size(); ++i) {
    if (units[i].v[type] == PROJECTILE) { // This is a projectile

      if (units[i].v[life] <= 0) { // must place this unit again
        for (int j = 0; j < units.size(); ++j) {
          // Find an enemy ship that can fire this projectile
          if (units[j].v[type] == ENEMY) {
            if (units[j].v[firing_rate] <= 0) {
              math::vec8 &bullet{units[i]};
              math::vec8 &enemy{units[j]};
              bullet.v[x_pos] = enemy.v[x_pos]; // set x
              bullet.v[y_pos] = enemy.v[y_pos]; // set y
              bullet.v[delta_x] = math::compute_units(
                  (player_x - enemy.v[x_pos]) * 10.0, millis,
                  fps);                                   // Delta->target.
              bullet.v[delta_y] = enemy.v[delta_y] * 2.0; // set speed
              bullet.v[life] =
                  math::compute_units(100, millis, fps); // life left
              enemy.v[firing_rate] = bullet.v[life];
              break; // break out of this loop since we found a candidate.
            }
          } else if (units[j].v[type] == PLAYER) {
            if (units[j].v[firing_rate] <= 0) {
              math::vec8 &bullet{units[i]};
              math::vec8 &player{units[j]};
              bullet.v[x_pos] = player.v[x_pos]; // set x
              bullet.v[y_pos] = player.v[y_pos]; // set y
              bullet.v[delta_x] = 0;             // Delta->target.
              bullet.v[delta_y] = math::compute_units(_height, millis, fps);
              bullet.v[life] =
                  math::compute_units(100, millis, fps); // life left
              player.v[firing_rate] = bullet.v[life];
              break; // break out of this loop since we found a candidate.
            }
          }
        }
      }
    }
  }
  // Search through the enemies and try to find one with the correct cooldown
  // time so that it can fire a projectile.

  // Plot a projectile from the 'free' list at the enemy position (update
  // deltas) and remove it from the 'free' list.
}

void draw_units(const std::vector<texture> &tex, texture &fg,
                std::vector<math::vec8> &units, double millis, double fps,
                int dir) {
  math::vec4 clip{8.0, 8.0, fg.bounds.v[x_pos] - 8.0, fg.bounds.v[y_pos] - 8.0};

  if (units.empty()) {
    srand(2635);
    // Define what we need in each unit.
    // 0 = x pos, 1 = y pos, 2 = delta x, 3 = delta y, 4 = alive, 5 = firing
    // rate, 6 = cooldown, 7 = type
    for (int i = 0; i < 100; ++i)
      units.push_back(math::vec8(
          (rand() % static_cast<int>(clip.v[delta_x])) + clip.v[x_pos],
          (rand() % static_cast<int>(clip.v[delta_y])) + clip.v[y_pos], 0, 0, 1,
          0, 60, 2));

    for (int i = 0; i < 10; ++i)
      units.push_back(math::vec8(
          (rand() % static_cast<int>(clip.v[delta_x])) + clip.v[x_pos],
          (rand() % static_cast<int>(clip.v[delta_y])) + clip.v[y_pos], 0, 0, 1,
          1, 0, 1));

    units.push_back(math::vec8(fg.bounds.v[x_pos] / 2, fg.bounds.v[y_pos] / 2,
                               0, 0, 1, 3, 1, 0));
  }

  math::vec8 &player = units[units.size() - 1];
  handle_player_movement(player, dir, millis, fps);

  for (auto &i : units) {
    i.v[x_pos] += i.v[delta_x];
    i.v[y_pos] += i.v[delta_y];

    if (i.v[type] == ENEMY)
      handle_enemy_movement(i, clip, millis, fps);
    else if (i.v[type] == PROJECTILE)
      handle_projectile_movement(i, clip, millis, fps);

    // For all items, we perform clipping.
    if (i.v[x_pos] < clip.v[x_pos])
      i.v[x_pos] = clip.v[x_pos];
    if (i.v[x_pos] > clip.v[delta_x])
      i.v[x_pos] = clip.v[delta_x];

    const texture &item = tex[static_cast<int>(i.v[type])];
    int _y = 0;
    for (int y = static_cast<int>(i.v[y_pos] - (item.bounds.v[y_pos] / 2));
         y < static_cast<int>(i.v[y_pos] + (item.bounds.v[y_pos] / 2)); ++y) {
      int _x = 0;
      for (int x = static_cast<int>(i.v[x_pos] - (item.bounds.v[x_pos] / 2));
           x < static_cast<int>(i.v[x_pos] + (item.bounds.v[x_pos] / 2)); ++x) {
        // Check if the pixel is visible on the screen.
        if (y >= 0 && x >= 0 && x < fg.bounds.v[x_pos] &&
            y < fg.bounds.v[y_pos]) {
          detail::Uint32 col = item.tex[_y * item.bounds.v[x_pos] + _x];
          if (col)
            fg.tex[y * fg.bounds.v[x_pos] + x] = col;
        }
        ++_x;
      }
      ++_y;
    }
  }
  fire_projectiles(units, millis, fps);
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
  for (int y = 0; y < t.bounds.v[y_pos]; ++y) {
    for (int x = 0; x < t.bounds.v[x_pos] - 4; ++x) {
      t.tex[y * t.bounds.v[x_pos] + x] =
          blend_color(blend_color(t.tex[y * t.bounds.v[x_pos] + x + 0],
                                  t.tex[y * t.bounds.v[x_pos] + x + 1]),
                      blend_color(t.tex[y * t.bounds.v[x_pos] + x + 2],
                                  t.tex[y * t.bounds.v[x_pos] + x + 3]));
    }
  }

  // Blend vertically
  for (int x = 0; x < t.bounds.v[x_pos]; ++x) {
    for (int y = 0; y < t.bounds.v[y_pos] - 4; ++y) {
      t.tex[y * t.bounds.v[x_pos] + x] =
          blend_color(blend_color(t.tex[(y + 0) * t.bounds.v[x_pos] + x],
                                  t.tex[(y + 1) * t.bounds.v[x_pos] + x]),
                      blend_color(t.tex[(y + 2) * t.bounds.v[x_pos] + x],
                                  t.tex[(y + 3) * t.bounds.v[x_pos] + x]));
    }
  }
}

void compute_shadows(texture &fg, texture &sg, const math::vec3 &light) {
  // place the fg somewhere between the 'origin' and the light source.
  double fg_z = 40.0;
  double start_x = light.v[x_pos];
  double start_y = light.v[y_pos];
  double delta_z = light.v[delta_x] - fg_z;

  for (int y = 0; y < fg.bounds.v[y_pos]; ++y) {
    for (int x = 0; x < fg.bounds.v[x_pos]; ++x) {
      // compute the gradients here.
      // First check if we are going to hit something on the image buffer.
      if (fg.tex[(y * fg.bounds.v[x_pos]) + x]) {
        double delta_x = static_cast<double>(x) - light.v[x_pos];
        double delta_y = static_cast<double>(y) - light.v[y_pos];

        // Now get number of steps
        double x_step = delta_x / delta_z;
        double y_step = delta_y / delta_z;

        double end_x = start_x + (x_step * (delta_z + fg_z));
        double end_y = start_y + (y_step * (delta_z + fg_z));

        int x_idx = static_cast<int>(end_x);
        int y_idx = static_cast<int>(end_y);
        if (x_idx >= 0 && x_idx < fg.bounds.v[x_pos] && y_idx >= 0 &&
            y_idx < fg.bounds.v[y_pos]) {
          sg.tex[y_idx * fg.bounds.v[x_pos] + x_idx] = 0xff222222;
        }
      }
    }
  }

  // Naively blur the shadow map to remove artifacts
  blur_texture(sg);
}

void draw_stage(detail::Uint32 *buffer, const math::vec2 &iResolution,
                texture &bg, texture &sg, texture &fg, texture &bar,
                double millis, int dir) {
  double ratio_x =
      static_cast<double>(bg.bounds.v[x_pos]) / iResolution.v[x_pos];
  double ratio_y =
      static_cast<double>(bg.bounds.v[y_pos]) / iResolution.v[y_pos];

  double current_x = 0;
  double current_y = 0;

  int width = static_cast<int>(iResolution.v[x_pos]);
  int height = static_cast<int>(iResolution.v[y_pos]);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx = static_cast<int>(current_y) * bg.bounds.v[x_pos] +
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

  {
    ratio_x = static_cast<double>(bar.bounds.v[x_pos]) / iResolution.v[x_pos];
    current_x = 0;
    current_y = 0;
    // Simply overwrite whatever has been drawn already and draw our HUD on it.
    for (int y = 0; y < ((bar.bounds.v[y_pos] / ratio_y)); ++y) {
      for (int x = 0; x < width; ++x) {
        int idx = static_cast<int>(current_y) * bar.bounds.v[x_pos] +
                  static_cast<int>(current_x);
        buffer[y * width + x] = bar.tex[idx];
        current_x += ratio_x;
      }
      current_y += ratio_y;
      current_x = 0;
    }
  }
}

#if 0
TODO(shaheed.abdol) - Treat projectiles as 'dead' until we need to fire them.
                      Treat projectiles as alive until they hit an obstacle.
					  Some nice 'explosion' animations would be awesome.
#endif // 0
} // namespace game