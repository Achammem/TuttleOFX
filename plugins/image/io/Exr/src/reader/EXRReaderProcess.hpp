#ifndef _EXR_READER_PROCESS_HPP_
#define _EXR_READER_PROCESS_HPP_

#include <tuttle/plugin/global.hpp>
#include <tuttle/plugin/ImageGilProcessor.hpp>
#include <tuttle/plugin/memory/OfxAllocator.hpp>

#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>

#include <ImfInputFile.h>

#include <boost/scoped_ptr.hpp>

namespace tuttle {
namespace plugin {
namespace exr {
namespace reader {

/**
 * @brief Exr reader
 */
template <class View> class EXRReaderProcess : public ImageGilProcessor<View> {
protected:
  typedef typename View::value_type Pixel;
  typedef std::vector<char, OfxAllocator<char> > DataVector;

  EXRReaderPlugin &_plugin; ///< Rendering plugin
  EXRReaderProcessParams _params;
  boost::scoped_ptr<Imf::InputFile> _exrImage; ///< Pointer to an exr image

  template <typename PixelType>
  void initExrChannel(DataVector &data, Imf::Slice &slice,
                      Imf::FrameBuffer &frameBuffer, Imf::PixelType pixelType,
                      std::string channelID, const Imath::Box2i &dw, int w,
                      int h);

  template <class DView>
  void channelCopy(Imf::InputFile &input, Imf::FrameBuffer &frameBuffer,
                   const EXRReaderProcessParams &params, DView &dst, int w,
                   int h, size_t nc);

  template <class DView, typename workingView>
  void sliceCopy(Imf::InputFile &input, const Imf::Slice *slice, DView &dst,
                 const EXRReaderProcessParams &params, int w, int h, int n);

  std::string getChannelName(size_t index);

public:
  EXRReaderProcess<View>(EXRReaderPlugin &instance);

  void setup(const OFX::RenderArguments &args);

  void multiThreadProcessImages(const OfxRectI &procWindowRoW);

  // Read exr image
  template <class DView> void readImage(DView dst, const std::string &filepath);
};
}
}
}
}

#include "EXRReaderProcess.tcc"

#endif
