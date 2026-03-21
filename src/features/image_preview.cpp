#include "features/image_preview.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string_view>
#include <unistd.h>

#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
#include <chafa.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "dsa/stb_image.h"

namespace fs = std::filesystem;

namespace pnana {
namespace features {

ImagePreview::ImagePreview()
    : loaded_(false), image_width_(0), image_height_(0), render_width_(0), render_height_(0) {}

ImagePreview::~ImagePreview() {
    clear();
}

bool ImagePreview::isImageFile(const std::string& filepath) {
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp" ||
           ext == ".webp";
}

bool ImagePreview::isSupported() {
    // 双后端模式：始终支持图片预览
    // Chafa 可用时使用高质量渲染，否则使用块状字符渲染
    return true;
}

bool ImagePreview::isChafaAvailable() {
#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
    return true;
#else
    return false;
#endif
}

bool ImagePreview::loadImage(const std::string& filepath, int width, int max_height) {
    clear();

    if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
        return false;
    }

    const int MAX_PREVIEW_WIDTH = 300;
    const int MAX_PREVIEW_HEIGHT = 150;
    if (width > MAX_PREVIEW_WIDTH) {
        width = MAX_PREVIEW_WIDTH;
    }
    if (max_height > 0 && max_height > MAX_PREVIEW_HEIGHT) {
        max_height = MAX_PREVIEW_HEIGHT;
    }

    int image_width = 0;
    int image_height = 0;
    int channels = 0;
    unsigned char* image_data =
        stbi_load(filepath.c_str(), &image_width, &image_height, &channels, STBI_rgb_alpha);
    if (!image_data) {
        const char* reason = stbi_failure_reason();
        LOG_ERROR("ImagePreview::loadImage() - stb_image decode failed: " +
                  std::string(reason ? reason : "unknown error"));
        stbi_image_free(image_data);
        return false;
    }

    image_width_ = image_width;
    image_height_ = image_height;
    image_path_ = filepath;

    float scale = static_cast<float>(width) / image_width;
    int new_height = static_cast<int>(image_height * scale * 0.6f);

    if (max_height > 0 && new_height > max_height) {
        new_height = max_height;
        scale = static_cast<float>(new_height) / (image_height * 0.6f);
        width = static_cast<int>(image_width * scale);
        if (width > MAX_PREVIEW_WIDTH) {
            width = MAX_PREVIEW_WIDTH;
            scale = static_cast<float>(width) / image_width;
            new_height = static_cast<int>(image_height * scale * 0.6f);
        }
    } else if (new_height > MAX_PREVIEW_HEIGHT) {
        new_height = MAX_PREVIEW_HEIGHT;
        scale = static_cast<float>(new_height) / (image_height * 0.6f);
        width = static_cast<int>(image_width * scale);
    }

    if (new_height <= 0) {
        new_height = 1;
    }
    if (width <= 0) {
        width = 1;
    }

    render_width_ = width;
    render_height_ = new_height;

    bool success = false;
#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
    // 后端 1：使用 Chafa 进行高质量渲染
    success = loadWithChafa(image_data, image_width, image_height);
#else
    // 后端 2：使用 stb_image + 块状字符进行渲染
    success = loadWithBlockChars(image_data, image_width, image_height);
#endif

