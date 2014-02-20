#include <boost/python.hpp>
// From Boost.Python:
// The rule is that <Python.h> must be included before any system
// headers (so it can get control over some awful macros).
#include <Python.h> // Need to be included, because it is not always included by "boost/python.hpp".

#include "TextPlugin.hpp"
#include "TextProcess.hpp"
#include "TextDefinitions.hpp"

#include <terry/globals.hpp>
#include <terry/merge/MergeFunctors.hpp>
#include <terry/merge/ViewsMerging.hpp>

#include <tuttle/plugin/exceptions.hpp>
#include <tuttle/plugin/exceptions.hpp>
#include <tuttle/common/ofx/core.hpp>

#include <boost/gil/extension/color/hsl.hpp>
#include <boost/gil/gil_all.hpp>

#include <boost/filesystem.hpp>
#include <boost/ptr_container/ptr_inserter.hpp>

#include <sstream>
#include <string>
#include <iostream>

#ifndef __WINDOWS__
#include <fontconfig/fontconfig.h>
#endif

namespace tuttle {
namespace plugin {
namespace text {

template <class View, class Functor>
TextProcess<View, Functor>::TextProcess(TextPlugin &instance)
    : ImageGilProcessor<View>(instance, eImageOrientationFromTopToBottom),
      _plugin(instance) {
  //	Py_Initialize();
  _clipSrc = instance.fetchClip(kOfxImageEffectSimpleSourceClipName);
  this->setNoMultiThreading();
}

template <class View, class Functor>
void TextProcess<View, Functor>::setup(const OFX::RenderArguments &args) {
  using namespace terry;
  ImageGilProcessor<View>::setup(args);

  if (_clipSrc->isConnected()) {
    _src.reset(_clipSrc->fetchImage(args.time));
    if (!_src.get())
      BOOST_THROW_EXCEPTION(exception::ImageNotReady()
                            << exception::dev() + "Error on clip " +
                                   quotes(_clipSrc->name())
                            << exception::time(args.time));
    if (_src->getRowDistanceBytes() == 0)
      BOOST_THROW_EXCEPTION(exception::WrongRowBytes()
                            << exception::dev() + "Error on clip " +
                                   quotes(_clipSrc->name())
                            << exception::time(args.time));

    if (OFX::getImageEffectHostDescription()->hostName ==
        "uk.co.thefoundry.nuke") {
      // bug in nuke, getRegionOfDefinition() on OFX::Image returns bounds
      _srcPixelRod = _clipSrc->getPixelRod(args.time, args.renderScale);
    } else {
      _srcPixelRod = _src->getRegionOfDefinition();
    }
    _srcView = ImageGilProcessor<View>::template getCustomView<View>(
        _src.get(), _srcPixelRod);
  }

  _params = _plugin.getProcessParams(args.renderScale);

  if (!_params._isExpression) {
    _text = _params._text;
  } else {
    try {
      Py_Initialize();

      boost::python::object main_module = boost::python::import("__main__");
      boost::python::object main_namespace = main_module.attr("__dict__");

      std::ostringstream context;
      context << "def _timecode(time_sec, time_frame, fps):" << std::endl;
      context << "    return "
                 "'{0:02d}:{1:02d}:{2:02d}:{3:02d}'.format(int(time_sec / "
                 "3600), "
                 "int(time_sec / 60) % 60, "
                 "int(time_sec) % 60, "
                 "int(time_frame % fps) )" << std::endl;
      context << "def timecode_sec(time_sec, fps):" << std::endl;
      context << "    time_frame = int(time_sec * float(fps))" << std::endl;
      context << "    return _timecode(time_sec, time_frame, fps)" << std::endl;
      context << "def timecode_frame(time_frame, fps):" << std::endl;
      context << "    time_sec = time_frame / float(fps) if fps else time_frame"
              << std::endl;
      context << "    return _timecode(time_sec, time_frame, fps)" << std::endl;

      context << "class TuttleArgs:" << std::endl;
      context << "    time_frame = " << args.time << std::endl;
      context << "    renderScale = [" << args.renderScale.x << ","
              << args.renderScale.y << "]" << std::endl;
      context << "    renderWindow = [" << args.renderWindow.x1 << ","
              << args.renderWindow.y1 << "," << args.renderWindow.x2 << ","
              << args.renderWindow.y2 << "]" << std::endl;

      OfxRectD dstCanonicalRod = this->_clipDst->getCanonicalRod(args.time);
      context << "    dstCanonicalRod = [" << dstCanonicalRod.x1 << ","
              << dstCanonicalRod.y1 << "," << dstCanonicalRod.x2 << ","
              << dstCanonicalRod.y2 << "]" << std::endl;
      OfxRectI dstPixelRod = this->_clipDst->getPixelRod(args.time);
      context << "    dstPixelRod = [" << dstPixelRod.x1 << ","
              << dstPixelRod.y1 << "," << dstPixelRod.x2 << ","
              << dstPixelRod.y2 << "]" << std::endl;
      context << "    fps = " << _clipSrc->getFrameRate() << std::endl;

      context << "    @property" << std::endl;
      context << "    def time_sec(self):" << std::endl;
      context << "        return self.time_frame / float(self.fps) if self.fps "
                 "else self.time_frame" << std::endl;

      context << "    @property" << std::endl;
      context << "    def timecode(self):" << std::endl;
      context << "        return timecode_frame(self.time_frame, self.fps)"
              << std::endl;

      context << "args = TuttleArgs()" << std::endl;

      // TUTTLE_TLOG_INFO( "TEXT exec:\n" << context.str().c_str() );
      // TUTTLE_TLOG_INFO( "TEXT eval:\n" << _params._text.c_str() );

      boost::python::exec(context.str().c_str(), main_namespace);

      std::ostringstream toEval;
      toEval << "str(" << _params._text << ")";
      boost::python::object returnText =
          boost::python::eval(toEval.str().c_str(), main_namespace);

      _text = boost::python::extract<std::string>(returnText);
    }
    catch (boost::python::error_already_set const &) {
      // if we can't evaluate the expression
      // use the text without interpretation

      // Get error message from python
      PyObject *ptype, *pvalue, *ptraceback;
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);
#if PY_MAJOR_VERSION < 3
      // Python version is < 3.0
      char *pStrErrorMessage = PyString_AsString(pvalue);
#elif PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 3
      // PYTHON version is < 3.3
      PyObject *stringObj = PyUnicode_AsUTF8String(pvalue);
      char *pStrErrorMessage = PyBytes_AsString(stringObj);
      ;
#else
      // PYTHON version is >= 3.3
      char *pStrErrorMessage = PyUnicode_AsUTF8(pvalue);
#endif
      TUTTLE_LOG_ERROR("Python error : " << pStrErrorMessage);
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 3
      Py_DECREF(stringObj);
#endif

      _text = _params._text;
    }
    //		Py_Finalize();
  }

