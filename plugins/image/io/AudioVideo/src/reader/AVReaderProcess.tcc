#include "AVReaderPlugin.hpp"

#include <boost/numeric/conversion/cast.hpp>

namespace tuttle {
namespace plugin {
namespace av {
namespace reader {

template <class View>
AVReaderProcess<View>::AVReaderProcess(AVReaderPlugin &instance)
    : ImageGilProcessor<View>(instance, eImageOrientationFromTopToBottom),
      _plugin(instance) {
  this->setNoMultiThreading();
}

template <class View>
void AVReaderProcess<View>::setup(const OFX::RenderArguments &args) {
  if (!_plugin.ensureVideoIsOpen())
    BOOST_THROW_EXCEPTION(exception::Failed()
                          << exception::user("Can't open this video file")
                          << exception::filename(
                                 _plugin._paramFilepath->getValue()));

  // Fetch output image
  if (!_plugin._reader.read(boost::numeric_cast<int>(args.time)))
    BOOST_THROW_EXCEPTION(exception::Failed()
                          << exception::user() +
                                 "Can't open the frame at time " + args.time
                          << exception::filename(
                                 _plugin._paramFilepath->getValue()));

  ImageGilProcessor<View>::setup(args);
}

/**
 * @brief Function called by rendering thread each time a process must be done.
 * @param[in] procWindowRoW  Processing window in RoW
 */
template <class View>
void
AVReaderProcess<View>::multiThreadProcessImages(const OfxRectI &procWindowRoW) {
  using namespace boost::gil;
  BOOST_ASSERT(procWindowRoW == this->_dstPixelRod);

  rgb8c_view_t avSrcView =
      interleaved_view(_plugin._reader.width(), _plugin._reader.height(),
                       (const rgb8c_pixel_t *)(_plugin._reader.data()),
                       _plugin._reader.width() * 3);

  copy_and_convert_pixels(avSrcView, this->_dstView);
}
}
}
}
}
