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

  math::vec2 iResolution(static_cast<double>(g_renderer->screen.GetWidth()),
                         static_cast<double>(g_renderer->screen.GetHeight()));
  math::vec3 light(160.0, 120.0, 240.0);
  std::vector<math::vec8> units;

  std::vector<game::texture> textures;
  textures.push_back(game::texture("..//res//player.graw"));
  textures.push_back(game::texture("..//res//enemy.graw"));
  textures.push_back(game::texture("..//res//projectile.graw"));

  int tw = 320;
  int th = 240;
  int offset = 0;
  // This texture is simply the background image.
  game::texture bg("..//res//bg[0].graw");
  game::texture bar("../res//bar.graw");
  game::texture img(math::vec2i(tw, th)); // the final composed image.
  game::texture fg(math::vec2i(tw, th));  // the image containing the sprites.
  game::texture sg(math::vec2i(tw, th));  // the shadow map

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
  game::BitmapRenderer bmp;
  Renderer renderer("BSPTree", &Update,
                    reinterpret_cast<detail::IBitmapRenderer *>(&bmp));
  return 0;
}