  // Step 1. Create terry image
  // Step 2. Initialize freetype
  // Step 3. Make Glyphs Array
  // Step 4. Make Metrics Array
  // Step 5. Make Kerning Array
  // Step 6. Get Coordinates (x,y)
  // Step 7. Render Glyphs on GIL View
  // Step 8. Save GIL Image

  // Step 1. Create terry image -----------

  // Step 2. Initialize freetype ---------------
  FT_Library library;
  FT_Init_FreeType(&library);

  FT_Face face;

  std::string selectedFont = "";

  if (!boost::filesystem::exists(_params._fontPath) ||
      boost::filesystem::is_directory(_params._fontPath)) {
#ifdef __WINDOWS__
    BOOST_THROW_EXCEPTION(exception::FileNotExist(_params._fontPath)
                          << exception::user("Text: Error in Font Path.")
                          << exception::filename(_params._fontPath));
#else
    FcInit();

    FcChar8 *file;
    FcResult result;
    FcConfig *config = FcInitLoadConfigAndFonts();
    FcPattern *p =
        FcPatternBuild(NULL, FC_WEIGHT, FcTypeInteger, FC_WEIGHT_BOLD, FC_SLANT,
                       FcTypeInteger, FC_SLANT_ITALIC, NULL);

    FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, NULL);
    FcFontSet *fs = FcFontList(config, p, os);

    selectedFont = (char *)FcNameUnparse(fs->fonts[_params._font]);

    int weight = (_params._bold == 1) ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM;
    int slant = (_params._italic == 1) ? FC_SLANT_ITALIC : FC_SLANT_ROMAN;

    p = FcPatternBuild(NULL, FC_FAMILY, FcTypeString, selectedFont.c_str(),
                       FC_WEIGHT, FcTypeInteger, weight, FC_SLANT,
                       FcTypeInteger, slant, NULL);

    FcPatternGetString(FcFontMatch(0, p, &result), FC_FAMILY, 0, &file);
    FcPatternGetString(FcFontMatch(0, p, &result), FC_FILE, 0, &file);
    selectedFont = (char *)file;
#endif
  } else {
    selectedFont = _params._fontPath;
  }
  FT_New_Face(library, selectedFont.c_str(), 0, &face);

  FT_Set_Pixel_Sizes(face, _params._fontX, _params._fontY);

