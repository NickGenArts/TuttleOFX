#include "CheckerboardPluginFactory.hpp"
#include "CheckerboardPlugin.hpp"
#include "CheckerboardDefinitions.hpp"

#include <tuttle/plugin/ImageGilProcessor.hpp>
#include <tuttle/plugin/PluginException.hpp>

#include <string>
#include <iostream>
#include <stdio.h>
#include <cmath>
#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <boost/gil/gil_all.hpp>
#include <boost/scoped_ptr.hpp>

namespace tuttle {
namespace plugin {
namespace checkerboard {

/**
 * @brief Function called to describe the plugin main features.
 * @param[in, out]   desc     Effect descriptor
 */
using namespace OFX;
void CheckerboardPluginFactory::describe( OFX::ImageEffectDescriptor &desc )
{
    // basic labels
    desc.setLabels( "Checkerboard", "Checkerboard",
                    "Checkerboard" );
    desc.setPluginGrouping( "tuttle" );

    // add the supported contexts, only filter at the moment
    desc.addSupportedContext( eContextGeneral );

    // add supported pixel depths
    desc.addSupportedBitDepth( eBitDepthUByte );
    desc.addSupportedBitDepth( eBitDepthUShort );
    desc.addSupportedBitDepth( eBitDepthFloat );

    // set a few flags
    desc.setSingleInstance( false );
	 desc.setHostFrameThreading( true );
    desc.setSupportsMultiResolution( false );
    desc.setSupportsTiles( kSupportTiles );
    desc.setTemporalClipAccess( kSupportTemporalClipAccess );
    desc.setRenderTwiceAlways( false );
    desc.setSupportsMultipleClipPARs( false );
}

/**
 * @brief Function called to describe the plugin controls and features.
 * @param[in, out]   desc       Effect descriptor
 * @param[in]        context    Application context
 */
void CheckerboardPluginFactory::describeInContext( OFX::ImageEffectDescriptor &desc,
                                             OFX::ContextEnum context )
{
    OFX::ClipDescriptor* srcClip = desc.defineClip( kOfxImageEffectSimpleSourceClipName );
    srcClip->addSupportedComponent( ePixelComponentRGBA );
    srcClip->addSupportedComponent( ePixelComponentAlpha );
    srcClip->setSupportsTiles( kSupportTiles );

    // Create the mandated output clip
    OFX::ClipDescriptor* dstClip = desc.defineClip( kOfxImageEffectOutputClipName );
    dstClip->addSupportedComponent( ePixelComponentRGBA );
    dstClip->addSupportedComponent( ePixelComponentAlpha );
    dstClip->setSupportsTiles( kSupportTiles );

    OFX::Int2DParamDescriptor *boxes = desc.defineInt2DParam( kCheckerboardBoxes );
	boxes->setDefault(10, 10);
	boxes->setLabel( "boxes number" );
    boxes->setHint( "Number of boxes of the checkerboard." );

    OFX::RGBAParamDescriptor* color1 = desc.defineRGBAParam( kCheckerboardColor1 );
	color1->setDefault(0, 0, 0, 1);
	color1->setLabel( "color1" );

    OFX::RGBAParamDescriptor* color2 = desc.defineRGBAParam( kCheckerboardColor2 );
	color2->setDefault(1, 1, 1, 1);
	color2->setLabel( "color2" );
}

/**
 * @brief Function called to create a plugin effect instance
 * @param[in] handle  effect handle
 * @param[in] context    Application context
 * @return  plugin instance
 */
OFX::ImageEffect* CheckerboardPluginFactory::createInstance(OfxImageEffectHandle handle,
                                            OFX::ContextEnum context)
{
    return new CheckerboardPlugin(handle);
}

}
}
}

namespace OFX 
{
    namespace Plugin 
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            static tuttle::plugin::checkerboard::CheckerboardPluginFactory p("fr.tuttle.checkerboard", 1, 0);
            ids.push_back(&p);
        }
    }
}