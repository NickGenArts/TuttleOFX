/**
 * @file MergeProcess.tcc
 * @brief
 * @author
 * @date    27/11/09 13:42
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


namespace tuttle {
namespace plugin {
namespace merge {

using namespace boost::gil;

struct FunctorPlus {
    template <typename SrcAV, typename SrcBV, typename DstV>
    void operator()(const SrcAV & srcA, const SrcBP & srcB, DstV & dst) const {
        dst = (DstV)(srcA + srcB);
    }
    template <typename SrcAV, typename SrcBV, typename DstV>
    DstV & operator()(const SrcAV & srcA, const SrcBP & srcB) const {
        return (DstV)(srcA + srcB);
    }
};

template<class View>
MergeProcess<View>::MergeProcess( MergePlugin &instance )
: tuttle::plugin::ImageGilProcessor<View>( instance )
, tuttle::plugin::Progress( instance )
, _plugin( instance )
{
}

template<class View>
void MergeProcess<View>::setupAndProcess( const OFX::RenderArguments &args )
{
    try
    {
        boost::scoped_ptr<OFX::Image> srcA( _plugin.getSrcClip( )->fetchImage( args.time ) );
        if( !srcA.get( ) )
            throw( ImageNotReadyException( ) );
        OfxRectI sBoundsA = srcA->getBounds( );
        OFX::BitDepthEnum srcBitDepthA = srcA->getPixelDepth( );
        OFX::PixelComponentEnum srcComponentsA = srcA->getPixelComponents( );

        boost::scoped_ptr<OFX::Image> srcB( _plugin.getSrcClip( )->fetchImage( args.time ) );
        if( !srcB.get( ) )
            throw( ImageNotReadyException( ) );

        OfxRectI sBounds = srcB->getBounds( );
        OFX::BitDepthEnum srcBitDepth = srcB->getPixelDepth( );
        OFX::PixelComponentEnum srcComponents = srcB->getPixelComponents( );

        // Build sources view
        this->_srcViewA = interleaved_view( std::abs( sBoundsA.x2 - sBoundsA.x1 ), std::abs( sBoundsA.y2 - sBoundsA.y1 ),
                                            static_cast < value_t* > ( srcA->getPixelData( ) ),
                                            srcA->getRowBytes( ) );

        this->_srcViewB = interleaved_view( std::abs( sBoundsB.x2 - sBoundsB.x1 ), std::abs( sBoundsB.y2 - sBoundsB.y1 ),
                                            static_cast < value_t* > ( srcB->getPixelData( ) ),
                                            srcB->getRowBytes( ) );

        boost::scoped_ptr<OFX::Image> dst( _plugin.getDstClip( )->fetchImage( args.time ) );
        if( !dst.get( ) )
            throw( ImageNotReadyException( ) );
        OfxRectI dBounds = dst->getBounds( );
        OFX::BitDepthEnum dstBitDepth = dst->getPixelDepth( );
        OFX::PixelComponentEnum dstComponents = dst->getPixelComponents( );

        // Make sure bit depths are same
        if( srcBitDepthA != dstBitDepth || srcComponentsA != dstComponents )
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
void MergeProcess<View>::multiThreadProcessImages( OfxRectI procWindow )
{
    try
    {
        View srcA = subimage_view( this->_srcViewA, procWindow.x1, procWindow.y1,
                                   procWindow.x2 - procWindow.x1,
                                   procWindow.y2 - procWindow.y1 );
        View srcB = subimage_view( this->_srcViewB, procWindow.x1, procWindow.y1,
                                   procWindow.x2 - procWindow.x1,
                                   procWindow.y2 - procWindow.y1 );
        View dst = subimage_view( this->_dstView, procWindow.x1, procWindow.y1,
                                  procWindow.x2 - procWindow.x1,
                                  procWindow.y2 - procWindow.y1 );
        transform_pixels(srcA, srcB, dst, default_color_merging<FunctorPlus>());
    }
    catch( PluginException err )
    {
        COUT_EXCEPTION( err );
    }
}

}
}
}