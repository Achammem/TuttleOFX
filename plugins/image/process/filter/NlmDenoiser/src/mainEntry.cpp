#define OFXPLUGIN_VERSION_MAJOR 1
#define OFXPLUGIN_VERSION_MINOR 2

#include <tuttle/plugin/Plugin.hpp>
#include "NLMDenoiserPluginFactory.hpp"

namespace OFX {
namespace Plugin {

void getPluginIDs(OFX::PluginFactoryArray &ids) {
  mAppendPluginFactory(ids,
                       tuttle::plugin::nlmDenoiser::NLMDenoiserPluginFactory,
                       "tuttle.nlmdenoiser");
}
}
}
