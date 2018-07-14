#include "config.h"

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
        auto img = cv::imread(path, CV_8UC1);
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

        h.remap[toUpper(it.first)] = img;
      }
    }

    auto remap_rgb = th->find("remap_rgb");
    if (remap_rgb) {
      auto trs = remap_rgb->as<toml::Table>();
      for (const auto& it : trs) {
        auto channel = toUpper(it.first);
        auto path = it.second.get<std::string>("path");
        auto img = cv::imread(path);
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

        h.remap_rgb[toUpper(it.first)] = img;
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