    stbi_image_free(image_data);
    return success;
}

#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
bool ImagePreview::loadWithChafa(unsigned char* image_data, int image_width, int image_height) {
    ChafaCanvasConfig* config = chafa_canvas_config_new();
    chafa_canvas_config_set_geometry(config, render_width_, render_height_);
    chafa_canvas_config_set_canvas_mode(config, CHAFA_CANVAS_MODE_TRUECOLOR);
    chafa_canvas_config_set_fg_only_enabled(config, TRUE);
    chafa_canvas_config_set_color_extractor(config, CHAFA_COLOR_EXTRACTOR_AVERAGE);
#if defined(CHAFA_COLOR_SPACE_SRGB)
    chafa_canvas_config_set_color_space(config, CHAFA_COLOR_SPACE_SRGB);
#else
    chafa_canvas_config_set_color_space(config, CHAFA_COLOR_SPACE_RGB);
#endif
    chafa_canvas_config_set_dither_mode(config, CHAFA_DITHER_MODE_DIFFUSION);
    chafa_canvas_config_set_cell_geometry(config, 1, 2);
    chafa_canvas_config_set_pixel_mode(config, CHAFA_PIXEL_MODE_SYMBOLS);

    ChafaSymbolMap* symbol_map = chafa_symbol_map_new();
    chafa_symbol_map_add_by_tags(
        symbol_map, static_cast<ChafaSymbolTags>(CHAFA_SYMBOL_TAG_BLOCK | CHAFA_SYMBOL_TAG_SOLID));
    chafa_canvas_config_set_symbol_map(config, symbol_map);

    ChafaCanvas* canvas = chafa_canvas_new(config);
    chafa_canvas_draw_all_pixels(canvas, CHAFA_PIXEL_RGBA8_UNASSOCIATED, image_data, image_width,
                                 image_height, image_width * 4);

    preview_lines_.clear();
    preview_pixels_.clear();
    preview_pixels_.resize(render_height_);

    for (int y = 0; y < render_height_; ++y) {
        std::string line;
        preview_pixels_[y].resize(render_width_);

        for (int x = 0; x < render_width_; ++x) {
            gunichar ch = chafa_canvas_get_char_at(canvas, x, y);
            gint fg_color = 0;
            chafa_canvas_get_raw_colors_at(canvas, x, y, &fg_color, nullptr);

            PreviewPixel pixel{};
            pixel.r = static_cast<unsigned char>((fg_color >> 16) & 0xFF);
            pixel.g = static_cast<unsigned char>((fg_color >> 8) & 0xFF);
            pixel.b = static_cast<unsigned char>(fg_color & 0xFF);

            char utf8_buffer[8] = {};
            int utf8_length = g_unichar_to_utf8(ch, utf8_buffer);
            if (utf8_length <= 0) {
                pixel.ch = " ";
                line += " ";
            } else {
                pixel.ch.assign(utf8_buffer, static_cast<size_t>(utf8_length));
                line += pixel.ch;
            }

            preview_pixels_[y][x] = pixel;
        }

        preview_lines_.push_back(line);
    }

    chafa_symbol_map_unref(symbol_map);
    chafa_canvas_unref(canvas);
    chafa_canvas_config_unref(config);

    loaded_ = !preview_pixels_.empty();
    return loaded_;
}
#endif

bool ImagePreview::loadWithBlockChars(unsigned char* image_data, int image_width,
                                      int image_height) {
    preview_lines_.clear();
    preview_pixels_.clear();
    preview_pixels_.resize(render_height_);

    const char* block_chars[] = {" ", "░", "▒", "▓", "█"};

    for (int y = 0; y < render_height_; ++y) {
        std::string line;
        preview_pixels_[y].resize(render_width_);

        for (int x = 0; x < render_width_; ++x) {
            int src_x = static_cast<int>(x * image_width / render_width_);
            int src_y = static_cast<int>(y * image_height / render_height_);

            int pixel_index = (src_y * image_width + src_x) * 4;
            unsigned char r = image_data[pixel_index];
            unsigned char g = image_data[pixel_index + 1];
            unsigned char b = image_data[pixel_index + 2];

            float brightness = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
            int char_index = static_cast<int>(brightness * 4.0f);
            if (char_index > 4)
                char_index = 4;

            PreviewPixel pixel{};
            pixel.r = r;
            pixel.g = g;
            pixel.b = b;
            pixel.ch = block_chars[char_index];
            line += pixel.ch;

            preview_pixels_[y][x] = pixel;
        }

        preview_lines_.push_back(line);
    }

    loaded_ = !preview_pixels_.empty();
    return loaded_;
}

void ImagePreview::clear() {
    preview_lines_.clear();
    preview_pixels_.clear();
    loaded_ = false;
    image_width_ = 0;
    image_height_ = 0;
    image_path_.clear();
    render_width_ = 0;
    render_height_ = 0;
}

ftxui::Element ImagePreview::render() const {
    using namespace ftxui;

    if (!loaded_ || preview_pixels_.empty()) {
        return (text("Failed to load image preview") | color(Color::Red) | center) | flex |
               bgcolor(Color::Black);
    }

    Elements preview_rows;

    preview_rows.push_back(hbox({text(std::string(pnana::ui::icons::IMAGE) + " Image Preview: ") |
                                     color(Color::Blue) | bold,
                                 text(image_path_) | color(Color::White)}));

    preview_rows.push_back(
        hbox({text("  Size: ") | color(Color::GrayDark),
              text(std::to_string(image_width_) + "x" + std::to_string(image_height_)) |
                  color(Color::White)}));

    preview_rows.push_back(separator());

    for (size_t i = 0; i < preview_pixels_.size(); ++i) {
        Elements pixel_elements;
        const auto& row = preview_pixels_[i];

        for (size_t j = 0; j < row.size(); ++j) {
            const auto& pixel = row[j];
            ftxui::Color pixel_color = Color::RGB(pixel.r, pixel.g, pixel.b);
            pixel_elements.push_back(text(pixel.ch) | color(pixel_color));
        }

        preview_rows.push_back(hbox(pixel_elements));
    }

    // Center the preview block within the available editor (code area) space.
    // `flex` ensures we occupy the available space, `center` positions the content.
    return (vbox(preview_rows) | center) | flex | bgcolor(Color::Black);
}

} // namespace features
} // namespace pnana
