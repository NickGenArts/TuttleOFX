#include "OpenImageIOWriterDefinitions.hpp"
#include "OpenImageIOWriterPlugin.hpp"

#include <tuttle/plugin/image/gil/globals.hpp>
#include <tuttle/plugin/exceptions.hpp>

#include <OpenImageIO/imageio.h>

#include <boost/gil/gil_all.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/at.hpp>

namespace tuttle {
namespace plugin {
namespace openImageIO {
namespace writer {


template<class View>
OpenImageIOWriterProcess<View>::OpenImageIOWriterProcess( OpenImageIOWriterPlugin& instance )
	: ImageGilFilterProcessor<View>( instance )
	, _plugin( instance )
{
	this->setNoMultiThreading();
}

/**
 * @brief Function called by rendering thread each time a process must be done.
 * @param[in] procWindowRoW  Processing window in RoW
 * @warning no multithread here !
 */
template<class View>
void OpenImageIOWriterProcess<View>::multiThreadProcessImages( const OfxRectI& procWindowRoW )
{
	BOOST_ASSERT( procWindowRoW == this->_srcPixelRod );
	using namespace boost::gil;
	OpenImageIOWriterProcessParams params = _plugin.getProcessParams( this->_renderArgs.time );
	try
	{
		/// @todo tuttle: use params._components
		writeImage( this->_srcView, params._filepath, params._bitDepth );
	}
	catch( exception::Common& e )
	{
		e << exception::filename( params._filepath );
		throw;
	}
	catch(... )
	{
		BOOST_THROW_EXCEPTION( exception::Unknown()
			<< exception::user( "Unable to write image")
			<< exception::dev( boost::current_exception_diagnostic_information() )
			<< exception::filename(params._filepath) );
	}
	copy_pixels( this->_srcView, this->_dstView );
}

/**
 *
 */
template<class View>
void OpenImageIOWriterProcess<View>::writeImage( const View& src, const std::string& filepath, const OpenImageIO::TypeDesc bitDepth )
{
	using namespace boost;
	using namespace OpenImageIO;
	boost::scoped_ptr<ImageOutput> out( ImageOutput::create( filepath ) );
	if( out.get() == NULL )
	{
		BOOST_THROW_EXCEPTION( OFX::Exception::Suite( kOfxStatErrValue ) );
	}
	ImageSpec spec( src.width(), src.height(), gil::num_channels<View>::value, bitDepth );
	out->open( filepath, spec );

	typedef mpl::map<
	    mpl::pair<gil::bits8, mpl::integral_c<TypeDesc::BASETYPE, TypeDesc::UINT8> >,
	    mpl::pair<gil::bits16, mpl::integral_c<TypeDesc::BASETYPE, TypeDesc::UINT16> >,
	    mpl::pair<gil::bits32, mpl::integral_c<TypeDesc::BASETYPE, TypeDesc::UINT32> >,
	    mpl::pair<gil::bits32f, mpl::integral_c<TypeDesc::BASETYPE, TypeDesc::FLOAT> >
	    > MapBits;
	typedef typename gil::channel_type<View>::type ChannelType;

	const stride_t xstride = gil::is_planar<View>::value ? sizeof(Channel) : src.num_channels() * sizeof(Channel);
	const stride_t ystride = src.pixels().row_size(); // xstride * src.width();
//	const stride_t zstride = gil::is_planar<View>::value ? ystride * src.height() : sizeof(Channel);
	const stride_t zstride = ystride * src.height();
	
	out->write_image(
			mpl::at<MapBits, ChannelType>::type::value,
			&( ( *src.begin() )[0] ), // get the adress of the first channel value from the first pixel
			xstride,
			ystride,
			zstride,
			&progressCallback,
			this
		);
	out->close();
}

}
}
}
}
