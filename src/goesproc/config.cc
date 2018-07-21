#include "config.h"

#include <sstream>
#include <toml/toml.h>

#include "lib/util.h"

namespace {

bool loadHandlers(const toml::Value& v, Config& out) {
  auto ths = v.find("handler");
  if (ths->size() == 0) {
    out.ok = false;
    out.error = "No handlers found";
    return false;
  }

  for (size_t i = 0; i < ths->size(); i++) {
    auto th = ths->find(i);

    Config::Handler h;
    h.type = th->get<std::string>("type");

    auto product = th->find("product");
    if (product) {
      h.product = product->as<std::string>();
    }

    // Singular region is the old way to filter
    auto region = th->find("region");
    if (region) {
      h.regions.push_back(region->as<std::string>());
    }

    // Plural regions is the new way to filter
    auto regions = th->find("regions");
    if (regions) {
      h.regions = regions->as<std::vector<std::string>>();
    }

    auto channels = th->find("channels");
    if (channels) {
      h.channels = channels->as<std::vector<std::string>>();
    }

    auto dir = th->find("dir");
    if (dir) {
      h.dir = dir->as<std::string>();
    } else {
      // Fall back on "directory"
      dir = th->find("directory");
      if (dir) {
        h.dir = dir->as<std::string>();
      } else {
        h.dir = ".";
      }
    }

    auto format = th->find("format");
    if (format) {
      h.format = format->as<std::string>();
    } else {
      h.format = "png";
    }

    auto crop = th->find("crop");
    if (crop) {
      auto vs = crop->as<std::vector<int>>();
      if (vs.size() != 4) {
        out.ok = false;
        out.error = "Expected \"crop\" to hold 4 integers";
        return false;
      }

      h.crop.minColumn = vs[0];
      h.crop.maxColumn = vs[1];
      h.crop.minLine = vs[2];
      h.crop.maxLine = vs[3];

      if (h.crop.width() < 1) {
        out.ok = false;
        out.error = "Expected \"crop\" to have positive width";
        return false;
      }

      if (h.crop.height() < 1) {
        out.ok = false;
        out.error = "Expected \"crop\" to have positive height";
        return false;
      }
    }

    auto remap = th->find("remap");
    if (remap) {
      auto trs = remap->as<toml::Table>();
      for (const auto& it : trs) {
        auto channel = toUpper(it.first);
        auto path = it.second.get<std::string>("path");
        auto img = cv::imread(path, CV_LOAD_IMAGE_UNCHANGED);
        if (!img.data) {
          out.ok = false;
          out.error = "Unable to load image at: " + path;
          return false;
        }
        if (img.total() != 256) {
          out.ok = false;
          out.error = "Expected channel remap image to have 256 pixels";
          return false;
        }

        h.remap[channel] = img;
      }
    }

    auto gradient = th->find("gradient");
    if (gradient) {
      auto trs = gradient->as<toml::Table>();
      for (const auto& it : trs) {
        auto channel = toUpper(it.first);
        auto interpolation = it.second.find("interpolation");

        if (interpolation) {
          if (0 == interpolation->as<std::string>().compare("hsv")) {
            h.lerptype = LERP_HSV;
          } else if (0 == interpolation->as<std::string>().compare("rgb")) {
            h.lerptype = LERP_RGB;
          } else {
            out.ok = false;
            out.error = "Unknown gradient interpolation type (supported types are \"rgb\" and \"hsv\")";
            return false;
          }
        }

        Gradient grad;
        auto points = it.second.get<toml::Array>("points");
        for (const auto& point : points) {
          auto units = point.find("units");
          if (!units) units = point.find("u");

          // HTML-style #xxxxxx or #xxx color definition
          auto color = point.find("color");
          if (!color) color = point.find("c");

          // ... or RGB components (0-1 or 0-255)
          auto red = point.find("r");
          auto green = point.find("g");
          auto blue = point.find("b");

          // ... or HSV components (from 0-1 or 0-360[hue] and 0-100[sat/val])
          auto hue = point.find("h");
          auto sat = point.find("s");
          auto val = point.find("v");

          float r, g, b;
          r = g = b = -1;

          if (!units || !units->isNumber()) {
            out.ok = false;
            out.error = "Expected numeric units field in gradient point";
            return false;
          }

          // Hex color code takes precedence over individual RGB
          //  color components.
          if (color && color->is<std::string>()) {
            auto c = color->as<std::string>();
            std::stringstream t;
            unsigned int ti;

            if (c.at(0) == '#') {
              t << std::hex << c.substr(1);
              t >> ti;

              if (c.length() == 4) {
                // #xxx style
                r = ((ti & 0xF00) >> 8) / 15.0;
                g = ((ti & 0xF0) >> 4) / 15.0;
                b = (ti & 0xF) / 15.0;
              } else if (c.length() == 7) {
                // #xxxxxx style
                r = ((ti & 0xFF0000) >> 16) / 255.0;
                g = ((ti & 0xFF00) >> 8) / 255.0;
                b = (ti & 0xFF) / 255.0;
              } else if (red && green && blue) {
                out.ok = false;
                out.error = "Invalid hex color code in gradient point definition";
                return false;
              }
            }
          } else {
            r = red->asNumber();
            g = green->asNumber();
            b = blue->asNumber();
          }

          if (r >= 0 && g >= 0 && b >= 0) {
            auto newpoint = GradientPoint::fromRGB(units->asNumber(), r, g, b);
            grad.addPoint(newpoint);
          } else if (hue && sat && val) {
            // Use HSV components if defined and RGB not available
            auto newpoint = GradientPoint::fromHSV(units->asNumber(),
                                                   hue->asNumber(),
                                                   sat->asNumber(),
                                                   val->asNumber());
            grad.addPoint(newpoint);
          } else {
            out.ok = false;
            out.error = "Invalid color in gradient point";
            return false;
          }
        }
        h.gradient[channel] = grad;
      }
    }

    auto lut = th->find("lut");
    if (lut) {
      auto path = lut->get<std::string>("path");
      auto img = cv::imread(path);
      if (!img.data) {
        out.ok = false;
        out.error = "Unable to load image at: " + path;
        return false;
      }
      if (img.total() != (256 * 256)) {
        out.ok = false;
        out.error = "Expected false color table to have 256x256 pixels";
        return false;
      }

      h.lut = img;
    }

    auto filename = th->find("filename");
    if (filename) {
      // To keep this readable, it can be specified as a list.
      // The resulting value is the concatenation of elements.
      if (filename->is<toml::Array>()) {
        for (const auto& str : filename->as<std::vector<std::string>>()) {
          h.filename += str;
        }
      } else {
        h.filename = filename->as<std::string>();
      }
    }

    // Sanity check
    if (h.lut.data && h.channels.size() != 2) {
      out.ok = false;
      out.error = "Using a false color table requires selecting 2 channels";
      return false;
    }

    out.handlers.push_back(h);
  }

  return true;
}

} // namespace

Config Config::load(const std::string& file) {
  Config out;

  auto pr = toml::parseFile(file);
  if (!pr.valid()) {
    out.ok = false;
    out.error = pr.errorReason;
    return out;
  }

  const auto& v = pr.value;
  if (!loadHandlers(v, out)) {
    return out;
  }

  return out;
}

Config::Config() : ok(true) {
}
