#ifndef _TUTTLE_PLUGIN_RAWREADERPLUGIN_HPP_
#define _TUTTLE_PLUGIN_RAWREADERPLUGIN_HPP_

#include "RawReaderDefinitions.hpp"
#include <tuttle/plugin/context/ReaderPlugin.hpp>

namespace tuttle {
namespace plugin {
namespace raw {
namespace reader {

template <typename Scalar> struct RawReaderProcessParams {
  std::string _filepath; ///< filepath
  EFiltering _filtering;
  EInterpolation _interpolation;
  float _gammaPower;
  float _gammaToe;
  double _redAbber;
  double _blueAbber;

  double _bright;
  double _threshold;
  bool _fourColorRgb;

  EHighlight _hightlight;

  double _exposure;
  double _exposurePreserve;

  EWhiteBalance _whiteBalance;

  boost::gil::point2<Scalar> _greyboxPoint;
  boost::gil::point2<Scalar> _greyboxSize;
};

/**
 * @brief Raw reader
 *
 */
class RawReaderPlugin : public ReaderPlugin {
public:
  typedef float Scalar;
  typedef boost::gil::point2<double> Point2;

public:
  RawReaderPlugin(OfxImageEffectHandle handle);

public:
  RawReaderProcessParams<Scalar> getProcessParams(const OfxTime time);

  void updateInfos(const OfxTime time);

  void changedParam(const OFX::InstanceChangedArgs &args,
                    const std::string &paramName);
  bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args,
                             OfxRectD &rod);
  void getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences);

  void render(const OFX::RenderArguments &args);

public:
  /// @name user parameters
  /// @{
  OFX::ChoiceParam *_paramFiltering; ///< Filtering mode
  OFX::ChoiceParam *_paramInterpolation;

  OFX::DoubleParam *_paramGammaPower;
  OFX::DoubleParam *_paramGammaToe;
  OFX::DoubleParam *_paramRedAbber;
  OFX::DoubleParam *_paramBlueAbber;

  OFX::DoubleParam *_paramBright;
  OFX::DoubleParam *_paramThreshold;
  OFX::BooleanParam *_paramFourColorRgb;

  OFX::ChoiceParam *_paramHighlight;

  OFX::DoubleParam *_paramExposure;
  OFX::DoubleParam *_paramExposurePreserve;

  OFX::ChoiceParam *_paramWhiteBalance;

  OFX::Double2DParam *_paramGreyboxPoint;
  OFX::Double2DParam *_paramGreyboxSize;

  /// metadata
  OFX::StringParam *_paramManufacturer;
  OFX::StringParam *_paramModel;
  OFX::IntParam *_paramIso;
  OFX::IntParam *_paramShutter;
  OFX::DoubleParam *_paramAperture;
  OFX::StringParam *_paramDateOfShooting;
  OFX::StringParam *_paramGPS;
  OFX::StringParam *_paramDesc;
  OFX::StringParam *_paramArtist;
  /// @}
};
}
}
}
}

#endif