  // Step 3. Make Glyphs Array ------------------
  rgba32f_pixel_t rgba32f_foregroundColor(
      _params._fontColor.r, _params._fontColor.g, _params._fontColor.b,
      _params._fontColor.a);
  color_convert(rgba32f_foregroundColor, _foregroundColor);
  std::transform(_text.begin(), _text.end(),
                 boost::ptr_container::ptr_back_inserter(_glyphs),
                 make_glyph(face));

  // Step 4. Make Metrics Array --------------------
  std::transform(_glyphs.begin(), _glyphs.end(), std::back_inserter(_metrics),
                 terry::make_metric());

  // Step 5. Make Kerning Array ----------------
  std::transform(_glyphs.begin(), _glyphs.end(), std::back_inserter(_kerning),
                 terry::make_kerning());

  // Step 6. Get Coordinates (x,y) ----------------
  _textSize.x = std::for_each(_metrics.begin(), _metrics.end(),
                              _kerning.begin(), terry::make_width());
  _textSize.y =
      std::for_each(_metrics.begin(), _metrics.end(), terry::make_height());

  if (_metrics.size() > 1)
    _textSize.x += _params._letterSpacing * (_metrics.size() - 1);

  switch (_params._vAlign) {
  case eParamVAlignTop: {
    _textCorner.y = 0;
    break;
  }
  case eParamVAlignCenter: {
    _textCorner.y = (this->_dstView.height() - _textSize.y) * 0.5;
    break;
  }
  case eParamVAlignBottom: {
    _textCorner.y = this->_dstView.height() - (_textSize.y + _textSize.y / 3);
    break;
  }
  }
  switch (_params._hAlign) {
  case eParamHAlignLeft: {
    _textCorner.x = 0;
    break;
  }
  case eParamHAlignCenter: {
    _textCorner.x = (this->_dstView.width() - _textSize.x) * 0.5;
    break;
  }
  case eParamHAlignRight: {
    _textCorner.x = this->_dstView.width() - _textSize.x;
    break;
  }
  }

  if (_params._verticalFlip) {
    _dstViewForGlyphs = flipped_up_down_view(this->_dstView);
    _textCorner.y -= _params._position.y;
  } else {
    _dstViewForGlyphs = this->_dstView;
    _textCorner.y += _params._position.y;
  }

  _textCorner.x += _params._position.x;
}

/**
 * @brief Function called by rendering thread each time a process must be done.
 * @param[in] procWindowRoW  Processing window in RoW
 */
template <class View, class Functor>
void TextProcess<View, Functor>::multiThreadProcessImages(
    const OfxRectI &procWindowRoW) {
  using namespace terry;

  rgba32f_pixel_t backgroundColor(
      _params._backgroundColor.r, _params._backgroundColor.g,
      _params._backgroundColor.b, _params._backgroundColor.a);
  fill_pixels(this->_dstView, backgroundColor);

  if (_clipSrc->isConnected()) {
    // merge_views( this->_dstView, _srcView, this->_dstView,
    // FunctorMatte<Pixel>() );
    merge_views(this->_dstView, _srcView, this->_dstView, Functor());
  }

  // Step 7. Render Glyphs ------------------------
  // if outside dstRod
  // ...
  // else
  const OfxRectI textRod = { _textCorner.x,
                             _textCorner.y,
                             _textCorner.x + _textSize.x,
                             _textCorner.y + _textSize.y + _textSize.y / 3 };
  const OfxRectI textRoi = rectanglesIntersection(textRod, procWindowRoW);
  const OfxRectI textLocalRoi = translateRegion(textRoi, -_textCorner);

  // TUTTLE_LOG_VAR( TUTTLE_INFO, _textSize );
  // TUTTLE_LOG_VAR( TUTTLE_INFO, _textCorner );

  // TUTTLE_LOG_VAR( TUTTLE_INFO, textRod );
  // TUTTLE_LOG_VAR( TUTTLE_INFO, procWindowRoW );
  // TUTTLE_LOG_VAR( TUTTLE_INFO, textRoi );
  // TUTTLE_LOG_VAR( TUTTLE_INFO, textLocalRoi );
  // TUTTLE_LOG_VAR2( TUTTLE_INFO, _dstViewForGlyphs.width(),
  // _dstViewForGlyphs.height() );

  View tmpDstViewForGlyphs =
      subimage_view(_dstViewForGlyphs, _textCorner.x, _textCorner.y,
                    _textSize.x, _textSize.y);

  std::for_each(_glyphs.begin(), _glyphs.end(), _kerning.begin(),
                render_glyph<View>(tmpDstViewForGlyphs, _foregroundColor,
                                   _params._letterSpacing,
                                   Rect<std::ptrdiff_t>(textLocalRoi)));
}
}
}
}
