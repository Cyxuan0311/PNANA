#ifndef PNANA_FEATURES_IMAGE_PREVIEW_H
#define PNANA_FEATURES_IMAGE_PREVIEW_H

#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>
#include <vector>

#ifdef BUILD_IMAGE_PREVIEW_SUPPORT

namespace pnana {
namespace features {

// 预览像素数据
struct PreviewPixel {
    unsigned char r, g, b;
    std::string ch;
};

// 图片预览器
class ImagePreview {
  public:
    ImagePreview();
    ~ImagePreview();

    // 加载图片并转换为 ASCII 艺术
    // width: 预览宽度（字符数）
    // max_height: 最大预览高度（行数），如果为0则根据宽度自动计算
    bool loadImage(const std::string& filepath, int width = 80, int max_height = 0);

    // 获取预览文本行（包含 ANSI 转义码）
    std::vector<std::string> getPreviewLines() const {
        return preview_lines_;
    }

    // 获取预览数据（用于 FTXUI 渲染）
    std::vector<std::vector<PreviewPixel>> getPreviewPixels() const {
        return preview_pixels_;
    }

    // 检查文件是否是图片
    static bool isImageFile(const std::string& filepath);

    // 检查是否支持图片预览（需要 FFmpeg）
    static bool isSupported();

    // 清空预览
    void clear();

    // 渲染预览（返回 FTXUI 元素）
    ftxui::Element render() const;

    // 是否已加载图片
    bool isLoaded() const {
        return loaded_;
    }

    // 获取图片信息
    int getImageWidth() const {
        return image_width_;
    }
    int getImageHeight() const {
        return image_height_;
    }
    std::string getImagePath() const {
        return image_path_;
    }
    int getRenderWidth() const {
        return render_width_;
    }
    int getRenderHeight() const {
        return render_height_;
    }

  private:
    std::vector<std::string> preview_lines_;                // ANSI 转义码格式
    std::vector<std::vector<PreviewPixel>> preview_pixels_; // 像素数据格式
    bool loaded_;
    int image_width_;
    int image_height_;
    std::string image_path_;
    int render_width_;  // 实际渲染的宽度
    int render_height_; // 实际渲染的高度

    // 将 RGB 转换为灰度
    unsigned char rgbToGray(unsigned char r, unsigned char g, unsigned char b);

    // 获取字符（根据灰度值）
    std::string getCharForGray(unsigned char gray_value);

    // 获取颜色代码（24位真彩色）
    std::string getColorCode(unsigned char r, unsigned char g, unsigned char b);

    // 检测终端是否支持真彩色
    bool detectTrueColorSupport();
};

} // namespace features
} // namespace pnana

#endif // BUILD_IMAGE_PREVIEW_SUPPORT

#endif // PNANA_FEATURES_IMAGE_PREVIEW_H
