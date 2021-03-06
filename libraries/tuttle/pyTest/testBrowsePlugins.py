# scons: all

from pyTuttle.tuttle import *

def setUp():
	core().preload(False)

def testBrowsePlugins():
	pluginCache = core().getPluginCache()

	print [p for p in pluginCache.getPluginPath()]
	print [p.getIdentifier() for p in pluginCache.getPlugins()]

def testBrowseIEPlugins():
	pluginCache = core().getImageEffectPluginCache()

	print [p.getDescriptor().getShortLabel() for p in pluginCache.getPlugins()]
	print [p.getDescriptor().getLabel() for p in pluginCache.getPlugins()]
	print [p.getDescriptor().getLongLabel() for p in pluginCache.getPlugins()]
	print [p.getDescriptor().getPluginGrouping() for p in pluginCache.getPlugins()]

#def testPluginInfos():
#	g = tuttle.Graph()
#	p = g.createNode( plugin )
#	node = p.asImageEffectNode()
#
#	grouping = node.getProperties().fetchProperty("OfxImageEffectPluginPropGrouping").getStringValue(0)
#	grouping = grouping.split('/')
#
