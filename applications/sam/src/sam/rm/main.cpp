#include <sam/common/color.hpp>

#include <tuttle/common/clip/Sequence.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace bal = boost::algorithm;
namespace ttl = tuttle::common;
using namespace tuttle::common;

bool         colorOutput    = false;
bool         verbose        = false;
std::ssize_t firstImage     = 0;
std::ssize_t lastImage      = 0;

sam::Color _color;

// A helper function to simplify the main part.
template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
	copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, " "));
	return os;
}

void removeSequence( const ttl::Sequence& s )
{
	const std::ssize_t first = std::max( s.getFirstTime(), firstImage );
	const std::ssize_t last	 = std::min( s.getLastTime(), lastImage );
	
	//TUTTLE_COUT("remove from " << first << " to " << last);

	for( ttl::Sequence::Time t = first; t <= last; t += s.getStep() )
	{
		bfs::path sFile = s.getAbsoluteFilenameAt(t);
		if( !bfs::exists( sFile ) )
		{
			TUTTLE_CERR("Could not remove: " << _color._red << sFile.string() << _color._std );
		}
		else
		{
			if(verbose)
			{
				TUTTLE_COUT("remove: " << _color._folder << sFile.string() << _color._std );
			}
			bfs::remove( sFile );
		}
	}
}

void removeFileObject( std::list<boost::shared_ptr<ttl::FileObject> > &listing, std::vector<boost::filesystem::path> &notRemoved )
{
	BOOST_FOREACH( const std::list< boost::shared_ptr<ttl::FileObject> >::value_type & s, listing )
	{
		if( !(s->getMaskType () == ttl::eMaskTypeDirectory))
		{
			if(verbose)
				TUTTLE_COUT( "remove " << *s );
			if( s->getMaskType () == ttl::eMaskTypeSequence )
				removeSequence( (ttl::Sequence&) *s );
			else
			{
				std::vector<bfs::path> paths = s->getFiles();
				for(unsigned int i=0; i<paths.size(); i++)
					bfs::remove( paths.at(i) );
			}
		}
		else // is a directory
		{
			std::vector<boost::filesystem::path> paths = s->getFiles();
			for(unsigned int i=0; i<paths.size(); i++)
			{
				if( bfs::is_empty( paths.at(i) ) )
				{
					if(verbose)
						TUTTLE_COUT( "remove " << *s );
					bfs::remove( paths.at(i) );
				}
				else
				{
					notRemoved.push_back( paths.at(i) );
				}
			}
		}
	}
}

void removeFiles( std::vector<boost::filesystem::path> &listing )
{
	std::sort(listing.begin(), listing.end());
	std::reverse(listing.begin(), listing.end());
	BOOST_FOREACH( const boost::filesystem::path& paths, listing )
	{
		if(bfs::is_empty(paths))
		{
			if(verbose)
			{
				TUTTLE_COUT( "remove " << _color._folder << paths << _color._std );
			}
			bfs::remove(paths);
		}
		else
		{
			TUTTLE_CERR( "could not remove " << _color._error << paths << _color._std );
		}
	}
}


