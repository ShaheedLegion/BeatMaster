// BeatMaster.cpp : Contains rendering functions for application.
// Shaheed Abdol - 2015.
#include "Game.hpp"
#include <chrono>

// This is the guts of the renderer, without this it will do nothing.
DWORD WINAPI Update(LPVOID lpParameter) {
  // Seed random number generator.
  srand(2635);

  auto start_time = std::chrono::high_resolution_clock::now();

  Renderer *g_renderer = static_cast<Renderer *>(lpParameter);
  game::BitmapRenderer *bmp =
      static_cast<game::BitmapRenderer *>(g_renderer->screen.GetRenderer());

  // Fetch memory pool - this holds enough memory to be used for other things.
  util::mem_pool &pool{g_renderer->screen.GetAllocator()};

  math::vec2 iResolution(static_cast<double>(g_renderer->screen.GetWidth()),
                         static_cast<double>(g_renderer->screen.GetHeight()));

  // We place a light 'somewhere' in the scene for shadow projection.
  math::vec3 light(game::_width * 0.5, game::_height * 0.5, 240.0);

  std::vector<math::vec8> units;
  std::vector<game::texture> textures;

  textures.push_back(game::texture("..//res//player.graw", pool));
  textures.push_back(game::texture("..//res//enemy.graw", pool));
  textures.push_back(game::texture("..//res//projectile.graw", pool));

  game::texture bg("..//res//bg[0].graw", pool);
  game::texture bar("../res//bar.graw", pool);
  game::texture img(math::vec2i(game::_width, game::_height), pool);
  game::texture fg(math::vec2i(game::_width, game::_height), pool);
  game::texture sg(math::vec2i(game::_width, game::_height), pool);

  int offset = 0;
  auto end_time = std::chrono::high_resolution_clock::now();

  while (g_renderer->IsRunning()) {
    auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    start_time = std::chrono::high_resolution_clock::now();
    auto millis = std::chrono::duration<double, std::milli>(ticks).count();
    int dir = bmp ? bmp->GetDirection() : -1;
    detail::Uint32 *buffer = g_renderer->screen.GetPixels();

    // Copy the background onto the image.
    img.copy(bg, offset++);

    bmp->SetTicks(millis);
    double fps{bmp->GetFPS()};

    // Clear out the foreground texture.
    fg.clear();

    // Next render the entities onto the fg texture.
    game::draw_units(textures, fg, units, millis, fps, dir);

    // Clear the shadow map
    sg.clear();
    // Compute the shadow map from the rendered entities
    game::compute_shadows(fg, sg, light);

    // Composition everything onto the img buffer
    game::draw_stage(buffer, iResolution, img, sg, fg, bar, millis, dir);

    // Flip buffers, and sleep a bit.
    g_renderer->screen.Flip(true);
    g_renderer->updateThread.Delay(1);
    end_time = std::chrono::high_resolution_clock::now();
  }

  return 0;
}

int main(int argc, char *argv[]) {
  game::BitmapRenderer bmp;
  Renderer renderer("BeatMaster", &Update,
                    reinterpret_cast<detail::IBitmapRenderer *>(&bmp));
  return 0;
}