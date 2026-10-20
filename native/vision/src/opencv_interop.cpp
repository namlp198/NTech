#include "opencv_interop.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nvision::vision::interop {

int opencv_type_of(const acquisition::PixelFormat format) {
    switch (format) {
    case acquisition::PixelFormat::mono8:
    case acquisition::PixelFormat::bayer_gr8:
        return CV_8UC1;
    case acquisition::PixelFormat::mono16:
        return CV_16UC1;
    case acquisition::PixelFormat::rgb8:
    case acquisition::PixelFormat::bgr8:
    case acquisition::PixelFormat::hsv8:
        return CV_8UC3;
    case acquisition::PixelFormat::mono_float32:
        return CV_32FC1;
    }

    throw std::invalid_argument("pixel format has no OpenCV equivalent");
}

cv::Mat borrow(const acquisition::ImageView& view) {
    const acquisition::ImageSource& source = view.source();
    const std::size_t pixel_size = acquisition::bytes_per_pixel(source.pixel_format());
    const std::size_t offset = static_cast<std::size_t>(view.origin_y()) * source.stride_bytes() +
                               static_cast<std::size_t>(view.origin_x()) * pixel_size;

    // A view is always a rectangle inside its source raster, so the source stride is a
    // valid OpenCV step and no row has to be copied to make the window contiguous.
    return cv::Mat(
        static_cast<int>(view.height()),
        static_cast<int>(view.width()),
        opencv_type_of(source.pixel_format()),
        const_cast<std::uint8_t*>(source.bytes().data() + offset),
        static_cast<std::size_t>(source.stride_bytes()));
}

acquisition::ImageLease materialize(
    core::ImageId id,
    const cv::Mat& produced,
    const acquisition::PixelFormat format) {
    if (produced.empty()) {
        throw std::invalid_argument("an empty OpenCV matrix cannot become an image");
    }
    if (produced.dims != 2) {
        throw std::invalid_argument("only a two-dimensional OpenCV matrix can become an image");
    }
    if (produced.type() != opencv_type_of(format)) {
        throw std::invalid_argument("the produced matrix does not match its declared pixel format");
    }

    const auto width = static_cast<std::uint32_t>(produced.cols);
    const auto height = static_cast<std::uint32_t>(produced.rows);
    const std::size_t row_bytes = static_cast<std::size_t>(produced.cols) * produced.elemSize();

    std::vector<std::uint8_t> bytes(row_bytes * height);
    for (std::uint32_t row = 0U; row < height; ++row) {
        const std::uint8_t* source_row = produced.ptr<std::uint8_t>(static_cast<int>(row));
        std::copy(source_row, source_row + row_bytes, bytes.begin() + static_cast<std::ptrdiff_t>(row * row_bytes));
    }

    return std::make_shared<const acquisition::Image>(
        std::move(id),
        width,
        height,
        static_cast<std::uint32_t>(row_bytes),
        format,
        std::move(bytes));
}

}  // namespace nvision::vision::interop