int main( int argc, char** argv )
{
	using namespace tuttle::common;

	EMaskType                    researchMask      = eMaskTypeSequence;	// by default show sequences
	EMaskOptions                 descriptionMask   = eMaskOptionsColor;	// by default show nothing
	bool                        recursiveListing  = false;
	std::vector<std::string>     paths;
	std::vector<std::string>     filters;

	// Declare the supported options.
	bpo::options_description mainOptions;
	mainOptions.add_options()
			("all,a"           , "do not ignore entries starting with .")
			("directories,d"   , "remove directories in path(s)")
			("expression,e"    , bpo::value<std::string>(), "remove with a specific pattern, ex: *.jpg,*.png")
			("files,f"         , "remove files in path(s)")
			("help,h"          , "show this help")
			("mask,m"          , "not remove sequences in path(s)")
			("path-root,p"     , "show the root path for each objects")
			("recursive,R"     , "remove subdirectories recursively")
			("verbose,v"       , "explain what is being done")
			("color"           , "display with colors")
			("first-image"     , bpo::value<std::ssize_t>(), "specify the first image")
			("last-image"      , bpo::value<std::ssize_t>(), "specify the last image")
			("full-rm"         , "remove directories, files and sequences")
			("brief"           , "brief summary of the tool")
			;
	
	// describe hidden options
	bpo::options_description hidden;
	hidden.add_options()
			("input-dir", bpo::value< std::vector<std::string> >(), "input directories")
			("enable-color", bpo::value<std::string>(), "enable (or disable) color")
			;
	
	// define default options 
	bpo::positional_options_description pod;
	pod.add("input-dir", -1);
	
	bpo::options_description cmdline_options;
	cmdline_options.add(mainOptions).add(hidden);

	bpo::positional_options_description pd;
	pd.add("", -1);
	
	//parse the command line, and put the result in vm
	bpo::variables_map vm;

	bpo::notify(vm);

	try
	{
		bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(pod).run(), vm);

		// get environment options and parse them
		if( const char* env_rm_options = std::getenv("SAM_RM_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix( env_rm_options, " " );
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
	}
	catch( const bpo::error& e)
	{
		TUTTLE_COUT( "sam-rm: command line error: " << e.what() );
		exit( -2 );
	}
	catch(...)
	{
		TUTTLE_COUT("sam-rm: unknown error in command line.");
		exit( -2 );
	}

	if ( vm.count("color") )
	{
		colorOutput = true;
	}
	if ( vm.count("enable-color") )
	{
		std::string str = vm["enable-color"].as<std::string>();

		if( str == "1" || boost::iequals(str, "y") || boost::iequals(str, "Y") || boost::iequals(str, "yes") || boost::iequals(str, "Yes") || boost::iequals(str, "true") || boost::iequals(str, "True") )
		{
			colorOutput = true;
		}
		else
		{
			colorOutput = false;
		}
	}

	if( colorOutput )
	{
		descriptionMask |= eMaskOptionsColor;
		_color.enable();
	}

	if ( vm.count( "help" ) )
	{
		TUTTLE_COUT( _color._blue  << "TuttleOFX project [http://sites.google.com/site/tuttleofx]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "NAME" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-rm - remove directory contents" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "SYNOPSIS" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-rm [options] [directories]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "DESCRIPTION" << _color._std << std::endl );
		TUTTLE_COUT( "Remove sequence of image files, and could remove trees (folder, files and sequences)." << std::endl );
		TUTTLE_COUT( _color._blue  << "OPTIONS" << _color._std << std::endl );
		TUTTLE_COUT( mainOptions );
		return 0;
	}

	if ( vm.count("brief") )
	{
		TUTTLE_COUT( _color._green << "remove directory contents" << _color._std);
		return 0;
	}

	if (vm.count("expression"))
	{
		bal::split( filters, vm["expression"].as<std::string>(), bal::is_any_of(","));
	}

	if (vm.count("directories"))
	{
		researchMask |= eMaskTypeDirectory;
	}
	
	if (vm.count("files"))
	{
		researchMask |= eMaskTypeFile;
	}
	
	if (vm.count("mask"))
	{
		researchMask &= ~eMaskTypeSequence;
	}
	
	if (vm.count("verbose"))
	{
		verbose = true;
	}

	if (vm.count("first-image"))
	{
		firstImage  = vm["first-image"].as< std::ssize_t >();
	}

	if (vm.count("last-image"))
	{
		lastImage  = vm["last-image"].as< std::ssize_t >();
	}

	if (vm.count("full-rm"))
	{
		researchMask |= eMaskTypeDirectory;
		researchMask |= eMaskTypeFile;
		researchMask |= eMaskTypeSequence;
	}
	
	if (vm.count("all"))
	{
		// add .* files
		descriptionMask |= eMaskOptionsDotFile;
	}
	
	if (vm.count("path-root"))
	{
		descriptionMask |= eMaskOptionsPath;
	}
	
	// defines paths, but if no directory specify in command line, we add the current path
	if (vm.count("input-dir"))
	{
		paths = vm["input-dir"].as< std::vector<std::string> >();
	}
	else
	{
		TUTTLE_COUT( _color._error << "No sequence and/or directory are specified." << _color._std );
		return 1;
	}

	if (vm.count("recursive"))
	{
		recursiveListing = true;
	}
	// 	for(unsigned int i=0; i<filters.size(); i++)
	// 	TUTTLE_COUT("filters = " << filters.at(i));
	// 	TUTTLE_COUT("research mask = " << researchMask);
	// 	TUTTLE_COUT("options  mask = " << descriptionMask);

	try
	{
		std::vector<boost::filesystem::path> pathsNoRemoved;
		BOOST_FOREACH( bfs::path path, paths )
		{
			//TUTTLE_COUT( "path: "<< path );
			if( bfs::exists( path ) )
			{
				if( bfs::is_directory( path ) )
				{
					//TUTTLE_COUT( "is a directory" );
					if(recursiveListing)
					{
						for ( bfs::recursive_directory_iterator end, dir(path); dir != end; ++dir )
						{
							if( bfs::is_directory( *dir ) )
							{
								//TUTTLE_COUT( *dir );
								std::list<boost::shared_ptr<FileObject> > listing = fileObjectsInDir( (bfs::path)*dir, filters, researchMask, descriptionMask );
								removeFileObject( listing, pathsNoRemoved );
							}
						}
					}
					std::list<boost::shared_ptr<FileObject> > listing = fileObjectsInDir( (bfs::path)path, filters, researchMask, descriptionMask );
					removeFileObject( listing, pathsNoRemoved );
				}
				else
				{
					//TUTTLE_COUT( "is NOT a directory "<< path.branch_path() << " | "<< path.leaf() );
					filters.push_back( path.leaf().string() );
					std::list<boost::shared_ptr<FileObject> > listing = fileObjectsInDir( (bfs::path)path.branch_path(), filters, researchMask, descriptionMask );
					removeFileObject( listing, pathsNoRemoved );
				}
			}
			else
			{
				//TUTTLE_COUT( "not exist ...." );
				try
				{
					Sequence s(path.branch_path(), descriptionMask );
					s.initFromDetection( path.string(), Sequence::ePatternDefault );
					if( s.getNbFiles() )
					{
						//TUTTLE_COUT( s );
						removeSequence( s );
					}
				}
				catch(... )
				{
					TUTTLE_CERR ( _color._error << "Unrecognized pattern \"" << path << "\"" << _color._std );
				}
			}
		}
		// delete not empty folder the first time
		removeFiles( pathsNoRemoved );
	}
	catch (bfs::filesystem_error &ex)
	{
		TUTTLE_COUT( _color._error << ex.what() << _color._std );
	}
	catch(... )
	{
		TUTTLE_CERR ( _color._error << boost::current_exception_diagnostic_information() << _color._std );
	}
	return 0;
}

