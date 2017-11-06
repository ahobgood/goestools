#include <iostream>

#include "file.h"

int dump(const std::string& name) {
  auto file = LRIT::File(name);
  std::cout << name << ":" << std::endl;
  for (const auto& it : file.getHeaderMap()) {
    if (it.first == LRIT::PrimaryHeader::CODE) {
      auto h = file.getHeader<LRIT::PrimaryHeader>();
      std::cout << "Primary (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  File type: "
                << int(h.fileType) << std::endl;
      std::cout << "  Total header length: "
                << h.totalHeaderLength << std::endl;
      std::cout << "  Data length: "
                << h.dataLength << std::endl;
    } else if (it.first == LRIT::ImageStructureHeader::CODE) {
      auto h = file.getHeader<LRIT::ImageStructureHeader>();
      std::cout << "Image structure (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Bits per pixel: "
                << int(h.bitsPerPixel) << std::endl;
      std::cout << "  Columns: "
                << h.columns << std::endl;
      std::cout << "  Lines: "
                << h.lines << std::endl;
      std::cout << "  Compression: "
                << int(h.compression) << std::endl;
    } else if (it.first == LRIT::ImageNavigationHeader::CODE) {
      auto h = file.getHeader<LRIT::ImageNavigationHeader>();
      std::cout << "Image navigation (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Projection name: "
                << h.projectionName << std::endl;
      std::cout << "  Column scaling: "
                << h.columnScaling << std::endl;
      std::cout << "  Line scaling: "
                << h.lineScaling << std::endl;
      std::cout << "  Column offset: "
                << h.columnOffset << std::endl;
      std::cout << "  Line offset: "
                << h.lineOffset << std::endl;
    } else if (it.first == LRIT::ImageDataFunctionHeader::CODE) {
      auto h = file.getHeader<LRIT::ImageDataFunctionHeader>();
      std::cout << "Image data function (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Data: "
                << "(omitted, length: " << h.data.size() << ")" << std::endl;
    } else if (it.first == LRIT::AnnotationHeader::CODE) {
      auto h = file.getHeader<LRIT::AnnotationHeader>();
      std::cout << "Annotation (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Text: "
                << h.text << std::endl;
    } else if (it.first == LRIT::TimeStampHeader::CODE) {
      std::array<char, 128> tsbuf;
      auto h = file.getHeader<LRIT::TimeStampHeader>();
      auto ts = h.getUnix();
      auto len = strftime(
        tsbuf.data(),
        tsbuf.size(),
        "%Y-%m-%d %H:%M:%S",
        gmtime(&ts.tv_sec));
      std::cout << "Time stamp (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Time stamp: "
                << std::string(tsbuf.data(), len) << std::endl;
    } else if (it.first == LRIT::AncillaryTextHeader::CODE) {
      auto h = file.getHeader<LRIT::AncillaryTextHeader>();
      std::cout << "Ancillary text (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Text: "
                << h.text << std::endl;
    } else if (it.first == LRIT::SegmentIdentificationHeader::CODE) {
      auto h = file.getHeader<LRIT::SegmentIdentificationHeader>();
      std::cout << "Segment identification (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Image identifier: "
                << h.imageIdentifier << std::endl;
      std::cout << "  Segment number: "
                << h.segmentNumber << std::endl;
      std::cout << "  Start column of segment: "
                << h.segmentStartColumn << std::endl;
      std::cout << "  Start line of segment: "
                << h.segmentStartLine << std::endl;
      std::cout << "  Number of segments: "
                << h.maxSegment << std::endl;
      std::cout << "  Width of final image: "
                << h.maxColumn << std::endl;
      std::cout << "  Height of final image: "
                << h.maxLine << std::endl;
    } else if (it.first == LRIT::NOAALRITHeader::CODE) {
      auto h = file.getHeader<LRIT::NOAALRITHeader>();
      std::cout << "NOAA LRIT (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Agency signature: "
                << std::string(h.agencySignature) << std::endl;
      std::cout << "  Product ID: "
                << h.productID << std::endl;
      std::cout << "  Product SubID: "
                << h.productSubID << std::endl;
      std::cout << "  Parameter: "
                << h.parameter << std::endl;
      std::cout << "  NOAA-specific compression: "
                << int(h.noaaSpecificCompression) << std::endl;
    } else if (it.first == LRIT::HeaderStructureRecordHeader::CODE) {
      // Ignore...
    } else if (it.first == LRIT::RiceCompressionHeader::CODE) {
      auto h = file.getHeader<LRIT::RiceCompressionHeader>();
      std::cout << "Rice compression (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Flags: "
                << h.flags << std::endl;
      std::cout << "  Pixels per block: "
                << int(h.pixelsPerBlock) << std::endl;
      std::cout << "  Scan lines per packet: "
                << int(h.scanLinesPerPacket) << std::endl;
    } else {
      std::cerr << "Header " << it.first << " not handled..." << std::endl;
      return 1;
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  for (auto i = 1; i < argc; i++) {
    auto rv = dump(argv[i]);
    if (rv != 0) {
      return rv;
    }
  }
  return 0;
}
