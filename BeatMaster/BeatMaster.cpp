// BeatMaster.cpp : Contains rendering functions for application.
// Shaheed Abdol - 2015.
#include "Game.hpp"
#include <chrono>

// This is the guts of the renderer, without this it will do nothing.
DWORD WINAPI Update(LPVOID lpParameter) {
  auto start_time = std::chrono::high_resolution_clock::now();
  srand(2635);
  Renderer *g_renderer = static_cast<Renderer *>(lpParameter);
  game::BitmapRenderer *bmp =
      static_cast<game::BitmapRenderer *>(g_renderer->screen.GetRenderer());

  util::mem_pool& pool{g_renderer->screen.GetAllocator()};

  math::vec2 iResolution(static_cast<double>(g_renderer->screen.GetWidth()),
                         static_cast<double>(g_renderer->screen.GetHeight()));
  math::vec3 light(game::_width * 0.5, game::_height * 0.5, 240.0);
  std::vector<math::vec8> units;

  std::vector<game::texture> textures;
  textures.push_back(game::texture("..//res//player.graw", pool));
  textures.push_back(game::texture("..//res//enemy.graw", pool));
  textures.push_back(game::texture("..//res//projectile.graw", pool));

  int offset = 0;

  game::texture bg("..//res//bg[0].graw", pool);
  game::texture bar("../res//bar.graw", pool);
  game::texture img(math::vec2i(game::_width, game::_height), pool);
  game::texture fg(math::vec2i(game::_width, game::_height), pool);
  game::texture sg(math::vec2i(game::_width, game::_height), pool);

  auto end_time = std::chrono::high_resolution_clock::now();
  while (g_renderer->IsRunning()) {
    auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    start_time = std::chrono::high_resolution_clock::now();
    auto millis = std::chrono::duration<double, std::milli>(ticks).count();
    int dir = bmp ? bmp->GetDirection() : -1;
    detail::Uint32 *buffer = g_renderer->screen.GetPixels();

    img.copy(bg, offset++); // copy the background onto the image.

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

    g_renderer->screen.Flip(true);
    g_renderer->updateThread.Delay(1);
    end_time = std::chrono::high_resolution_clock::now();
  }

  return 0;
}

int main(int argc, char *argv[]) {

#if UNIT_TEST
  for (int j = 0; j < 2; ++j) {
    auto start_time = std::chrono::high_resolution_clock::now();
    Sleep(10);
    auto end_time = std::chrono::high_resolution_clock::now();

    // We want an algorith that figures out the correct delta.
    double startPos = 0;
    double endPos = 100;
    i = 100;
    while (--i) {
      auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
          end_time - start_time);
      startPos += math::compute_delta(endPos, millis, 0);
      Sleep(100);
      end_time = std::chrono::high_resolution_clock::now();
    }
    std::cout << ""
  }
  return 0;
#endif // UNIT_TEST

  game::BitmapRenderer bmp;
  Renderer renderer("BeatMaster", &Update,
                    reinterpret_cast<detail::IBitmapRenderer *>(&bmp));
  return 0;
}