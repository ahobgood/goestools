#include "source.h"

#ifdef BUILD_AIRSPY
#include "airspy_source.h"
#endif

#ifdef BUILD_RTLSDR
#include "rtlsdr_source.h"
#endif

#include "nanomsg_source.h"

std::unique_ptr<Source> Source::build(
    const std::string& type,
    Config& config) {
  if (type == "airspy") {
#ifdef BUILD_AIRSPY
    auto airspy = Airspy::open();
    airspy->setSampleRate(3000000);
    airspy->setFrequency(config.airspy.frequency);
    airspy->setGain(config.airspy.gain);
    airspy->setBiasTee(config.airspy.bias_tee);
    airspy->setSamplePublisher(std::move(config.airspy.samplePublisher));
    return std::unique_ptr<Source>(airspy.release());
#else
    throw std::runtime_error(
      "You configured goesrecv to use the \"airspy\" source, "
      "but goesrecv was not compiled with Airspy support. "
      "Make sure to install the Airspy library before compiling goestools, "
      "and look for a message saying 'Found libairspy' when running cmake."
      );
#endif
  }
  if (type == "rtlsdr") {
#ifdef BUILD_RTLSDR
    auto rtlsdr = RTLSDR::open();
    rtlsdr->setSampleRate(2400000);
    rtlsdr->setFrequency(config.rtlsdr.frequency);
    rtlsdr->setTunerGain(config.rtlsdr.gain);
    rtlsdr->setBiasTee(config.rtlsdr.bias_tee);
    rtlsdr->setSamplePublisher(std::move(config.rtlsdr.samplePublisher));
    return std::unique_ptr<Source>(rtlsdr.release());
#else
    throw std::runtime_error(
      "You configured goesrecv to use the \"rtlsdr\" source, "
      "but goesrecv was not compiled with RTL-SDR support. "
      "Make sure to install the RTL-SDR library before compiling goestools, "
      "and look for a message saying 'Found librtlsdr' when running cmake."
      );
#endif
  }
  if (type == "nanomsg") {
    auto nanomsg = Nanomsg::open(config);
    nanomsg->setSampleRate(config.nanomsg.sampleRate);
    nanomsg->setSamplePublisher(std::move(config.nanomsg.samplePublisher));
    return std::unique_ptr<Source>(nanomsg.release());
  }

  throw std::runtime_error("Invalid source: " + type);
}

Source::~Source() {
}
