/**
 * @file %CLASSNAME%Process.tcc
 * @brief
 * @author
 * @date    %DATE%
 *
 */


#include <tuttle/common/image/gilGlobals.hpp>
#include <tuttle/plugin/ImageGilProcessor.hpp>
#include <tuttle/plugin/Progress.hpp>
#include <tuttle/plugin/PluginException.hpp>

#include <cstdlib>
#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <boost/gil/gil_all.hpp>


namespace %NAMESPACE% {
namespace plugin {

using namespace boost::gil;

template<class View>
%CLASSNAME%Process<View>::%CLASSNAME%Process( %CLASSNAME%Plugin &instance )
: tuttle::plugin::ImageGilProcessor<View>( instance )
, tuttle::plugin::Progress( instance )
, _plugin( instance )
{
}

template<class View>
void %CLASSNAME%Process<View>::setupAndProcess( const OFX::RenderArguments &args )
{
    try
    {
        boost::scoped_ptr<OFX::Image> src( _plugin.getSrcClip( )->fetchImage( args.time ) );
        if( !src.get( ) )
            throw( ImageNotReadyException( ) );
        OfxRectI sBounds = src->getBounds( );
        OFX::BitDepthEnum srcBitDepth = src->getPixelDepth( );
        OFX::PixelComponentEnum srcComponents = src->getPixelComponents( );

        // Build source view
        this->_srcView = interleaved_view( std::abs( sBounds.x2 - sBounds.x1 ), std::abs( sBounds.y2 - sBounds.y1 ),
                                           static_cast < value_t* > ( src->getPixelData( ) ),
                                           src->getRowBytes( ) );

        boost::scoped_ptr<OFX::Image> dst( _plugin.getDstClip( )->fetchImage( args.time ) );
        if( !dst.get( ) )
            throw( ImageNotReadyException( ) );
        OfxRectI dBounds = dst->getBounds( );
        OFX::BitDepthEnum dstBitDepth = dst->getPixelDepth( );
        OFX::PixelComponentEnum dstComponents = dst->getPixelComponents( );

        // Make sure bit depths are same
        if( srcBitDepth != dstBitDepth || srcComponents != dstComponents )
        {
            throw( BitDepthMismatchException( ) );
        }

        // Build destination view
        this->_dstView = interleaved_view( std::abs( dBounds.x2 - dBounds.x1 ), std::abs( dBounds.y2 - dBounds.y1 ),
                                           static_cast < value_t* > ( dst->getPixelData( ) ),
                                           dst->getRowBytes( ) );

        // Set the render window
        this->setRenderWindow( args.renderWindow );
        // Call the base class process member
        this->process( );
    }
    catch( PluginException e )
    {
        COUT_EXCEPTION( e );
    }
}

/**
 * @brief Function called by rendering thread each time 
 *        a process must be done.
 *
 * @param[in] procWindow  Processing window
 */
template<class View>
void %CLASSNAME%Process<View>::multiThreadProcessImages( OfxRectI procWindow )
{
    try
    {
        View src = subimage_view( this->_srcView, procWindow.x1, procWindow.y1,
                                  procWindow.x2 - procWindow.x1,
                                  procWindow.y2 - procWindow.y1 );
        View dst = subimage_view( this->_dstView, procWindow.x1, procWindow.y1,
                                  procWindow.x2 - procWindow.x1,
                                  procWindow.y2 - procWindow.y1 );
        // Put your computations here
        copy_pixels( src, dst );
    }
    catch( PluginException err )
    {
        COUT_EXCEPTION( err );
    }
}

}
}