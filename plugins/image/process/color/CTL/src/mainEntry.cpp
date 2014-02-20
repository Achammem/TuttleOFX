#define OFXPLUGIN_VERSION_MAJOR 1
#define OFXPLUGIN_VERSION_MINOR 1

#include "CTLPluginFactory.hpp"
#include <tuttle/plugin/Plugin.hpp>

namespace OFX {
namespace Plugin {

void getPluginIDs(OFX::PluginFactoryArray &ids) {
  mAppendPluginFactory(ids, tuttle::plugin::ctl::CTLPluginFactory,
                       "tuttle.ctl");
}
}
}
