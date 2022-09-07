#include "PPU466.hpp"

#include <glm/glm.hpp>

#include "data_path.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "PlayMode.hpp"

#include <algorithm>
#include <filesystem> // https://en.cppreference.com/w/cpp/filesystem
#include <fstream>
#include <iostream>
#include <vector>

#define MAX_COLOR_PER_PALETTE 4
#define MAX_PALETTES_PER_TABLE 8
#define MAX_TILES_PER_TABLE 16*16

void read_png(std::string file_path, 
              std::vector<PPU466::Palette> *palette_table, uint32_t *palette_index, std::vector<uint8_t> *palette_color_count,
              std::vector<PPU466::Tile> *tile_table, uint32_t *tile_index,
              std::vector<PlayMode::LoadedSprite> *sprite_table, uint32_t *sprite_index) {
    
    glm::uvec2 file_size = glm::uvec2();
    std::vector< glm::u8vec4 > data = std::vector< glm::u8vec4 >();

    load_png(file_path, &file_size, &data, LowerLeftOrigin);

    uint8_t color_count = 0;
    PPU466::Palette palette = PPU466::Palette();

    uint8_t new_palette_index = *palette_index;

    for (auto pixel : data) {
        if (std::find(palette.begin(), palette.begin() + color_count, pixel) == palette.begin() + color_count) {
            palette[color_count] = glm::u8vec4(pixel[0], pixel[1], pixel[2], pixel[3]);
            color_count++;
        }
        if (color_count > MAX_COLOR_PER_PALETTE) {
            throw std::runtime_error("Too many colors in sprite!");
        }
    }

    // Try and combine with previous to save palette space
    auto is_combinable = [color_count](uint8_t i){ return color_count + i <= MAX_COLOR_PER_PALETTE; };
    auto find_itr = std::find_if((*palette_color_count).begin(), (*palette_color_count).end(), is_combinable);
    if (find_itr != (*palette_color_count).end()) {
        new_palette_index = std::distance((*palette_color_count).begin(), find_itr);

        // Combine colors
        uint8_t existing_color_count = (*palette_color_count)[new_palette_index];
        for (uint8_t i = 0; i < color_count; i++) {
            (*palette_table)[new_palette_index][existing_color_count + i] = palette[i];
        }
        palette = (*palette_table)[new_palette_index];

        // Update color count
        (*palette_color_count)[new_palette_index] += color_count;
    } else {
        palette_color_count->push_back(color_count);
    }

    if (new_palette_index >= MAX_PALETTES_PER_TABLE) {
        throw std::runtime_error("Too many palettes in table!");
    }

    

    PlayMode::LoadedSprite loaded_sprite = PlayMode::LoadedSprite();

    // Using a similar idea as in data_path.cpp to get the sprite name without path or extension
    loaded_sprite.name = file_path.substr(file_path.rfind("/") + 1, file_path.rfind("."));
    loaded_sprite.name = loaded_sprite.name.substr(0, loaded_sprite.name.rfind("."));

    loaded_sprite.sprites = std::array<PPU466::Sprite, MAX_SPRITES_IN_LOADEDSPRITE>();
    uint8_t loaded_sprite_index = 0;

    for (uint r = 0; r < file_size.y; r += 8) {
        for (uint c = 0; c < file_size.x; c += 8) {
            // Start of a new tile
            PPU466::Tile tile = PPU466::Tile();

            for (uint8_t r_i = 0; r_i < 8; r_i++) {
                uint8_t bit0 = 0;
                uint8_t bit1 = 0;

                for (uint8_t c_i = 0; c_i < 8; c_i++) {
                    uint row = r + r_i;
                    uint col = c + c_i;

                    if (row >= file_size.y || col >= file_size.x) {
                        // No need to fill
                        break;
                    }

                    uint data_index = row * file_size.x + col;
                    glm::u8vec4 pixel = data[data_index];

                    auto find_itr = std::find(palette.begin(), palette.end(), pixel);
                    uint8_t color_index = std::distance(palette.begin(), find_itr);

                    bit0 |= ((color_index & 1) << c_i);
                    bit1 |= ((color_index & 2) >> 1 << c_i);
                }

                tile.bit0[r_i] = bit0;
                tile.bit1[r_i] = bit1;
            }
            
            if (*tile_index >= MAX_TILES_PER_TABLE) {
                throw std::runtime_error("Too many tiles in table!");
            }

            PPU466::Sprite sprite = PPU466::Sprite();
            sprite.x = c;
            sprite.y = r;
            sprite.index = *tile_index;
            sprite.attributes = new_palette_index;

            if (loaded_sprite_index > MAX_SPRITES_IN_LOADEDSPRITE) {
                throw std::runtime_error("This sprite is too large!");
            }
            loaded_sprite.sprites[loaded_sprite_index] = sprite;
            loaded_sprite_index++;

            (*tile_table).push_back(tile);
            (*tile_index)++;
        }
    }

    (*sprite_table).push_back(loaded_sprite);
    (*sprite_index)++;

    if (new_palette_index == *palette_index) {
        (*palette_table).push_back(palette);
        (*palette_index)++;
    }

    std::cout << "PNG Asset " << loaded_sprite.name << " saved\n";
}

int main(int argc, char **argv) {
    // Using same wrapper as starter code in main.cpp for Windows error handling
#ifdef _WIN32
	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif
    std::vector< uint8_t > palette_color_count = std::vector< uint8_t >();
    std::vector< PPU466::Palette > palette_table = std::vector< PPU466::Palette >();
    std::vector< PPU466::Tile > tile_table = std::vector< PPU466::Tile >();
    std::vector< PlayMode::LoadedSprite > sprite_table = std::vector< PlayMode::LoadedSprite >();

    [[maybe_unused]] uint32_t palette_index = 0;
    [[maybe_unused]] uint32_t tile_index = 0;
    [[maybe_unused]] uint32_t sprite_index = 0;


    // Referenced sample code in documentation https://en.cppreference.com/w/cpp/filesystem/directory_iterator
#ifdef __MACOSX__
    for (auto const& dir_entry : std::__fs::filesystem::directory_iterator{ data_path("") }) {
        if (dir_entry.path().extension() == ".png") {
            read_png(dir_entry.path(), 
                     &palette_table, &palette_index, &palette_color_count,
                     &tile_table, &tile_index, 
                     &sprite_table, &sprite_index);
        }
    }
#else
    for (auto const& dir_entry : std::filesystem::directory_iterator{ data_path("") }) {
        if (dir_entry.path().extension() == ".png") {
            read_png(dir_entry.path(), 
                     &palette_table, &palette_index, &palette_color_count,
                     &tile_table, &tile_index, 
                     &sprite_table, &sprite_index);
        }
    }
#endif

    // Referenced documentation for ostream constructor https://cplusplus.com/reference/ostream/ostream/ostream/
    std::filebuf fb;
    fb.open(data_path("../dist/resources/palette_table.chunk"), std::ios::out);
    std::ostream palette_os(&fb);
    write_chunk("pltt", palette_table, &palette_os);
    fb.close();

    fb.open(data_path("../dist/resources/tile_table.chunk"), std::ios::out);
    std::ostream tile_os(&fb);
    write_chunk("tile", tile_table, &tile_os);
    fb.close();

    fb.open(data_path("../dist/resources/game_sprites.chunk"), std::ios::out);
    std::ostream sprite_os(&fb);
    write_chunk("sprt", sprite_table, &sprite_os);
    fb.close();

	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}